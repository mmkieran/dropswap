#pragma once

/*Kieran McDonald 08/23/2020
* This game is for educational purposes only
* It's completely for fun!
*/

#include "mymath.h"
#include "game_inputs.h"

#include <stdint.h>
#include <vector>
#include <map>

#define GAME_PLAYERS 2
#define GAME_MAX_PLAYERS 20
#define GAME_FRAME_DELAY 2

typedef struct Board Board;
typedef struct Resources Resources;
typedef struct NetPlay NetPlay;
typedef unsigned char Byte;
typedef struct FBO FBO;

struct GameWindow;

//todo this isn't used right now
enum GameMode {
   versus,
   team
};

enum SoundEffect {
   sound_swap = 0,
   sound_clear,
   sound_land,
   sound_crashland,
   sound_chain,
   sound_anxiety,
   sound_waltz,
   sound_COUNT
};

uint64_t sdlGetCounter();
void sdlSleep(int delay);

struct KeepTime {
   uint64_t gameStart;
   uint64_t timeFreq;

   uint64_t getTime() {
      uint64_t current = sdlGetCounter();
      uint64_t out = ((current - gameStart) * 1000000) / timeFreq;
      return out;
   }
};

//@@Start Serialize
struct Game {

   GameWindow* sdl = nullptr;
   std::vector <FBO*> fbos;

   NetPlay* net;

   UserInput p1Input;
   UserInput inputs[GAME_PLAYERS];

   float windowWidth;
   float windowHeight;

   std::vector <Board*> boards;
   Resources* resources = nullptr;

   std::map <SoundEffect, bool> soundToggles;

   int bHeight = 12;
   int bWidth = 6;

   int tWidth = 64;
   int tHeight = 64;

   int controls = 0;  //Using keyboard or controller
   int sounds = 0;  //Sound on or off

   bool isRunning = false;  //used in main loop

   int players = 1;
   bool playing = false;

   bool paused = false;
   int pauseTimer = 0;
   int pauseLength = 0;

   bool syncTest = false;  //For GGPO Sync test
   int frameCount = 0;
   int checksum = 0;

   int lastTime = 0;
   int timer = 0;
   int timeDelta = 0;
   int ggpoTime = 0;  //debug

   KeepTime kt;

   uint64_t seed = 0;  //used for random number generation
};
//@@End Serialize

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
bool gameRunning(Game* game);
void gameCheckPause(Game* game, UserInput input);

void gameHandleEvents(Game* game);
void gameGiveIdleToGGPO(Game* game, int time);

void gameRunFrame();
void gameUpdate(Game* game);
void gameRender(Game* game);

void gameDestroy(Game* game);

void imguiStartFrame(Game* game);
void imguiRender(Game* game);

void gameMenuUI(Game* game);
void boardUI(Game* game);

int gameLoad(Game* game, unsigned char*& start);
std::vector <Byte> gameSave(Game* game);

int gameLoadState(Game* game, const char* path);
FILE* gameSaveState(Game* game, const char* filename);

void gameStartMatch(Game* game);
void gameEndMatch(Game* game);
