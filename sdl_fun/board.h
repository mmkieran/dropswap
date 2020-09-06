#pragma once

#include "game.h"
#include "garbage.h"
#include "render.h"

#include <random>

#define SWAPTIME 75

struct Tile;
struct Cursor;

//The top left of the board is (0, 0) for rendering and for array indices
//@@Start Serialize
struct Board {
   int startH = 12;
   int endH;
   int wBuffer;  //Create some extra board to store falling garbage and upcoming rows
   int w = 6;
   int tileWidth;
   int tileHeight;
   float offset = 0;

   Tile* tiles = nullptr;
   Cursor* cursor = nullptr;
   Game* game = nullptr;

   std::vector <VisualEvent> visualEvents;

   float level = 1;
   float fallSpeed = 2;
   float moveSpeed = 0.25;
   bool paused = false;
   int pauseLength = 0;
   double score = 0;
   bool bust = false;
   int chain = 1;

   int player = 1;

   GarbagePile* pile = nullptr;

   uint64_t seed = 0;
   uint64_t randomCalls = 0;
   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution;

};
//@@End Serialize

Board* boardCreate(Game* game);
Board* boardDestroy(Board* board);

int boardFillTiles(Board* board);

Tile* boardGetTile(Board* board, int row, int col);

void boardUpdate(Board* board, UserInput input);
void boardRender(Game* game, Board* board);

void boardMoveUp(Board* board, float height);
void boardSwap(Board* board);

void boardFall(Board* board, float velocity);
void boardCheckClear(Board* board, std::vector <Tile*> tileList, bool fallCombo);
void boardRemoveClears(Board* board);
void boardAssignSlot(Board* board, bool buffer);

std::vector <Tile> boardDebug(Board* board);
void makeItRain(Board* board);
void boardClear(Board* board);

int tileGetRow(Board* board, Tile* tile);
int tileGetCol(Board* board, Tile* tile);

int boardRandomTile(Board* board);
void boardStartRandom(Board* board);
void boardLoadRandom(Board* board);