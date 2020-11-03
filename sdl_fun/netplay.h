#pragma once

#include "ggpo/ggponet.h"

#include "game.h"
#include "serialize.h"
#include "game_ui.h"

#include <string>
#include <winsock2.h>  //For windows sockets 

#define BUFFERLEN 8192

enum PlayerConnectState {
   Disconnected = 0,
   Connecting,
   Synchronizing,
   Running,
   Disconnecting,
};

struct PlayerConnectionInfo {
   GGPOPlayerType       type;
   GGPOPlayerHandle     handle = -1;
   PlayerConnectState   state;
   int                  connect_progress = 0;
   int                  disconnect_timeout = 0;
   int                  disconnect_start = 0;
};

//This is to transfer information from imgui window to the Player struct
struct SessionInfo {
   bool host = false;
   bool me = false;
   u_short localPort = 7001;
   int playerType = 0;
   char ipAddress[32] = "127.0.0.1";
   char name[30] = { 0 };
   int pNum = 1;
   int team = 0;
};

struct NetPlay {
   GGPOSession* ggpo = nullptr;                           //Pointer to ggpo structure
   bool syncTest = false;                                 //For GGPO Sync test

   GGPOPlayer players[GAME_MAX_PLAYERS];                  //Structure used to create a GGPO session
   GGPOPlayerHandle localPlayer = -1;                     //What is the GGPO player number for this player

   PlayerConnectionInfo connections[GAME_MAX_PLAYERS];    //Information about the connection status
   int hostConnNum = -1;                                  //What is the connection number of the host (for connections)
   int myConnNum = -1;                                    //What is my connection number (for connections)
   UserInput inputs[GAME_MAX_PLAYERS];                    //Structure is populated with synchronized inputs every frame

   SessionInfo hostSetup[GAME_MAX_PLAYERS];               //Used to map connection info from UI to GGPO structs
   int frameDelay[3] = { 2, 1, 10 };                      //How long should we delay the local user input in the session
   int disconnectTime[3] = { 10000, 0, 30000 };           //How long do we wait before we disconnect a player (0 is forever)
   std::vector <std::string> messages;                    //For network messages to user through UI
   bool upnp = false;                                     //Is UPNP running... needed for port forwarding
   int timeSync = 0;                                      //Used to delay updating the game if one player is too many frames ahead
};

//Server statuses for TCP game info transfer
enum ServerStatus {
   server_none = 0,               //Not running
   server_started,                //Start socket creation
   server_listening,              //Listening for incoming connections
   server_accept,
   server_receive,
   server_send,
   server_waiting,                //Waiting for user to hit START
   server_ready,
   server_done,                   //Game setup is complete... ready for GGPO
};

//Client status for TCP game info transfer
enum ClientStatus {
   client_none = 0,               //Not running
   client_started,                //Start socket creation
   client_connecting,             //Attempting connection
   client_connected,              //Connected to the host
   client_sent,
   client_received,
   client_waiting,                //Waiting for input
   client_loaded,                 //Received game data and ready to play
   client_done,
};

//Has the socket successfully sent or received data
enum SocketStatus {
   sock_none = 0,
   sock_sent,
   sock_received,
   sock_ready,
};

//Socket structures for winsocks
struct SocketInfo {
   char name[30];                        //Player name used to identify socket
   SOCKET sock = { 0 };                  //Integer id for the socket
   sockaddr_in address = { 0 };          //Information about the socket such as address and port
   char recBuff[BUFFERLEN];              //Buffer used to receive messages
   SocketStatus status = sock_none;      //Used to check if things were went or received successfully
};

////Currently not used...
//enum CommError {
//   error_sock_create = 0,
//   //server start
//   error_sock_bind,
//   error_sock_listen,
//   error_sock_accept,
//   //client start
//   error_sock_connect,
//   //connected
//   error_sock_send,
//   error_sock_receive,
//};

int fletcher32_checksum(short* data, size_t len);
int ggpoCheckSum(Game* game);

void gameAdvanceFrame(Game* game);
void gameRunFrame();

void ggpoCreateSession(Game* game, SessionInfo connects[], unsigned short participants);
void ggpoClose(GGPOSession* ggpo);

const char* ggpoShowStatus(Game* game, int playerIndex);

int ggpoDisconnectPlayer(int player);
void ggpoEndSession(Game* game);

bool winsockStart();
void winsockCleanup();

void tcpClientLoop(u_short port, const char* ip, ClientStatus& status, const char* name, bool &running);
void tcpServerLoop(u_short port, int people, ServerStatus& status, bool &running);
void tcpCloseConnections();
void tcpCleanup();
void tcpReset();
void readGameData();
SocketInfo getSocket(int index);
void _connectionInfo();

void upnpStartup(Game* game);
int upnpAddPort(u_short port, const char* protocol = "UDP");
int upnpDeletePort(u_short port, const char* protocol = "UDP");
void upnpCleanup();