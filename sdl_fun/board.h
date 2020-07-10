#pragma once

#include "game.h"
#include <random>
#include <vector>

struct Tile;
struct Cursor;

//The top left of the board is (0, 0) for rendering and for array indices
struct Board {
   int startH = 12;
   int endH;
   int wBuffer;  //Create some extra board to store falling garbage and upcoming rows
   int w = 6;
   int tileWidth;
   int tileHeight;
   float offset = 0;

   Mesh* frame;

   Tile* tiles;
   Cursor* cursor;
   Game* game;

   double speed = 0.5;
   int moveTimer = 0;
   int fallTimer = 0;
   bool paused = false;
   int pauseLength = 0;
   double score;
   bool bust;
   int combo;

   std::vector <Garbage*> garbage;

   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution;

};

Board* boardCreate(Game* game);
void boardDestroy(Board* board);

int boardFillTiles(Board* board);

Tile* boardGetTile(Board* board, int row, int col);

void boardRender(Game* game, Board* board);
void boardMoveUp(Board* board, float height);

void boardSwap(Board* board);

void boardUpdateFalling(Board* board, float velocity);
void boardCheckClear(Board* board, std::vector <Tile*> tileList, bool fallCombo);
void boardRemoveClears(Board* board);
void boardUpdateArray(Board* board, bool buffer);

std::vector <Tile> boardDebug(Board* board);
int yPosToRow(Board* board, float y);
int xPosToCol(Board* board, float x);

