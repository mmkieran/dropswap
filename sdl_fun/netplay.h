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

//todo not sure where this belong right now
//This is to transfer information from imgui window to the Player struct
struct SessionInfo {
   bool host = false;
   bool me = false;
   int localPort = 7001;
   int playerType = 0;
   char ipAddress[32] = "127.0.0.1";
   char name[20] = { 0 };
   int pNum = 1;
   int team = 0;
};

struct NetPlay {
   GGPOSession* ggpo = nullptr;
   GGPOPlayer players[GAME_MAX_PLAYERS];
   PlayerConnectionInfo connections[GAME_MAX_PLAYERS];
   SessionInfo hostSetup[GAME_MAX_PLAYERS];
   GGPOPlayerHandle localPlayer = -1;
   int hostConnNum = -1;
   int myConnNum = -1;
   int frameDelay[3] = { 2, 1, 10 };
   int disconnectTime[3] = { 10000, 0, 30000 };
   std::vector <std::string> messages;
   bool upnp = false;
   int timeSync = 0;
};

enum ServerStatus {
   server_none = 0,
   server_started,
   server_listening,
   server_accept,
   server_receive,
   server_send,
   server_waiting,
   server_ready,
   server_done,
};

enum ClientStatus {
   client_none = 0,
   client_started,
   client_connected,
   client_sent,
   client_received,
   client_waiting,
   client_loaded,
   client_done,
};

enum SocketStatus {
   sock_none = 0,
   sock_sent,
   sock_received,
   sock_ready,
};

struct SocketInfo {
   char name[20];
   SOCKET sock = { 0 };
   sockaddr_in address = { 0 };
   char recBuff[BUFFERLEN];
   SocketStatus status = sock_none;
};

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
void tcpClientLoop(int port, const char* ip, ClientStatus& status, const char* name);
void tcpServerLoop(int port, int people, ServerStatus &status);
void tcpCleanup(int port);
void readGameData();
SocketInfo getSocket(int index);

void upnpStartup(Game* game);
int upnpAddPort(int port);
int upnpDeletePort(int port);