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
   int startH = 12;
   int endH;
   int wBuffer;  //Create some extra board to store falling garbage and upcoming rows
   int w = 6;
   int h = 12;
   int tileWidth;
   int tileHeight;
   float offset = 0;

   Tile* tiles = nullptr;
   std::vector <Cursor*> cursors;
   Game* game = nullptr;

   Mesh* mesh = nullptr;

   std::map <VisualEffect, VisualEvent> visualEvents;
   BoardStats boardStats;

   float level = 5;
   float fallSpeed = 10;
   float moveSpeed = 0.11;
   bool paused = false;
   bool waitForClear = false;
   int pauseLength = 0;
   double score = 0;
   bool danger = false;
   bool bust = false;
   int chain = 1;
   std::vector <int> enemies;  //Index used to access a board in game->boards
   std::vector <int> allies;   //Index used to access a board in game->boards 
   int target = -1;

   int team = 1;

   GarbagePile* pile = nullptr;

   uint64_t seed = 0;
   uint64_t randomCalls = 0;
   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution;

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