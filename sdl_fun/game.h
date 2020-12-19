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

#define GAME_MAX_PLAYERS    6
#define REPLAY_SIZE 5 * 60 * 60

struct GameWindow;

typedef struct Board Board;
typedef struct Cursor Cursor;
typedef struct Resources Resources;
typedef struct NetPlay NetPlay;
typedef unsigned char Byte;
typedef struct FBO FBO;
typedef struct Mesh Mesh;
typedef struct ImFont ImFont;
typedef struct Sprite Sprite;

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
   uint64_t gameStart = 0;                //The number of sdl ticks since the program started (0)
   uint64_t timeFreq = 0;                 //used to convert the sdl ticks to seconds
   double fps = 0;                        //Desired Frames Per Second in the game
   const int delay = 1000000 / 60;        //microseconds

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

struct User {
   char name[30];
   int level = 5;                //Selected board level
   int number = 1;               //1 based player number (GGPO handle in multi)
   int wins = 0;
   UserInput input;              //Holds the local inputs for the user
};

//Information about the player
struct Player {
   char name[30];
   int level = 5;                //Selected board level
   int number = 1;               //1 based player number (GGPO handle in multi)
   int team = 0;                 //0 is One and 1 is Two
   Board* board = nullptr;
   Cursor* cursor = nullptr;
   bool dead = false;
};

struct ReplayInput {
   UserInput input[4];              
};

enum GameMode {
   multi_solo = 0,                  //Individual boards
   multi_shared,                    //Shared board
   single_player,                   //todo one of many?
   game_mode_COUNT,                 //In case we need to iterate through them
};

struct GameSettings {
   int bHeight = 12;                               //The height of the (visible) board
   int bWidth = 6;                                 //The width of the (visible) board
   int tWidth = 32;                                //The width in pixels of tile
   int tHeight = 32;                               //The height in pixels of a tile

   GameMode mode = single_player;                  //The currently game mode setting, multi_solo is separate boards for each player

   std::vector <unsigned char> save;               //todo this is broken and doesn't belong here... This holds the state saves
   std::vector <ReplayInput> repInputs;            //Used to store the inputs for a replay
   int replaySpeed = 1;                            //The speed of the replay (frames per loop)
   bool replaying = false;                         //Are we viewing a replay?
};

//@@Start Serialize
struct Game {
   bool winsockRunning = false;                    //Is Windows Sockets running... required for GGPO and game transfer
   bool upnpRunning = false;                       //Did we find a valid UPNP device?
   GameWindow* sdl = nullptr;                      //SDL Window pointer
   float windowWidth;                              //SDL Window width
   float windowHeight;                             //SDL Window height
   int vsync = -1;                                 //Instead of frame delay use vsync to match monitor refresh rate (-1 is no)
   std::vector <FBO*> fbos;                        //FBOs are used to draw the board to a texture
   Mesh* mesh = nullptr;                           //Used to draw textures in render             
   NetPlay* net;                                   //Used for multiplayer    
   std::map <int, ImFont*> fonts;                  //ImGui fonts stored by their size in the map
   std::vector <Board*> boards;                    //Boards for all players
   std::vector <Sprite> drawList;                  //Used for rendering textures outside the boards or when not playing a game
   Resources* resources = nullptr;                 //Resources for the game like textures, sounds, and files
   std::map <SoundEffect, bool> soundToggles;      //Map of all the sounds and whether they are playing
   int sounds = 0;                                 //Game sound toggle (0 is disabled)
   bool isRunning = false;                         //Used in main application loop
   KeepTime kt;                                    //Structure to keep SDL ticks and calculate elapsed time
   int frameCount = 0;                             //How many frames have we done
   bool debug = false;                             //Toggle to show debug tools and options
   uint64_t seed = 0;                              //Holds the random number seed given to each board

   User user;                                      //Information about the user such as name and player number
   GameTimings timings;                            //Configurable times for things like clears
   GameSettings settings;                          //Configuration for the board such as tile number and size

   bool ai = false;                                //Is the AI playing for this user
   int aiDelay[3] = { 4, 1, 10 };                  //The AI takes an action every x frames

   std::map <int, Player> pList;                   //The list of players (1 based index)
   std::map < int, std::vector <Board*> > teams;   //todo this isn't used currently
   int players = 1;                                //How many players are in the game (not spectators)
   bool playing = false;                           //Is a game in progress
   int busted = -1;                                //Is the game over, but not closed
   int paused = 0;                                 //Is the game paused
   bool waiting = false;                           //Is the game waiting for something
   int waitLength = 0;                             //How long is the game waiting
   int timer = 0;                                  //Game clock in milliseconds
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
void gameReplay(Game* game);
void gameUpdate(Game* game);
void gameRender(Game* game);

void gameDestroy(Game* game);

void imguiRender(Game* game);
void gameUpdateSprites(Game* game);
void gameDrawSprites(Game* game);

void gameStartMatch(Game* game);
void gameEndMatch(Game* game);

void gameDelayFrame(Game* game, uint64_t end, uint64_t start);

void gameAI(Game* game);

void gameSwapWindow(Game* game);
void sdlSetVsync(Game* game, bool toggle);

void gameCaptureReplayInputs(Game* game);