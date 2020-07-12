#pragma once

#include <SDL.h>

#include "board.h"
#include "garbage.h"

enum TileEnum {
   tile_empty = 0,
   tile_circle,
   tile_diamond,
   tile_utriangle,
   tile_dtriangle,
   tile_star,
   tile_heart,
   tile_silver,
   tile_garbage,
   tile_cleared
};

struct Tile {

   TileEnum type;

   float xpos;
   float ypos;

   Mesh* mesh;

   bool falling;
   int clearTime;
   bool chain;

   Garbage* garbage;

};

void tileSetTexture(Board* board, Tile* tile);
void tileInit(Board* board, Tile* tile, int row, int col, TileEnum type, bool firstTime = false);

void tileUpdate(Board* board, Tile* tile);
void tileDraw(Board* board, Tile* tile);