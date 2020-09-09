#pragma once

#include "game.h"

void imguiSetup(Game* game);
void imguiStartFrame(Game* game);

void mainUI(Game* game);
void boardUI(Game* game);

void gameMenuUI(Game* game);  //Old and being retired

void ggpoSessionUI(Game* game, bool* p_open);
void ggpoNetStatsUI(Game* game, bool* p_open);

void gameSettingsUI(Game* game, bool* p_open);

