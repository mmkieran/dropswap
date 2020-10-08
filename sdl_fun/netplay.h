#pragma once

#include "ggpo/ggponet.h"

#include "game.h"
#include "serialize.h"
#include "game_ui.h"

#include <string>

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
   int player = 0;
};

struct NetPlay {
   GGPOSession* ggpo = nullptr;
   GGPOPlayer players[GAME_MAX_PLAYERS];
   PlayerConnectionInfo connections[GAME_MAX_PLAYERS];
   GGPOPlayerHandle localPlayer = -1;
   bool host = false;
   int hostConnNum = -1;
   int myConnNum = -1;
   int frameDelay[3] = { 2, 1, 10 };
   int disconnectTime[3] = { 10000, 0, 30000 };
   std::vector <std::string> messages;
   bool useUPNP = true;
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