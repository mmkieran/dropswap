#pragma once
#include "tile.h"
#include "cursor.h"
#include <random>


//The top left of the board is (0, 0) for rendering and for array indices
struct Board {
   int h = 12;
   int w = 6;

   int tileWidth;
   int tileHeight;

   Tile* tiles;

   double speed;
   int time;

   bool gracePeriod;

   double score;
   bool bust;

   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution;

};

Board* boardCreate(int height, int width, int tileHeight, int tileWidth);
void boardDestroy(Board* board);

Tile* boardGetTile(Board* board, int row, int col);

int boardFillTiles(Board* board);

void boardUpdate(Board* board);
void boardRender(Board* board);
void boardMoveUp(Board* board, Cursor* cursor);

void boardSwap(Board* board, Cursor* cursor);

//void boardCheckFalling(Board* board);
void boardUpdateFalling(Board* board);

