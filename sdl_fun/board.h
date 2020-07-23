#pragma once

#include "game.h"
#include "garbage.h"

#include <random>
#include <map>

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

   Vec2 origin = { 0, 0 };  //the frame of the board starts here
   Mesh* frame;

   Tile* tiles;
   Cursor* cursor;
   Game* game;

   float level = 1;
   float speed = 2;
   bool paused = false;
   int pauseLength = 0;
   double score = 0;
   bool bust = false;
   int combo = 1;

   std::map <int, Garbage*> garbage;

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
void makeItRain(Board* board);
void boardClear(Board* board);

int tileGetRow(Board* board, Tile* tile);
int tileGetCol(Board* board, Tile* tile);

