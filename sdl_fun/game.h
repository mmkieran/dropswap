#pragma once

#include "mymath.h"

typedef struct Board Board;
typedef struct Resources Resources;

struct GameWindow;

struct Game {

   GameWindow* sdl = nullptr;

   float windowWidth;
   float windowHeight;

   Board* board = nullptr;
   Resources* resources = nullptr;

   int bHeight = 24;
   int bWidth = 12;

   int tWidth = 32;
   int tHeight = 32;

   bool isRunning = false;  //used in main loop
   bool playing = false;

   bool paused = false;
   int pauseTimer = 0;
   int pauseLength = 0;

   int timer = 0;
   int timeDelta = 0;

};

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
bool gameRunning(Game* game);

void gameHandleEvents(Game* game);

void gameUpdate(Game* game);
void gameRender(Game* game);

void gameDestroy(Game* game);

void imguiStartFrame(Game* game);
void imguiShowDemo();

void showGameMenu(Game* game);

void debugGarbage(Game* game);
void debugCursor(Game* game);