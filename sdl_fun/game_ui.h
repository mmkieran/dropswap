#pragma once

#include "game.h"

typedef struct Board Board;
typedef struct Resources Resources;
typedef struct NetPlay NetPlay;
typedef unsigned char Byte;
typedef struct FBO FBO;

void imguiSetup(Game* game);
void imguiStartFrame(Game* game);

void mainUI(Game* game);
void boardUI(Game* game);

void gameMenuUI(Game* game);
void ggpoSessionUI(Game* game, bool* p_open);
