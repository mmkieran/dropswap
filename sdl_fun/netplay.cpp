
#include "netplay.h"

#include <time.h>
#include <thread>
#include <random>

#include "imgui/imgui.h"

//Port Forwarding magic
#include "miniupnp/miniupnpc.h"
#include "miniupnp/upnpcommands.h"
#include "miniupnp/upnperrors.h"

//Globals used by UPNP
UPNPDev* upnp_devices = 0;
UPNPUrls upnp_urls;
IGDdatas upnp_data;
char aLanAddr[64];
u_short sessionPort = 7001;                        //What port does GGPO (UDP) use?
bool udpPortMapped = false;                        //Did we map a UDP port using UPNP?

//Globals used by TCP sockets
int connections = 0;                               //How many accepted connections do we have
std::vector <Byte> matchInfo;                      //Vector to store saved game data
std::map <int, SocketInfo> sockets;                //Connections between host and player... -1 is the listening/connecting socket
std::map <unsigned short, bool> tcpPorts = {       //List of potential TCP ports and if they are open for UPNP
   {7000, false}, {7001, false}, {7002, false}, 
   {7003, false}, {7004, false}, {7005, false},
};
std::vector <Byte> myInfo;                          //Randomly generated ID for the current user
FILE* dsLog = nullptr;                             //Handle for the log file

extern Game* game;                                 //Used to access the game pointer from Main.. I dunno how I feel about this

static void readGameData();
int fletcher32_checksum(short* data, size_t len);

//Debug log functions
bool netlogCreate() {
   int err = fopen_s(&dsLog, "saves/dsNetLog.txt", "w");
   if (err == 0) { 
      fprintf(dsLog, "Started dropswap net log...\n");
      fclose(dsLog);
      return true; 
   }
   else { return false; }
}

template <typename... Args>  //Template parameter pack (variadic arguments)
bool netlogWrite(const char* input, Args... args) {  //Function parameter pack
   int err = fopen_s(&dsLog, "saves/dsNetLog.txt", "a");
   if (err == 0) {
      fprintf(dsLog, input, args...);  //Pass function parameter pack to file write
      fclose(dsLog);
      return true;
   }
   else { return false; }
}

void netlogClose() {
   fclose(dsLog);
}

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

ConnState getServerConnState() {
   return game->net->hostSetup[game->net->hostConnNum].state;
}

ConnState getMyConnState() {
   return game->net->hostSetup[game->user.number - 1].state;
}
void SetConnectState(int handle, ConnState state) {
   game->net->hostSetup[handle - 1].state = state;
}

void SetConnectStateAll(ConnState state) {
   for (int i = 0; i < game->net->participants; i++) {
      game->net->hostSetup[i].state = state;
   }
}

void SetDisconnectTimeout(int handle, int when, int timeout) {
   game->net->hostSetup[handle - 1].dcStart = when;
   game->net->hostSetup[handle - 1].dcTime = timeout;
   game->net->hostSetup[handle - 1].state = Disconnecting;
}

//Don't call it a callback!
bool __cdecl ds_begin_game_callback(const char*) {

   //we don't need to do anything here apparently
   return true;
}

bool __cdecl ds_advance_frame_callback(int) {

   int disconnect_flags;

   //Figure out the inputs and check for disconnects
   ggpo_synchronize_input(game->net->ggpo, (void*)game->net->inputs, sizeof(UserInput) * game->players, &disconnect_flags);

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
       SetConnectState(info->u.connected.player, PeerFound);
       break;
   case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
       SetConnectState(info->u.disconnected.player, Synchronizing);
       break;
   case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
       SetConnectState(info->u.disconnected.player, Synched);
       break;
   case GGPO_EVENTCODE_RUNNING:
       SetConnectStateAll(Running);
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

bool winsockStart() {
   WSADATA wd = { 0 };  //Initialize windows socket... Error 10093 means it wasn't started
   int wsaResult = WSAStartup(MAKEWORD(2, 2), &wd);  //This was a lot of trouble for 2 lines of code >.<

   if (wsaResult != 0) {
      game->net->messages.push_back("WSAStartup failed");
      return false;
   }
   return true;
}

void winsockCleanup() {
   WSACleanup();  //Cleanup the socket stuff
}

//Discover if the local gateway device can use UPNP and port-forward the local port
void upnpStartup(Game* game) {
   if (game->net->netlog == true) { netlogCreate(); }  //todo dunno if this belongs here
   int error, status;
   upnp_devices = upnpDiscover(2000, NULL, NULL, 0, 0, 2, &error);
   if (error == 0) {
      status = UPNP_GetValidIGD(upnp_devices, &upnp_urls, &upnp_data, aLanAddr, sizeof(aLanAddr));
      if (status == 1) { 
         game->upnpRunning = true;
         return; 
      }
      else if (game->net->netlog == true) { netlogWrite("Failed to find a valid IGD device: %d\n", status); }
   }
   else if (game->net->netlog == true) { netlogWrite("UPNP discovery failed with error: %d\n", error); }
   game->upnpRunning = false;
}

int upnpAddPort(u_short port, const char* protocol) {
   int error;
   char upnpPort[32];
   sprintf(upnpPort, "%d", port);
   error = UPNP_AddPortMapping(upnp_urls.controlURL, upnp_data.first.servicetype, upnpPort, upnpPort, aLanAddr, "Drop and Swap", protocol, 0, "0");
   if (!error) {
      game->net->messages.push_back("Port mapping complete");
      return true;
   }
   else if (game->net->netlog == true) { netlogWrite("UPNP Port %d mapping failed with error: %s\n", port, strupnperror(error)); }
}

int upnpDeletePort(u_short port, const char* protocol) {
   char upnpPort[32];
   sprintf(upnpPort, "%d", port);

   int error = UPNP_DeletePortMapping(upnp_urls.controlURL, upnp_data.first.servicetype, upnpPort, protocol, 0);

   if (error != 0 && game->net->netlog == true) { netlogWrite("UPNP failed to delete port %d: %s\n", port, strupnperror(error)); }

   return 1;
}

//Free resources for UPNP devices/urls
void upnpCleanup() {
   if (upnp_devices) { 
      FreeUPNPUrls(&upnp_urls);
      freeUPNPDevlist(upnp_devices); 
   }
   //if (game->net->netlog == true) { netlogClose(); }
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

   game->pList.clear();  //Clear the player list

   int hostNumber = -1;
   int myNumber = -1;
   int spectators = 0;
   for (int i = 0; i < participants; i++) {
      if (connects[i].host == true) { hostNumber = i; }  //Who is hosting
      if (connects[i].me == true) { myNumber = i; }  //who am i? who am i?
      if (connects[i].playerType == 1) { spectators++; }  //todo we can separate spectators into another list here if needed
   }
   game->net->hostConnNum = hostNumber;

   sessionPort = connects[myNumber].localPort;  //Start the session using my port
   udpPortMapped = upnpAddPort(sessionPort, "UDP"); 

   if (game->net->syncTest == true) {  //Set syncTest to true to do a single player sync test
      char name[] = "DropAndSwap";
      game->players = 2;
      result = ggpo_start_synctest(&game->net->ggpo, &cb, name, game->players, sizeof(UserInput), 1);
   }
   else if (connects[myNumber].playerType == 1) {  //Spectating a GGPO Session
      game->players = participants - spectators;
      result = ggpo_start_spectating(&game->net->ggpo, &cb, "DropAndSwap", game->players, sizeof(UserInput), sessionPort, connects[hostNumber].ipAddress, connects[hostNumber].localPort);

      game->user.number = -1;  //todo check this later
      for (int i = 0; i < participants; i++) {  //Fill in GGPOPlayer struct
         if (connects[i].playerType == 0) {
            game->pList[connects[i].pNum].number = connects[i].pNum;
            game->pList[connects[i].pNum].team = connects[i].team;
            strcpy(game->pList[connects[i].pNum].name, connects[i].name);
         }
      }

      return;  //And we're done
   }
   else {  //Start a regular GGPO Session
      result = ggpo_start_session(&game->net->ggpo, &cb, "DropAndSwap", participants - spectators, sizeof(UserInput), sessionPort);
   }

   if (result == GGPO_OK) { game->net->messages.push_back("Started GGPO Session"); }

   // Give disconnects notification after 1000 ms and then disconnect clients after xxxx ms
   ggpo_set_disconnect_timeout(game->net->ggpo, game->net->disconnectTime[0]);  
   ggpo_set_disconnect_notify_start(game->net->ggpo, 1000);

   for (int i = 0; i < participants; i++) {  //Fill in GGPOPlayer struct
      if (hostNumber != myNumber && connects[i].playerType == 1) { continue; }  //Host takes care of spectators
      if (connects[i].playerType != 1) {  //Spectators don't have input size or player number
         game->net->players[i].size = sizeof(GGPOPlayer);
         game->net->players[i].player_num = connects[i].pNum;
      }
      if (connects[i].me == true && connects[i].playerType == 0) { game->net->players[i].type = GGPO_PLAYERTYPE_LOCAL; } //I'm a local player
      else if (connects[i].playerType == 1) { game->net->players[i].type = GGPO_PLAYERTYPE_SPECTATOR; } //I'm a spectator
      else if (connects[i].playerType == 0) { game->net->players[i].type = GGPO_PLAYERTYPE_REMOTE; }

      if (i != myNumber) {  //Everybody else is remote
         strcpy(game->net->players[i].u.remote.ip_address, connects[i].ipAddress);
         game->net->players[i].u.remote.port = connects[i].localPort;
      }

      GGPOPlayerHandle handle = -1;
      result = ggpo_add_player(game->net->ggpo, &game->net->players[i], &handle);  //Add a player to GGPO session
      if (result != GGPO_OK) { printf("Couldn't add player %d", i); }  //todo add a popup here

      if (game->net->players[i].type != GGPO_PLAYERTYPE_SPECTATOR) {  //Populate player list
         game->pList[handle].number = handle;
         game->pList[handle].team = connects[i].team;
         strcpy(game->pList[handle].name, connects[i].name);
      }

      if (game->net->players[i].type == GGPO_PLAYERTYPE_LOCAL) {
         game->user.number = handle;  //todo check this later
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
      gameCheckPause(game, game->net->inputs[i]);
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

      if (game->user.number != GGPO_INVALID_HANDLE) {  //Add local inputs for valid players
         processInputs(game); 
         if (game->ai == true) { gameAI(game); }
         result = ggpo_add_local_input(game->net->ggpo, game->user.number, &game->user.input, sizeof(UserInput));
      }
      //If we got the local inputs successfully, merge in remote ones
      if (GGPO_SUCCEEDED(result)) {
         result = ggpo_synchronize_input(game->net->ggpo, (void*)game->net->inputs, sizeof(UserInput) * game->players, &disconnect_flags);
         if (GGPO_SUCCEEDED(result)) {
            gameAdvanceFrame(game);  //Update the game 
         }
      }
   }
}

//End a GGPO session
void ggpoClose(GGPOSession* ggpo) {
   if (ggpo != nullptr) {
      ggpo_close_session(ggpo);
      game->net->ggpo = nullptr;
   }
   if (udpPortMapped == true) { upnpDeletePort(sessionPort, "UDP"); }
}

//Display the connection status based on the PlayerConnectState Enum
const char* ggpoShowStatus(Game* game, int playerIndex) {
   const char* out = "";
   if (game->net) {
      switch (game->net->hostSetup[playerIndex].state) {
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
      game->net->hostConnNum = -1;
      game->net->timeSync = 0;
      game->net->participants = 0;
      game->net->messages.clear();
      memset(&game->net->hostSetup[0], 0, sizeof(SessionInfo) * GAME_MAX_PLAYERS);
   }
}

//Send a message of a given size over the socket
bool sendMsg(SOCKET socket, const char* buffer, int len) {
   static int lastResult = 0;
   int result = send(socket, buffer, len, 0);  //send returns the number of bytes sent
   if (result == SOCKET_ERROR) {
      if (game->net->netlog == true && lastResult != result) {
         netlogWrite("Failed to send message: %d\n", WSAGetLastError());
         lastResult = result;
      }
      return false;
   }
   lastResult = 0;
   return true;
}

//Receive a message of a known length over the socket
bool recMsg(SOCKET socket, char* buffer, int len) {
   static int lastResult = 1;
   int result = recv(socket, buffer, len, 0);  //recv returns number of bytes received
   if (result > 0) { //We got something
      lastResult = 1;
      return true; 
   }  
   else if (result == 0 && result != lastResult) {
      if (game->net->netlog == true && result != lastResult) {
         //todo specially handle closed connection
         netlogWrite("Failed to receive message: %d\n", WSAGetLastError());
      }
      lastResult = result;
   }
   else if (result == SOCKET_ERROR) { 
      if (game->net->netlog == true && result != lastResult) {
         netlogWrite("Failed to receive message: %d\n", WSAGetLastError());
      }
      lastResult = result;
   }
   return false;
}

//Create a start listening on a socket
bool tcpHostListen(u_short port) {
   //create socket and verify
   static int lastCreate = 0;
   sockets[-1].sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sockets[-1].sock == INVALID_SOCKET) { 
      if (game->net->netlog == true && sockets[-1].sock != lastCreate) {
         netlogWrite("Host failed create socket: %d\n", WSAGetLastError()); 
         lastCreate = sockets[-1].sock;
      }
      return false;
   }
   lastCreate = 0;

   //assign IP and Port
   sockets[-1].address.sin_family = AF_INET;
   sockets[-1].address.sin_addr.s_addr = htonl(INADDR_ANY);  //htonl converts ulong to tcp/ip network byte order
   sockets[-1].address.sin_port = htons(port);

   if (game->net->upnp == true && game->upnpRunning == true && tcpPorts[port] == false) {
      bool upnp = upnpAddPort(port, "TCP"); 
      if (upnp) { tcpPorts[port] = true; }
      else { return false; }
   }

   //bind socket
   static int lastBind = 0;
   int bindResult = bind(sockets[-1].sock, (sockaddr*)&sockets[-1].address, sizeof(sockets[-1].address) );
   if (bindResult == SOCKET_ERROR) {
      if (game->net->netlog == true && bindResult != lastBind) {
         netlogWrite("Host failed to bind socket: %d\n", WSAGetLastError()); 
         lastBind = bindResult;
      }
      return false;
   }
   lastBind = 0;

   //start listening on socket
   static int lastListen = 0;
   int listenResult = listen(sockets[-1].sock, 5);
   if (listenResult == SOCKET_ERROR) {
      if (game->net->netlog == true && listenResult != lastListen) {
         netlogWrite("Host failed to start listening: %d\n", WSAGetLastError()); 
         lastListen = listenResult;
      }
      return false;
   }
   lastListen = 0;

   game->net->messages.push_back("Started listening for player connections");
   return true;
}

//Accepts connections on the listening socket and adds the info to the list of sockets
void tcpHostAccept(int people) {
   static unsigned int lastResult = 0;
   int len = sizeof(sockets[connections].address);
   SOCKET conn = accept(sockets[-1].sock, (sockaddr*)&sockets[connections].address, &len);
   if (conn == INVALID_SOCKET) { 
      int wsaerror = WSAGetLastError();
      if (game->net->netlog == true && wsaerror != lastResult) {
         netlogWrite("Host failed to accept socket: %d\n", wsaerror);
      }
      lastResult = wsaerror;
      return;
   }
   sockets[connections].sock = conn;
   unsigned short port = 0;
   WSANtohs(sockets[connections].sock, sockets[connections].address.sin_port, &port);  //Convert network byte order to host byte order
   if (game->net->upnp == true && game->upnpRunning == true && tcpPorts[port] == false) {
      bool upnp = upnpAddPort(port, "TCP");  //Add port forwarding for this port
      if (upnp == true) { tcpPorts[port] = true; }
   }
   connections++;
   char successMsg[30];
   sprintf(successMsg, "Connections accepted: %d / %d", connections, people);
   if (connections > 1) { game->net->messages.pop_back(); }
   game->net->messages.push_back(successMsg);
   lastResult = 0;
}

//Connect to a given port on the host
bool tcpClientStartup(u_short port, const char* ip) {
   //create socket and verify
   static int createResult = 0;
   sockets[-1].sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sockets[-1].sock == INVALID_SOCKET) {
      if (game->net->netlog == true && createResult != sockets[-1].sock) { 
         netlogWrite("Client failed to create socket: %d\n", WSAGetLastError()); 
         createResult = sockets[-1].sock;
      }
      return false;
   }
   createResult = 0;

   //assign IP and Port
   sockets[-1].address.sin_family = AF_INET;
   sockets[-1].address.sin_addr.s_addr = inet_addr(ip);
   sockets[-1].address.sin_port = htons(port);

   if (game->net->upnp == true && game->upnpRunning == true && tcpPorts[port] == false) {
      bool upnp = upnpAddPort(port, "TCP");
      if (upnp) { tcpPorts[port] = true; }
      else {
         game->net->messages.push_back("Failed to map ports during startup");
         return false;
      }
   }
   return true;
}

//Connect to a given port on the host
bool tcpClientConnect() {
   static int lastConnect = 0;
   int result = connect(sockets[-1].sock, (sockaddr*)&sockets[-1].address, sizeof(sockets[-1].address));
   if (result == SOCKET_ERROR) {
      if (game->net->netlog == true && lastConnect != result) {
         netlogWrite("Client connection failed: %d\n", WSAGetLastError());
         lastConnect = result;
      }
      return false;
   }
   lastConnect = 0;

   game->net->messages.push_back("Connected to host");
   return true;
}

void tcpCloseConnections() {
   for (auto&& sock : sockets) {
      //todo check for errors on close
      closesocket(sock.second.sock);
   }
   sockets.clear();
   connections = 0;
   game->net->messages.clear();
}

void tcpCleanup() {
   tcpCloseConnections();

   for (auto&& tcpPort : tcpPorts) {
      if (tcpPort.second == true) {
         upnpDeletePort(tcpPort.first, "TCP");
         tcpPort.second = false;
      }
   }
}

void tcpReset() {
   tcpCleanup();
   winsockCleanup();
   if (game->net->ggpo != nullptr) { ggpoClose(game->net->ggpo); }
   game->winsockRunning = winsockStart();
}

void tcpServerLoop(u_short port, int people, ServerStatus &status, bool& running) {
   running = true;
   while (running == true) {
      bool done = true;
      switch (status) {
      case server_none: 
         running = false;
         break;
      case server_started:
         if (tcpHostListen(port) == true) { 
            status = server_listening; 
            myInfo = sockRandomID(game->user.name);
            memcpy(&game->net->hostSetup[0].id, myInfo.data(), sizeof(Byte) * 32);
         }
         else {
            status = server_none;
            running = false;
            tcpCleanup();
            game->net->messages.push_back("Failed to start listening... see dsNetLog.txt");
         }
         connections = 0;
         break;
      case server_listening:
         if (connections == people) {
            status = server_receive;
         }
         else if (connections < people) { tcpHostAccept(people); }
         break;
      case server_receive:
         for (int i = 0; i < connections; i++) {
            if (sockets[i].status != sock_received) {
               if (recMsg(sockets[i].sock, sockets[i].recBuff, BUFFERLEN) == false) { done = false; }
               else {
                  sockets[i].status = sock_received;
                  strcpy(sockets[i].name, &sockets[i].recBuff[32 + sizeof(int)]);  //The leftover is the name
                  //game->net->messages.push_back("Received Player Info");
               }
            }
         }
         if (done == true) { 
            status = server_waiting; 
            game->net->messages.push_back("All players connected");
         }
         break;
      case server_waiting:
         std::this_thread::sleep_for(std::chrono::milliseconds(10)); //Sleep instead of going really fast
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
            game->net->messages.push_back("Waiting for player ready signals");
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
         if (done == true) { 
            status = server_done; 
            running = false;
         }
         break;
      }
   }
}

void tcpClientLoop(u_short port, const char* ip, ClientStatus &status, const char* name, bool& running) {
   running = true;
   while (running == true) {
      switch (status) {
      case client_none:
         running = false;
         break;
      case client_started:
         if (tcpClientStartup(port, ip) == true) { 
            status = client_connecting; 
            myInfo = sockRandomID(name);
            game->net->messages.push_back("Trying to contact host");
         }
         else {
            status = client_none;
            running = false;
            tcpCleanup();
            game->net->messages.push_back("Failed to start sockets... see dsNetLog.txt");
         }
         break;
      case client_connecting:
         if (tcpClientConnect() == true) { 
            status = client_connected; 
            game->net->messages.push_back("Waiting for host to setup the game");
         }
         break;
      case client_connected:
         if (sendMsg(sockets[-1].sock, (char*)myInfo.data(), myInfo.size() ) == true) { status = client_sent; }  
         break;
      case client_sent:
         if (recMsg(sockets[-1].sock, sockets[-1].recBuff, BUFFERLEN) == true) { status = client_received; }
         break;
      case client_received:
         readGameData();
         for (int i = 0; i < game->net->participants; i++) {
            if (memcmp(&game->net->hostSetup[i].id[0], &myInfo[0], 32 ) == 0) {  
               game->net->hostSetup[i].me = true;
            }
            else { game->net->hostSetup[i].me = false; }
         }
         game->net->messages.push_back("Game data received and loaded");
         game->net->messages.push_back("Hit Start when ready");
         status = client_waiting;
         break;
      case client_waiting:
         std::this_thread::sleep_for(std::chrono::milliseconds(10)); //Sleep instead of going really fast
         break;
      case client_loaded:
         if (sendMsg(sockets[-1].sock, "R", 1) == true) { //We are ready to play
            status = client_done; 
            running = false;
         }  
         break;
      case client_done:
         running = false;
         break;
      }
   }
}

SocketInfo getSocket(int index) {
   return sockets[index];
}

static void readGameData() {
   unsigned char* gData = (unsigned char*)sockets[-1].recBuff;
   gameLoad(game, gData);
   deserializeGameSetup(game, gData);
}

void _connectionInfo() {

   if (ImGui::CollapsingHeader("Socket Info")) {
      for (auto&& sock : sockets) {
         ImGui::Text("Socket Name: %s", sock.second.name);
         ImGui::Text("Socket Address: %s", inet_ntoa(sock.second.address.sin_addr));
         ImGui::Text("Socket Port: %d", ntohs(sock.second.address.sin_port));
         ImGui::Text("Socket Port noRev: %d", sock.second.address.sin_port);
      }
   }

   if (ImGui::CollapsingHeader("UPNP Ports")) {
      for (auto&& tcpPort : tcpPorts) {
         if (tcpPort.second == true) {
            ImGui::Text("UPNP Port: %d", tcpPort.first);
         }
      }
   }
   if (ImGui::CollapsingHeader("GGPO")) {
      if (game->net->ggpo != nullptr) {
         ImGui::Text("GGPO is Running");
         for (int i = 0; i < game->net->participants; i++) {
            ImGui::Text("Participant %d", i + 1);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ggpoShowStatus(game, i));
         }
      }
   }
}

//Initialize the random number generator
std::vector <Byte> sockRandomID(const char* name) {
   std::vector <Byte> stream;
   std::default_random_engine gen( time(0) );
   std::uniform_int_distribution <int> dist(0, 255);  //Generate a random unsigned char

   for (int i = 0; i < 32; i++) {
      int val = dist(gen);
      stream.push_back(val);
   }
   //Write the board level
   int oldSize = stream.size();
   int newSize = oldSize + sizeof(int); 
   stream.resize(newSize);
   auto writeLocation = stream.data() + oldSize;
   memcpy(writeLocation, &game->user.level, sizeof(int));

   //Write the name to the end of the stream
   int len = strlen(name);
   oldSize = stream.size();
   newSize = oldSize + sizeof(Byte) * (len + 1);  //Add one for the \0 char to end string
   stream.resize(newSize);
   writeLocation = stream.data() + oldSize;
   memcpy(writeLocation, name, sizeof(Byte) * (len + 1));

   return stream;
}