#pragma once

#include "game.h"

typedef struct PlayerConnectionInfo PlayerConnectionInfo;

void imguiSetup(Game* game);
void imguiStartFrame(Game* game);

void mainUI(Game* game);
void boardUI(Game* game);

void gameMenuUI(Game* game);  //Old and being retired
void onePlayerOptions(Game* game);

void ggpoSessionUI(Game* game, bool* p_open);
void ggpoNetStatsUI(Game* game, PlayerConnectionInfo connect);

void gameSettingsUI(Game* game, bool* p_open);

