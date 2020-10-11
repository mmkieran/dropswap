#pragma once

/*Kieran McDonald 08/23/2020
* This game is for educational purposes only
* It's completely for fun!
*/

#include "mymath.h"
#include "game_inputs.h"
#include "game_ui.h"

#include <stdint.h>
#include <vector>
#include <map>

#define GAME_MAX_PLAYERS    4

struct GameWindow;

typedef struct Board Board;
typedef struct Resources Resources;
typedef struct NetPlay NetPlay;
typedef unsigned char Byte;
typedef struct FBO FBO;
typedef struct ImFont ImFont;

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

   uint64_t gameStart = 0;
   uint64_t timeFreq = 0;
   double fps = 0;
   const int delay = 1000000 / 60;  //microseconds

   uint64_t getTime() {
      uint64_t current = sdlGetCounter();
      uint64_t out = ((current - gameStart) * 1000000) / timeFreq;
      return out;
   }
};

struct GameTimings {
   //Value, min and max
   int gracePeriod[3] = { 1000, 0, 5000 };     //Bonus pause time when your board reaches the top before you die
   int fallDelay[3] = { 100, 0, 1000 };        //The pause before a tile falls after swapping
   int removeClear[3] = { 1800, 500, 5000 };  //Time it takes to change a cleared tile to empty
   int enterSilver[3] = { 30000, 0, 120000 };  //Time before silvers start appearing
   int countIn[3] = { 2000, 0, 5000 };         //Time before the board starts moving and you can swap on startup
   int landPause[3] = { 1000, 0, 5000 };       //Pause board movement when garbage lands
   int deployTime[3] = { 3000, 0, 5000 };
};

//@@Start Serialize
struct Game {

   GameWindow* sdl = nullptr;
   int vsync;
   float windowWidth;
   float windowHeight;

   std::vector <FBO*> fbos;

   NetPlay* net;

   UserInput p1Input;
   UserInput inputs[GAME_MAX_PLAYERS];

   std::vector <Board*> boards;
   Resources* resources = nullptr;

   std::map <SoundEffect, bool> soundToggles;
   int sounds = 0;  //Sound on or off

   std::map <int, ImFont*> fonts;

   GameTimings timings;

   bool ai = false;  //debug
   int aiDelay[3] = { 4, 1, 10 };   //The AI takes an action every x frames

   int bHeight = 12;
   int bWidth = 6;
   int tWidth = 64;
   int tHeight = 64;

   bool isRunning = false;  //used in main loop

   int players = 1;
   bool playing = false;

   bool paused = false;

   bool debug = false;
   bool syncTest = false;  //For GGPO Sync test
   int frameCount = 0;
   std::vector <unsigned char> save;  //todo debug find a better place for this later

   int timer = 0;
   KeepTime kt;

   uint64_t seed = 0;  //used for random number generation
};
//@@End Serialize

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
bool gameRunning(Game* game);

bool gameCheckBust(Game* game);
void gameCheckPause(Game* game, UserInput input);

void gameHandleEvents(Game* game);
void gameGiveIdleToGGPO(Game* game, int time);

void gameRunFrame();
void gameSinglePlayer(Game* game);
void gameUpdate(Game* game);
void gameRender(Game* game);

void gameDestroy(Game* game);

void imguiRender(Game* game);

void gameStartMatch(Game* game);
void gameEndMatch(Game* game);

void gameDelayFrame(Game* game, uint64_t end, uint64_t start);

void gameAI(Game* game, int player);

void gameSwapWindow(Game* game);
void sdlSetVsync(Game* game, bool toggle);