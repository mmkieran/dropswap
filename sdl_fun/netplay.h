#pragma once

#include "ggpo/ggponet.h"

#include "game.h"


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

struct NetPlay {
   GGPOSession* ggpo = nullptr;
   Game* game = nullptr;
   GGPOPlayer players[GAME_PLAYERS];
   PlayerConnectionInfo connections[GAME_PLAYERS];
   GGPOPlayerHandle localPlayer = 0;
};

void ggpoInitPlayer(int playerCount, unsigned short localport, int remoteport);

void ggpoInitSpectator();

void gameAdvanceFrame(Game* game);
void gameRunFrame();