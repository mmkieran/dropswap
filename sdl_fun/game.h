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

//Used to get the current time with SDL
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

//Used to control various timings in the game
struct GameTimings {
   //Order is Value, min and max
   int gracePeriod[3] = { 1000, 0, 5000 };     //Bonus pause time when your board reaches the top before you die
   int fallDelay[3] = { 100, 0, 1000 };        //The pause before a tile falls after swapping
   int removeClear[3] = { 1800, 500, 5000 };   //Time it takes to change a cleared tile to empty
   int enterSilver[3] = { 30000, 0, 120000 };  //Time before silvers start appearing
   int countIn[3] = { 2000, 0, 5000 };         //Time before the board starts moving and you can swap on startup
   int landPause[3] = { 1000, 0, 5000 };       //Pause board movement when garbage lands
   int deployTime[3] = { 3000, 0, 5000 };      //How long does it take for garbage to be placed on the board
};

//Information about the player
struct Player {
   char name[30];
   int level = 1;
   int number = 1;
   int team = 0;
   Board* board = nullptr;
   int cursor = 0;
   bool dead = false;
   int wins = 0;
   UserInput input;
};

enum GameMode {
   multi_solo = 0,
   multi_shared,
};

struct GameSettings {
   int bHeight = 12;
   int bWidth = 6;
   int tWidth = 64;
   int tHeight = 64;

   GameMode mode = multi_solo;

   std::vector <unsigned char> save;
};

//@@Start Serialize
struct Game {
   bool winsockRunning = false;                //Is Windows Sockets running... required for GGPO and game transfer
   GameWindow* sdl = nullptr;                  //SDL Window pointer
   float windowWidth;                          //SDL Window width
   float windowHeight;                         //SDL Window height
   int vsync = -1;                             //Instead of frame delay use vsync to match monitor refresh rate (-1 is no)
   std::vector <FBO*> fbos;                    //FBOs are used to draw the board to a texture
   NetPlay* net;                               //Used for multiplayer    
   std::map <int, ImFont*> fonts;              //ImGui fonts stored by their size in the map
   std::vector <Board*> boards;                //Boards for all players
   Resources* resources = nullptr;             //Resources for the game like textures, sounds, and files
   std::map <SoundEffect, bool> soundToggles;  //Map of all the sounds and whether they are playing
   int sounds = 0;                             //Game sound toggle (0 is disabled)

   Player p;
   GameTimings timings;
   GameSettings settings;

   bool ai = false;                            
   int aiDelay[3] = { 4, 1, 10 };              //The AI takes an action every x frames

   bool isRunning = false;                     //Used in main application loop

   int players = 1;
   bool playing = false;

   bool paused = false;

   bool debug = false;                          //Toggle to show debug tools and options
   int frameCount = 0; 

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