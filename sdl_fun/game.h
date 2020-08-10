#pragma once

#include "mymath.h"
#include "myvector.h"

#include <stdint.h>

typedef struct Board Board;
typedef struct Resources Resources;

struct GameWindow;

struct UserInput {
   //Holding a button down
   bool leftHeld = false;
   bool rightHeld = false;
   bool upHeld = false;
   bool downHeld = false;

   //Presses
   bool pausePressed = false;
   bool swapPressed = false;

   bool powerPressed = false;

   bool leftPressed = false;
   bool rightPressed = false;
   bool upPressed = false;
   bool downPressed = false;

};

//todo this isn't used right now
enum GameMode {
   versus,
   team
};

//@@Start Serialize
struct Game {

   GameWindow* sdl = nullptr;
   UserInput p1Input;

   float windowWidth;
   float windowHeight;

   //Board* board = nullptr;
   Vector<Board*>* boards = nullptr;
   Resources* resources = nullptr;

   int bHeight = 12;
   int bWidth = 6;

   int tWidth = 64;
   int tHeight = 64;

   bool isRunning = false;  //used in main loop

   int players = 1;
   bool playing = false;

   bool paused = false;
   int pauseTimer = 0;
   int pauseLength = 0;

   int timer = 0;
   int timeDelta = 0;

   uint64_t seed = 0;  //used for random number generation
};
//@@End Serialize

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

int gameLoadState(Game* game, const char* path);
FILE* gameSaveState(Game* game);