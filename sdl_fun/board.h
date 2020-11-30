#pragma once

#include "game.h"
#include "garbage.h"
#include "render.h"
#include "tile.h"
#include "cursor.h"
#include "netplay.h"

#include <random>

#define SWAPTIME 80  //Visual delay when swapping tiles

struct Tile;
struct Cursor;

enum BoardPauseType {
   pause_combo = 0,
   pause_chain,
   pause_clear,
   pause_crashland,
   pause_garbageclear,
   pause_danger
};

struct BoardStats {
   std::map <int, int> chainCounts;
   std::map <int, int> comboCounts;

   int apm = 0;  //frames with action
   int lastChain = 0;
   int lastCombo = 0;
   int clears = 0;
   int dangeresque = 0;  //frames in danger
   int garbageCrushed = 0;
};

//The top left of the board is (0, 0) for rendering and for array indices
//@@Start Serialize
struct Board {
   int index = 0;                                        //The index of this board in game->boards
   int startH = 12;                                      //The visual beginning of the top of the board (doesn't include garbage buffer above)
   int endH;                                             //The visual end of the board (not including the buffer row)
   int wBuffer;                                          //Create some extra board to store falling garbage and upcoming rows
   int w = 6;                                            //How many tiles wide is the board
   int h = 12;                                           //How many tiles high is the board
   int tileWidth;                                        //How wide in pixels is a tile
   int tileHeight;                                       //How tall in pixels is a tile
   float offset = 0;                                     //How far has the board moved up (resets to 0 after a full tile height)

   Tile* tiles = nullptr;                                //An array of tiles as a continuous chunk of memory
   std::vector <Cursor*> cursors;                        //The list of cursors on the board
   Game* game = nullptr;                                 //So you can access the game struct

   Mesh* mesh = nullptr;                                 //Used to draw all the textures on the board
     
   std::map <VisualEffect, VisualEvent> visualEvents;    //What visual effects are taking place on the board (global)
   std::vector <Animation*> sprites;                     //A list of animations currently occuring on the board
   BoardStats boardStats;                                //Tracks the statistics for each board

   float level = 5;                                      //Player handicap (modified board speed and fall rate...increases with clears and caps at 10)
   float fallSpeed = 10;                                 //This is the base fall rate (ranges from 10-13)
   float moveSpeed = 0.06;                               //This ist he base move speed (ranges from 0.06 - 1.6)
   bool waitForClear = false;                            //Are we waiting for a tile to fall to move the board
   bool paused = false;                                  //Is the board paused
   int pauseLength = 0;                                  //The time (milliseconds) that the board is paused for
   double score = 0;                                     //todo currently not used
   bool danger = false;                                  //Are there tiles at the top of the board
   bool bust = false;                                    //Are there tiles at the top and the board is not paused
   int chain = 1;                                        //Is a chain occuring? 1 means no
   std::vector <int> enemies;                            //Index used to access a board in game->boards
   std::vector <int> allies;                             //Index used to access a board in game->boards 
   int target = -1;                                      //The index in game->boards that we are currently dumping garbage on
   int uniqueID = 0;                                     //Used to generate a unique ID for non-empty tiles
   std::map <int, Tile*> tileLookup;                     //A list of tiles by ID

   int team = 1;                                         //Teams are 0 and 1

   GarbagePile* pile = nullptr;                          //List of all garbage (deployed and incoming)

   uint64_t randomCalls = 0;                             //How many times have used the random number generator, used to save/reload the state of the generator
   std::default_random_engine generator;                 //The actual random number algorithm/engine
   std::uniform_int_distribution<int> distribution;      //The distribution for the random number generator (uniform)

};
//@@End Serialize

Board* boardCreate(Game* game, int team, int tWidth, int tHeight);
Board* boardDestroy(Board* board);

int boardFillTiles(Board* board);
void boardPauseTime(Board* board, BoardPauseType type, int size = 0);

Tile* boardGetTile(Board* board, int row, int col);

void boardUpdate(Board* board);
void boardRender(Game* game, Board* board);
void boardEnableVisual(Board* board, VisualEffect effect, int duration, double x = 0, double y = 0);
void boardRemoveVisuals(Board* board);

void boardMoveUp(Board* board, float height);
void _swapTiles(Tile* tile1, Tile* tile2);
void boardSwap(Board* board, Cursor* cursor);

void boardFall(Board* board, float velocity);
void boardCheckClear(Board* board, std::vector <Tile*> tileList, bool fallCombo);
void boardRemoveClears(Board* board);
void boardAssignSlot(Board* board, bool buffer);

void makeItRain(Board* board);
void boardClear(Board* board);

int tileGetRow(Board* board, Tile* tile);
int tileGetCol(Board* board, Tile* tile);

int boardRandomTile(Board* board);
void boardStartRandom(Board* board);
void boardLoadRandom(Board* board);

void boardAI(Game* game);

void boardDebug(Board* board, bool* p_open);