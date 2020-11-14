#pragma once

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
   tile_cleared,
   tile_COUNT
};

enum TileStatus {
   status_normal = 0,
   //status_clear,                     //I wanted to switch the tile_cleared type to this, but drawing the texture might be complicated
   status_drop,                        //Is the tile being dropped by a player
   status_stop,                        //The tile is not allowed to fall
   status_swap,                        //Used to interpolate the position of a tile after swapping
   status_disable,                     //no clearing, no falling, no swapping
   status_COUNT
};

//@@Start Serialize
struct Tile {
   TileType type;                      //Determines texture and behavior of tile (e.g. star)

   double xpos;                        //The x position of the tile on the board
   double ypos;                        //The y position of the tile on the board

   TileStatus status;                  //Temporary status that changes tile behvaior (e.g. disabled - can't fall or swap)

   Texture* texture;                   //Used to draw the tile

   bool falling;                       //Is the tile marked as falling
   uint64_t clearTime;                 //Used to determine when to remove cleared tiles
   uint64_t statusTime;                //Used to determine the length of a tile status
   bool chain = false;                 //Is the tile part of a chain on the board

   Garbage* garbage;                   //Points to the piece of garbage that a garbage tile belongs to
   int idGarbage = -1;                 //ID of the piece of garbage that a garbage tile belongs to

   VisualEffect effect = visual_none;  //Are we drawing something special for the tile
   uint64_t effectTime = 0;            //How long are we doing the visual effect
};
//@@End Serialize

void tileSetTexture(Board* board, Tile* tile);
void tileInit(Board* board, Tile* tile, int row, int col, TileType type);

void tileDraw(Board* board, Tile* tile, VisualEffect effect = visual_none, int effectTime = 0);