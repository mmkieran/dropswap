
#include "netplay.h"

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
   if (game->net->useUPNP) { bool upnpSuccess = upnpStartup(sessionPort); }

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

/*
After UPNP is successful...

Host
Server sends a message to each player using IP and Port (socket) - threaded?
Packet contains the saved game state plus frame delay, etc.
Server listens for response
When all responses are in, it starts ggpo session
Start game

Client
Each client sits listening for server
When they receive the game state and it loads successfully... if not do manual
They send a ready message back and start ggpo session
Start game

*/

#include <winsock2.h>  //For windows sockets 

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

int sockfd, connfd, len;

std::map <int, SOCKET> sockets;
int people = 1;
int connections = 0;

sockaddr_in server, client = { 0 };

#define BUFFERLEN 8192

char recvBuffer[BUFFERLEN];
int bufferLen = BUFFERLEN;

char* tcpClientConnect(int port, const char* ip = "127.0.0.1") {
   //create socket and verify
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd == -1) { return "Socket creation failed."; }

   //assign IP and Port
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = inet_addr(ip);
   server.sin_port = htons(port);

   //Here we need to start UPNP

   if (connect(sockfd, (sockaddr*)&server, sizeof(server)) != 0) {
      return "Socket creation failed.";
   }

   return "Connected to server socket";
}

bool tcpSendMsg(SOCKET socket, const char* buffer, int len) {
   int result = send(socket, buffer, len, 0);
   if (result != SOCKET_ERROR) { return true; }  //Failed to send
   return false;
}

bool recMsg(SOCKET socket) {
   //Do this in a while loop until all data received
   //Use a length prefix at the start of the data (size of vector) 
   int result = recv(socket, recvBuffer, bufferLen, 0);
   if (result <= 0) { return false ; } 
   if (result > 0) { return true; }  //We got something
}

void tcpClose() {
   closesocket(sockfd);
   closesocket(connfd);
   //WSACleanup();
}

//Creates a socket for listening on the host machine
char* tcpHostListen(int port) {
   //create socket and verify
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd == -1) { printf("Socket creation failed."); }

   //assign IP and Port
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = htonl(INADDR_ANY);  //htonl converts ulong to tcp/ip network byte order
   server.sin_port = htons(port);

   //bind socket
   if (bind(sockfd, (sockaddr*)&server, sizeof(server)) != 0) {
      return "socket binding failed...";
   }

   //start listening on socket
   if (listen(sockfd, 5) != 0) {
      return "Listen failed...";
   }

   return "Started Listening...";
}

//Accepts connections until we have all the players
char* tcpHostAccept() {
   if (connections < people) {  
      len = sizeof(client);
      connfd = accept(sockfd, (sockaddr*)&client, &len);
      if (connfd < 0) { return "socket accept failed..."; }
      connections++;
      sockets[connections] = connfd;
   }
}

void tcpHostSend() {
   char* buffer = "Howdy";  //Use serialize here to send game info
   bool sent = false;  //Need to keep track of send/receive status
   if (connections == people && sent == false) {
      for (int i = 0; i < connections; i++) {
         tcpSendMsg(sockets[i], buffer, sizeof(buffer));
      }
   }
}

void debugTCPConn() {
   if (!ImGui::Begin("TCP Test")) {
      ImGui::PopFont();
      ImGui::End();
      return;
   }

   static char myMessage[32] = "Howdy";
   static char myMessage2[32] = "Hello";
   static bool isServer = false;
   int bounds[3] = { 7001, 7000, 7008 };
   ImGui::SliderScalar("Port Number", ImGuiDataType_U32, &bounds[0], &bounds[1], &bounds[2]);

   static char listen[60];
   if (ImGui::Button("Listen")) {
      strcpy(listen, tcpServer(7002) );
      isServer = true;
   }
   ImGui::Text(listen);

   static char connResult[60];
   if (ImGui::Button("Connect")) {
      strcpy(listen, tcpClient(7002) );
   }
   ImGui::Text(connResult);

   static bool startUPNP = false;
   if (ImGui::Button("Start UPNP")) {
      startUPNP = upnpStartup(sessionPort);
   }
   if (startUPNP == true) { ImGui::Text("Started UPNP"); }

   static bool sendResult = false;
   if (ImGui::Button("Send")) {
      if (isServer == true) {
         sendResult = sendMsg(connfd, myMessage, 5);
      }
      else if (isServer == false) {
         sendResult = sendMsg(sockfd, myMessage2, 5);
      }
   }
   if (sendResult == true) { ImGui::Text("Sent successfully"); }

   static bool recResult = false;
   if (ImGui::Button("Receive")) {
      if (isServer == true) {
         recResult = recMsg(connfd);
      }
      if (isServer == false) {
         recResult = recMsg(sockfd);
      }
   }
   if (recResult == false) { ImGui::Text("Failed to receive anything"); }
   ImGui::Text(recvBuffer);

   if (ImGui::Button("Close")) {
      tcpClose();
   }

   ImGui::Text(inet_ntoa(client.sin_addr));
   ImGui::Text(inet_ntoa(server.sin_addr));
   ImGui::Text("%d", ntohs(client.sin_port) );
   ImGui::Text("%d", ntohs(server.sin_port) );

   ImGui::End();
}