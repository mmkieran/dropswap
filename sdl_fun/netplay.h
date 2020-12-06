#pragma once

#include "ggpo/ggponet.h"

#include "game.h"
#include "serialize.h"
#include "game_ui.h"

#include <string>
#include <winsock2.h>  //For windows sockets 

#define BUFFERLEN 8192

enum ConnState {
   Disconnected = 0,
   Connecting,
   Synchronizing,
   Synched,
   Running,
   Disconnecting,
};

//This is to transfer information from imgui window to the Player struct
struct SessionInfo {
   bool host = false;                           //Is this connection the host
   bool me = false;                             //Is this connection this computer
   u_short localPort = 7001;                    //GGPO (UDP) port
   int playerType = 0;                          //GGPO Player Type, 0 is player and 1 is spectator
   char ipAddress[32] = "127.0.0.1";            //GGPO IP address
   char name[30] = { 0 };                       //Player name
   unsigned char id[32];                        //Player's random (hopefully unique) id
   int pNum = 1;                                //GGPO Player Number
   int team = 0;                                //Team represents what board you control
   int level = 5;                               //Player handicap (level) for the board
   ConnState state = Disconnected;              //What is the current GGPO connection status of the player
   int dcTime = 0;                  
   int dcStart = 0;
};

struct NetPlay {
   GGPOSession* ggpo = nullptr;                           //Pointer to ggpo structure
   bool syncTest = false;                                 //For GGPO Sync test

   GGPOPlayer players[GAME_MAX_PLAYERS] = { 0 };          //Structure used to create a GGPO session
   UserInput inputs[GAME_MAX_PLAYERS] = { 0 };            //Structure is populated with synchronized inputs every frame
   SessionInfo hostSetup[GAME_MAX_PLAYERS] = { 0 };       //Used to map connection info from UI to GGPO structs
   int participants = 0;                                  //Number of players and spectators in the game
   int hostConnNum = -1;                                  //What is the connection number of the host (for connections)

   int frameDelay[3] = { 1, 1, 10 };                      //How long should we delay the local user input in the session
   int disconnectTime[3] = { 10000, 0, 30000 };           //How long do we wait before we disconnect a player (0 is forever)
   std::vector <std::string> messages;                    //For network messages to user through UI
   bool upnp = true;                                      //Do we want to use UPNP?
   bool netlog = false;                                   //Enable logging of network errors
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

void gameAdvanceFrame(Game* game);
void gameRunFrame();

void ggpoCreateSession(Game* game, SessionInfo connects[], unsigned short participants);
void ggpoClose(GGPOSession* ggpo);
const char* ggpoShowStatus(Game* game, int playerIndex);
void ggpoEndSession(Game* game);

ConnState getServerConnState();
ConnState getMyConnState();

bool winsockStart();
void winsockCleanup();

void tcpClientLoop(u_short port, const char* ip, ClientStatus& status, const char* name, bool &running);
void tcpServerLoop(u_short port, int people, ServerStatus& status, bool &running);
void tcpCloseConnections();
void tcpCleanup();
void tcpReset();
SocketInfo getSocket(int index);
std::vector <Byte> sockRandomID(const char* name);
void _connectionInfo();

void upnpStartup(Game* game);
int upnpAddPort(u_short port, const char* protocol = "UDP");
int upnpDeletePort(u_short port, const char* protocol = "UDP");
void upnpCleanup();