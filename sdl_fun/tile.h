#pragma once

#include <SDL.h>
#include "board.h"

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

   int xpos;
   int ypos;

   Mesh* mesh;

   bool falling;
   int clearTime;
   bool chain;

};

void tileSetTexture(Board* board, Tile* tile);
void tileInit(Board* board, Tile* tile, int row, int col, TileEnum type);

void tileUpdate(Board* board, Tile* tile);
void tileDraw(Board* board, Tile* tile);