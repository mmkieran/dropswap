#pragma once

#include "mymath.h"

typedef struct Board Board;
typedef struct Mesh Mesh;
typedef struct Resources Resources;

struct GameWindow;

struct Game {

   GameWindow* sdl;

   float windowWidth;
   float windowHeight;

   Board* board;
   Resources* resources;

   int bHeight = 12;
   int bWidth = 6;

   int tWidth = 64;
   int tHeight = 64;

   bool isRunning = false;

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