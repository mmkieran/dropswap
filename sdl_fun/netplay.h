#pragma once

#include "ggpo/ggponet.h"

#include "game.h"

//Use this to turn on synchronization testing... predicts every frame
#define SYNC_TEST true

enum PlayerConnectState {
   Connecting = 0,
   Synchronizing,
   Running,
   Disconnected,
   Disconnecting,
};

struct PlayerConnectionInfo {
   GGPOPlayerType       type;
   GGPOPlayerHandle     handle;
   PlayerConnectState   state;
   int                  connect_progress;
   int                  disconnect_timeout;
   int                  disconnect_start;
};

//todo not sure where this belong right now
//This is to transfer information from imgui window to the Player struct
struct SessionInfo {
   bool host = false;
   bool me = false;
   int localPort = 7001;
   int playerType = 0;
   char ipAddress[32] = "127.0.0.1";
};

struct NetPlay {
   GGPOSession* ggpo = nullptr;
   GGPOPlayer players[GAME_MAX_PLAYERS];
   PlayerConnectionInfo connections[GAME_MAX_PLAYERS];
   GGPOPlayerHandle localPlayer = -1;
   bool host = false;
   int hostConnNum = -1;
   int myConnNum = -1;
};

void gameAdvanceFrame(Game* game);
void gameRunFrame();

void ggpoCreateSession(Game* game, SessionInfo connects[], unsigned short participants);
void ggpoClose(GGPOSession* ggpo);

const char* ggpoShowStatus(Game* game, int playerIndex);

int ggpoDisconnectPlayer(int player);
void ggpoEndSession(Game* game);

void ggpoSendMessage(int msg);