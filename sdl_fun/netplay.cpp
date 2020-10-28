
#include "netplay.h"

#include <time.h>
#include <string>

//Port Forwarding magic
#include "miniupnp/miniupnpc.h"
#include "miniupnp/upnpcommands.h"
#include "miniupnp/upnperrors.h"

//UPNP Devices we discovered
UPNPDev* upnp_devices = 0;

//Internet Gateway Device info
UPNPUrls upnp_urls;
IGDdatas upnp_data;
char aLanAddr[64];
int sessionPort = 7001;

extern Game* game;  //I dunno how I feel about this

int fletcher32_checksum(short* data, size_t len) {
   int sum1 = 0xffff, sum2 = 0xffff;

   while (len) {
      size_t tlen = len > 360 ? 360 : len;
      len -= tlen;
      do {
         sum1 += *data++;
         sum2 += sum1;
      } while (--tlen);
      sum1 = (sum1 & 0xffff) + (sum1 >> 16);
      sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   /* Second reduction step to reduce sums to 16 bits */
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   return sum2 << 16 | sum1;
}

int ggpoCheckSum(Game* game) {
   std::vector <Byte> stream = gameSave(game);
   int len = stream.size();
   if (len == 0) { return 0; }
   int checksum = fletcher32_checksum((short*)stream.data(), len / 2);
   return checksum;
}

void SetConnectState(GGPOPlayerHandle handle, PlayerConnectState state) {
   for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
      if (game->net->connections[i].handle == handle) {
         game->net->connections[i].connect_progress = 0;
         game->net->connections[i].state = state;
         break;
      }
   }
}

void SetDisconnectTimeout(GGPOPlayerHandle handle, int when, int timeout) {
   for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
      if (game->net->connections[i].handle == handle) {
         game->net->connections[i].disconnect_start = when;
         game->net->connections[i].disconnect_timeout = timeout;
         game->net->connections[i].state = Disconnecting;
         break;
      }
   }
}

void SetConnectState(PlayerConnectState state) {
   for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
      game->net->connections[i].state = state;
   }
}

void UpdateConnectProgress(GGPOPlayerHandle handle, int progress) {
   for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
      if (game->net->connections[i].handle == handle) {
         game->net->connections[i].connect_progress = progress;
         break;
      }
   }
}

//Don't call it a callback!
bool __cdecl ds_begin_game_callback(const char*) {

   //we don't need to do anything here apparently
   return true;
}

bool __cdecl ds_advance_frame_callback(int) {

   int disconnect_flags;

   //Figure out the inputs and check for disconnects
   ggpo_synchronize_input(game->net->ggpo, (void*)game->inputs, sizeof(UserInput) * game->players, &disconnect_flags);

   //Call function to advance frame
   gameAdvanceFrame(game);

   return true;
}

bool __cdecl ds_load_game_callback(unsigned char* buffer, int len) {

   if (len > 0) {
      unsigned char* start = buffer;
      gameCallbackLoad(game, start);

      return true;
   }
   else { return false; }
}

bool __cdecl ds_save_game_callback(unsigned char** buffer, int* len, int* checksum, int) {

   std::vector <Byte> stream = gameSave(game);
   if (stream.size() == 0) { return false; }
   *len = stream.size();

   *buffer = (unsigned char*) malloc(*len);
   if (buffer) {
      memcpy(*buffer, stream.data(), *len);
      *checksum = fletcher32_checksum((short*)*buffer, *len / 2);

      return true;
   }
   else { return false; }

}

void __cdecl ds_free_buffer_callback(void* buffer) {
   if (buffer) { 
      free(buffer); 
   }
}

bool __cdecl ds_on_event_callback(GGPOEvent* info) {
   int progress;
   GGPOErrorCode result = GGPO_OK;
   int disconnect_flags = 0;

   switch (info->code) {
   case GGPO_EVENTCODE_CONNECTED_TO_PEER:
       SetConnectState(info->u.connected.player, Synchronizing);
       break;
   case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
       progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
       UpdateConnectProgress(info->u.synchronizing.player, progress);
       break;
   case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
       UpdateConnectProgress(info->u.synchronized.player, 100);
       break;
   case GGPO_EVENTCODE_RUNNING:
       SetConnectState(Running);
       break;
   case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
       SetDisconnectTimeout(info->u.connection_interrupted.player, game->kt.getTime(), info->u.connection_interrupted.disconnect_timeout);
       popupEnable(Popup_Disconnect);
       break;
   case GGPO_EVENTCODE_CONNECTION_RESUMED:
       SetConnectState(info->u.connection_resumed.player, Running);
       popupDisable(Popup_Disconnect);
       game->net->timeSync = 0;
       break;
   case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
       SetConnectState(info->u.disconnected.player, Disconnected);
       break;
   case GGPO_EVENTCODE_TIMESYNC:
       int maxFrames = 10;  //Max prediction frames is 8, so 10 sounds ok, I guess
       int ahead = info->u.timesync.frames_ahead;
       if (ahead > 0 && ahead < maxFrames) { game->net->timeSync = ahead; }
       //popupEnable(Popup_Waiting, ahead);  //This window is usually too fast to see
       //if (ahead > 0 && ahead < maxFrames) {sdlSleep(1000 * info->u.timesync.frames_ahead / 60)};
       break;
   }
   return true;
}

bool __cdecl ds_log_game_state_callback(char* filename, unsigned char* buffer, int len) {
   //This goes into sync log... which I disabled in GGPO synctest right now

   return true;
}

//Discover if the local gateway device can use UPNP and port-forward the local port
static bool upnpStartup(int port) {
   int error, status;
   upnp_devices = upnpDiscover(2000, NULL, NULL, 0, 0, 2, &error);
   if (error == 0) {
      status = UPNP_GetValidIGD(upnp_devices, &upnp_urls, &upnp_data, aLanAddr, sizeof(aLanAddr));
      //   if (status == 1) { printf("Found a valid IGD: %s", upnp_urls.controlURL); }
      if (status == 1) {
         game->net->messages.push_back("Discovered a device that can use UPNP"); 
         char upnpPort[32];
         sprintf(upnpPort, "%d", port);
         error = UPNP_AddPortMapping(upnp_urls.controlURL, upnp_data.first.servicetype, upnpPort, upnpPort, aLanAddr, "Drop and Swap", "UDP", 0, "0");
         if (!error) {
            game->net->messages.push_back("Port Mapping Complete"); 
            return true;
         }
      }
   }
   game->net->messages.push_back("Failed to complete port mapping");
   return false;
}

int upnpAddPort(int port) {
   int error;
   char upnpPort[32];
   sprintf(upnpPort, "%d", port);
   error = UPNP_AddPortMapping(upnp_urls.controlURL, upnp_data.first.servicetype, upnpPort, upnpPort, aLanAddr, "Drop and Swap", "UDP", 0, "0");
   if (!error) {
      game->net->messages.push_back("Port Mapping Complete");
      return true;
   }
}

int upnpDeletePort(int port) {
   char upnpPort[32];
   sprintf(upnpPort, "%d", port);

   int error = UPNP_DeletePortMapping(upnp_urls.controlURL, upnp_data.first.servicetype, upnpPort, "UDP", 0);

   //if (error != 0) { printf("Failed to delete port: %s", strupnperror(error)); }

   return 1;
}

//Deletes local port mapping and free resources for UPNP devices/urls
static bool upnpCleanup(int port) {
   char upnpPort[32];
   sprintf(upnpPort, "%d", port);

   int error = UPNP_DeletePortMapping(upnp_urls.controlURL, upnp_data.first.servicetype, upnpPort, "UDP", 0);

   //if (error != 0) { printf("Failed to delete port: %s", strupnperror(error)); }

   if (upnp_devices) { 
      FreeUPNPUrls(&upnp_urls);
      freeUPNPDevlist(upnp_devices); 
   }
   return error;
}

//Create a GGPO session and add players/spectators 
void ggpoCreateSession(Game* game, SessionInfo connects[], unsigned short participants) {
   GGPOErrorCode result;

   //Called at startup to setup GGPO session
   GGPOSessionCallbacks cb;
   cb.begin_game = ds_begin_game_callback;
   cb.advance_frame = ds_advance_frame_callback;  
   cb.load_game_state = ds_load_game_callback;
   cb.save_game_state = ds_save_game_callback;
   cb.free_buffer = ds_free_buffer_callback;
   cb.on_event = ds_on_event_callback;  
   cb.log_game_state = ds_log_game_state_callback;  //This is turned off right now

   int hostNumber = -1;
   int myNumber = -1;
   int spectators = 0;
   for (int i = 0; i < participants; i++) {
      if (connects[i].host == true) { hostNumber = i; }  //Who is hosting
      if (connects[i].me == true) { myNumber = i; }  //who am i? who am i?
      if (connects[i].playerType == 1) { spectators++; }
   }

   game->net->myConnNum = myNumber;
   game->net->hostConnNum = hostNumber;

   sessionPort = connects[myNumber].localPort;  //Start the session using my port
   if (game->net->useUPNP) { int upnpSuccess = upnpAddPort(sessionPort); }

   if (game->syncTest == true) {  //Set syncTest to true to do a single player sync test
      char name[] = "DropAndSwap";
      result = ggpo_start_synctest(&game->net->ggpo, &cb, name, game->players, sizeof(UserInput), 1);
      game->players = 2;
   }
   else if (connects[myNumber].playerType == 1){  //Spectating a GGPO Session
      result = ggpo_start_spectating(&game->net->ggpo, &cb, "DropAndSwap", participants - spectators, sizeof(UserInput), sessionPort, connects[hostNumber].ipAddress, connects[hostNumber].localPort);
      game->players = participants - spectators;
      return;  //And we're done
   }
   else {  //Start a regular GGPO Session
      result = ggpo_start_session(&game->net->ggpo, &cb, "DropAndSwap", participants - spectators, sizeof(UserInput), sessionPort);
   }

   if (result == GGPO_OK) { game->net->messages.push_back("Started GGPO Session"); }

   // Give disconnects notification after 1000 ms and then disconnect clients after xxxx ms
   ggpo_set_disconnect_timeout(game->net->ggpo, game->net->disconnectTime[0]);  
   ggpo_set_disconnect_notify_start(game->net->ggpo, 1000);

   int pNum = 1;
   for (int i = 0; i < participants; i++) {  //Fill in GGPOPlayer struct
      if (hostNumber != myNumber && connects[i].playerType == 1) { continue; }  //Host takes care of spectators
      if (connects[i].playerType != 1) {  //Spectators don't have input size or player number
         game->net->players[i].size = sizeof(GGPOPlayer);
         game->net->players[i].player_num = pNum;
         pNum++;
      }
      if (connects[i].me == true && connects[i].playerType == 0) { game->net->players[i].type = GGPO_PLAYERTYPE_LOCAL; } //I'm a local player
      else if (connects[i].playerType == 1) { game->net->players[i].type = GGPO_PLAYERTYPE_SPECTATOR; } //I'm a spectator
      else if (connects[i].playerType == 0) { game->net->players[i].type = GGPO_PLAYERTYPE_REMOTE; }

      if (i != myNumber) {  //Everybody else is remote
         strcpy(game->net->players[i].u.remote.ip_address, connects[i].ipAddress);
         game->net->players[i].u.remote.port = connects[i].localPort;
      }

      GGPOPlayerHandle handle;
      result = ggpo_add_player(game->net->ggpo, &game->net->players[i], &handle);  //Add a player to GGPO session
      if (result != GGPO_OK) { printf("Couldn't add player %d", i); }  //todo add a popup here

      game->net->connections[i].handle = handle;
      game->net->connections[i].type = game->net->players[i].type;

      if (game->net->players[i].type == GGPO_PLAYERTYPE_LOCAL) {
         game->net->localPlayer = handle;
         ggpo_set_frame_delay(game->net->ggpo, handle, game->net->frameDelay[0]);
      }
   }
   if (result == GGPO_OK) { 
      game->net->messages.push_back("Added player slots"); 
      game->players = participants - spectators;
   }
}

//Updates the game, notifies GGPO and advances the frame
void gameAdvanceFrame(Game* game) {
   for (int i = 0; i < game->players; i++) {  //Check for pauses
      gameCheckPause(game, game->inputs[i]);
   }
   gameUpdate(game);
   ggpo_advance_frame(game->net->ggpo);  //Tell GGPO we moved ahead a frame
}

//Used to synchronize inputs in GGPO and advance the frame
void gameRunFrame() {
   if (game->playing == false) { return; }
   if (game->net->timeSync > 0) {  //We're too many frames ahead
      game->net->timeSync--;
      return;
   }
   if (game->net && game->net->ggpo) {

      GGPOErrorCode result = GGPO_OK;
      int disconnect_flags;

      if (game->net->localPlayer != GGPO_INVALID_HANDLE) {  //Add local inputs for valid players
         processInputs(game); 
         if (game->ai == true) {        
            if (game->syncTest == false) { gameAI(game, game->net->localPlayer - 1); }
            else { gameAI(game, 0); }
         }
         result = ggpo_add_local_input(game->net->ggpo, game->net->localPlayer, &game->p1Input, sizeof(UserInput));
      }
      //If we got the local inputs successfully, merge in remote ones
      if (GGPO_SUCCEEDED(result)) {
         result = ggpo_synchronize_input(game->net->ggpo, (void*)game->inputs, sizeof(UserInput) * game->players, &disconnect_flags);
         if (GGPO_SUCCEEDED(result)) {
            gameAdvanceFrame(game);  //Update the game 
         }
      }
   }
}

//End a GGPO session
void ggpoClose(GGPOSession* ggpo) {
   if (ggpo) {
      ggpo_close_session(ggpo);
   }
   if (game->net->useUPNP) { upnpCleanup(sessionPort); }
}

//Display the connection status based on the PlayerConnectState Enum
const char* ggpoShowStatus(Game* game, int playerIndex) {
   const char* out = "";
   if (game->net) {
      switch (game->net->connections[playerIndex].state) {
      case Connecting:
         out = "Connecting";
         break;
      case Synchronizing:
         out = "Synchronizing";
         break;
      case Running:
         out = "Running";
         break;
      case Disconnected:
         out = "Disconnected";
         break;
      case Disconnecting:
         out = "Disconnecting";
         break;
      default:
         out = "";
         break;
      }
   }
   return out;
}

//Disconnect a player by number
int ggpoDisconnectPlayer(int player) {
   GGPOErrorCode result = ggpo_disconnect_player(game->net->ggpo, player);
   if (GGPO_SUCCEEDED(result)) {
      return 1;
   }
   else return 0;
}

//Disconnect players and recreate game->net
void ggpoEndSession(Game* game) {
   if (game->net) {
      ggpoClose(game->net->ggpo);
      int frameDelay = game->net->frameDelay[0];
      int disconnectTime = game->net->disconnectTime[0];
      bool useUPNP = game->net->useUPNP;
      delete game->net;

      game->net = new NetPlay;
      game->net->useUPNP = useUPNP;
      game->net->frameDelay[0] = frameDelay;
      game->net->disconnectTime[0] = disconnectTime;
   }
}

u_long mode = 1;  //enables non-block socket
int connections = 0;  //How many accepted connections do we have

std::vector <Byte> matchInfo;
std::vector <std::string> sockMess;
std::map <int, SocketInfo> sockets;

//Send a message of a given size over the socket
bool sendMsg(SOCKET socket, const char* buffer, int len) {
   int result = send(socket, buffer, len, 0);
   if (result != SOCKET_ERROR) { return true; }  //Failed to send
   return false;
}

//Receive a message of a known length over the socket
bool recMsg(SOCKET socket, char* buffer, int len) {
   int result = recv(socket, buffer, len, 0);
   if (result <= 0) { return false; }  //No message or error
   if (result > 0) { return true; }  //We got something
}

//Create a start listening on a socket
bool tcpHostListen(int port) {
   bool static upnp = false;
   //create socket and verify
   sockets[-1].sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sockets[-1].sock == -1) { sockMess.push_back("Socket creation failed."); }

   //ioctlsocket(sockets[-1].sock, FIONBIO, &mode);  //Make socket non-blocking

   if (upnp == false) { upnp = upnpStartup(port); }

   //assign IP and Port
   sockets[-1].address.sin_family = AF_INET;
   sockets[-1].address.sin_addr.s_addr = htonl(INADDR_ANY);  //htonl converts ulong to tcp/ip network byte order
   sockets[-1].address.sin_port = htons(port);

   //bind socket
   int bindResult = bind(sockets[-1].sock, (sockaddr*)&sockets[-1].address, sizeof(sockets[-1].address));
   if (bindResult != 0) {
      sockMess.push_back("socket binding failed...");
      return false;
   }

   //start listening on socket
   if (listen(sockets[-1].sock, 5) != 0) {
      sockMess.push_back("Listen failed...");
      return false;
   }

   upnp = false;
   game->net->messages.push_back("Started listening for player connections");
   return true;
}

//Accepts connections on the listening socket and adds the info to the list of sockets
char* tcpHostAccept() {
   int len = sizeof(sockets[connections].address);
   SOCKET conn = accept(sockets[-1].sock, (sockaddr*)&sockets[connections].address, &len);
   if (conn == INVALID_SOCKET) { 
      if (WSAGetLastError() == WSAEWOULDBLOCK) { return "Nothing to accept..."; }
      else { return "socket accept failed..."; }
   }
   sockets[connections].sock = conn;
   connections++;
}

//Connect to a given port on the host
bool tcpClientConnect(int port, const char* ip) {
   bool static upnp = false;
   //create socket and verify
   sockets[-1].sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sockets[-1].sock == -1) { return "Socket creation failed."; }

   //ioctlsocket(sockets[-1].sock, FIONBIO, &mode);  //Make socket non-blocking

   //assign IP and Port
   sockets[-1].address.sin_family = AF_INET;
   sockets[-1].address.sin_addr.s_addr = inet_addr(ip);
   sockets[-1].address.sin_port = htons(port);

   if (upnp == false) { upnp = upnpStartup(port); }

   int result = connect(sockets[-1].sock, (sockaddr*)&sockets[-1].address, sizeof(sockets[-1].address));
   if (result != 0) {
      sockMess.push_back("Socket connection failed.");
      return false;
   }
   //else if (result == WSAEWOULDBLOCK) { //Would have blocked the application
   //   sockMess.push_back("Connecting would have blocked...");
   //   return false;
   //}
   upnp = false;
   game->net->messages.push_back("Connected to host");
   return true;
}

void tcpCleanup(int port) {
   for (auto&& sock : sockets) {
      closesocket(sock.second.sock);
   }
   sockets.clear();
   connections = 0;
   sockMess.clear();

   upnpDeletePort(port);
   //WSACleanup();
}

void tcpServerLoop(int port, int people, ServerStatus &status) {
   while (status != server_done) {
      bool done = true;
      switch (status) {
      case server_started:
         if (tcpHostListen(port) == true) { status = server_listening; }
         break;

      case server_listening:
         if (connections == people) {
            status = server_receive;
         }
         else if (connections < people) { tcpHostAccept(); }
         break;

      case server_receive:
         for (int i = 0; i < connections; i++) {
            if (sockets[i].status != sock_received) {
               if (recMsg(sockets[i].sock, sockets[i].recBuff, BUFFERLEN) == false) { done = false; }
               else {
                  sockets[i].status = sock_received;
                  strcpy(sockets[i].name, sockets[i].recBuff);
               }
            }
         }
         if (done == true) { 
            status = server_waiting; 
            game->net->messages.push_back("All players connected");
         }
         break;

      case server_waiting:
         break;

      case server_send:
         for (int i = 0; i < connections; i++) {
            if (sockets[i].status != sock_sent) {
               matchInfo = gameSave(game);  //Serialize the game settings
               serializeGameSetup(game, matchInfo);  //Serialize the host setup
               if (sendMsg(sockets[i].sock, (char*)matchInfo.data(), matchInfo.size()) == false) {
                  done = false;
               }
               else { sockets[i].status = sock_sent; }
            }
         }
         if (done == true) { 
            status = server_ready; 
            game->net->messages.push_back("Game info sent. Waiting for responses.");
         }
         break;

      case server_ready:
         for (int i = 0; i < connections; i++) {
            if (sockets[i].status != sock_ready) {
               if (recMsg(sockets[i].sock, sockets[i].recBuff, BUFFERLEN) == false) { done = false; }
               else {
                  sockets[i].status = sock_ready;
               }
            }
         }
         if (done == true) { status = server_done; }
         break;

      case server_done:
         //Write something here
         break;
      }
   }
}

void tcpClientLoop(int port, const char* ip, ClientStatus &status, const char* name) {
   while (status != client_done) {
      switch (status) {
      case client_started:
         if (tcpClientConnect(port, ip) == true) { status = client_connected; }
         break;
      case client_connected:
         if (sendMsg(sockets[-1].sock, name, strlen(name)) == true) { status = client_sent; }  //20 is the size of pName
         break;
      case client_sent:
         if (recMsg(sockets[-1].sock, sockets[-1].recBuff, BUFFERLEN) == true) { status = client_received; }
         break;
      case client_received:
         readGameData();
         for (int i = 0; i < game->players; i++) {
            if (strcmp(game->net->hostSetup[i].name, game->pName) == 0) {
               game->net->hostSetup[i].me = true;
            }
         }
         game->net->messages.push_back("Game data received and loaded. Hit Start when ready.");
         status = client_waiting;
         break;
      case client_waiting:
         break;
      case client_loaded:
         if (sendMsg(sockets[-1].sock, "R", 1) == true) { status = client_done; }  //We are ready to play
         break;
      }
   }
}

SocketInfo getSocket(int index) {
   return sockets[index];
}

void readGameData() {
   unsigned char* gData = (unsigned char*)sockets[-1].recBuff;
   gameLoad(game, gData);
   deserializeGameSetup(game, gData);
}