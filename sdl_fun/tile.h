#pragma once

#include <SDL.h>

#include "board.h"
#include "garbage.h"

enum TileType {
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

enum TileStatus {
   status_normal = 0,
   status_clear,  //cleared
   status_fall,
   status_stop,  //no falling
   status_disable //no clearing, no falling, no swapping
};

struct Tile {

   TileType type;
   TileStatus status;

   float xpos;
   float ypos;

   Mesh* mesh;

   bool falling;
   uint64_t clearTime;
   uint64_t statusTime;
   bool chain;

   Garbage* garbage;
   int idGarbage;
};

void tileSetTexture(Board* board, Tile* tile);
void tileInit(Board* board, Tile* tile, int row, int col, TileType type, bool firstTime = false);

void tileUpdate(Board* board, Tile* tile);
void tileDraw(Board* board, Tile* tile);