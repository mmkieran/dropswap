#pragma once

#include "ggpo/ggponet.h"

#include "game.h"

enum GameMsg {
   Ready = 0,
   Garbage,
   Dead
};

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
struct SessionInfo {
   bool host = false;
   bool me = false;
   int localPort = 7001;
   int playerType = -1;
   char ipAddress[32] = "127.0.0.1";
   //int remotePort = 7002;
};

struct NetPlay {
   GGPOSession* ggpo = nullptr;
   Game* game = nullptr;
   GGPOPlayer players[GAME_MAX_PLAYERS];
   PlayerConnectionInfo connections[GAME_MAX_PLAYERS];
   GGPOPlayerHandle localPlayer = 0;
};

void ggpoInitPlayer(int playerCount, int pNumber, unsigned short localport, int remoteport);
void ggpoInitSpectator();

void ggpoSendMessage(uint64_t msg, unsigned short code, unsigned short handle);
void ggpoReadMessage(Game* game, UserInput input, unsigned short handle);

void gameAdvanceFrame(Game* game);
void gameRunFrame();

void ggpoClose(GGPOSession* ggpo);

const char* ggpoShowStatus(Game* game);