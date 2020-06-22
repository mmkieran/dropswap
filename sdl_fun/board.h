#pragma once

#include "game.h"

struct Tile;
class Cursor;

enum MoveEnum {
   move_up = 0,
   move_down,
   move_left,
   move_right,
};


//The top left of the board is (0, 0) for rendering and for array indices
struct Board {
   int startH = 12;
   int endH;
   int wBuffer;  //Create some extra board to store falling garbage and upcoming rows
   int w = 6;
   int tileWidth;
   int tileHeight;
   int offset = 0;

   Tile* tiles;
   Cursor* cursor;
   Game* game;

   double speed = 1;
   int moveTimer = 0;
   int fallTimer = 0;
   bool paused = false;
   int pauseLength = 0;
   double score;
   bool bust;
   int combo;

   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution;

};

Board* boardCreate(Game* game);
void boardDestroy(Board* board);

int boardFillTiles(Board* board);

Tile* boardGetTile(Board* board, int row, int col);

void boardRender(Game* game, Board* board);
void boardMoveUp(Board* board, int height);

void boardSwap(Board* board, Cursor* cursor);

void boardUpdateFalling(Board* board, int velocity);
//void boardUpdateFalling(Board* board);
void boardCheckClear(Board* board, std::vector <Tile*> tileList, bool fallCombo);
void boardRemoveClears(Board* board);
//void boardResetChain(Board* board);
void boardUpdateArray(Board* board, bool buffer);

std::vector <Tile> boardDebug(Board* board);

