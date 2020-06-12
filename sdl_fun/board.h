#pragma once

#include "game.h"

struct Tile;
class Cursor;

//The top left of the board is (0, 0) for rendering and for array indices
struct Board {
   int h = 12;
   int w = 6;

   bool updateBoard;
   bool updateFalling;

   int tileWidth;
   int tileHeight;

   Tile* tiles;
   Cursor* cursor;
   Game* game;

   double speed;
   int time;

   std::atomic_int gracePeriod;

   double score;
   bool bust;

   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution;

};

Board* boardCreate(Game* game);
void boardDestroy(Board* board);

int boardFillTiles(Board* board);

Tile* boardGetTile(Board* board, int row, int col);

void boardUpdate(Board* board);
void boardRender(Game* game, Board* board);
void boardMoveUp(Board* board, Cursor* cursor);

void boardSwap(Board* board, Cursor* cursor);

//void boardCheckFalling(Board* board);
void boardUpdateFalling(Board* board);

