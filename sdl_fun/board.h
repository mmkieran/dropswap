#pragma once

#include "game.h"

struct Tile;
class Cursor;

//The top left of the board is (0, 0) for rendering and for array indices
struct Board {
   int startH = 12;
   int endH;
   int w = 6;

   int wBuffer;  //Create some extra board to store falling garbage and upcoming rows

   int tileWidth;
   int tileHeight;

   Tile* tiles;
   Cursor* cursor;
   Game* game;

   double speed = 1;

   int moveTimer = 0;
   int fallTimer = 0;
   bool paused = false;
   int pauseLength = 0;

   int offset = 0;

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
void boardMoveUp(Board* board, int height);

void boardSwap(Board* board, Cursor* cursor);

void boardUpdateFalling(Board* board);
void boardClearBlocks(Board* board);

