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

   SDL_Texture* texture;   //tile image
   SDL_Rect srcRect;          //What part of the texture are we using
   SDL_Rect destRect;         //Where are we going to render it

   bool falling;
   int clearTime;

};

void tileLoadTexture(Board* board, Tile* tile);
void tileInitWithType(Board* board, Tile* tile, int row, int col, TileEnum type);

void tileUpdate(Board* board, Tile* tile);

void tileSetXPosition(Tile* tile, int x);
void tileSetYPosition(Tile* tile, int y);

int tileGetXPosition(Tile* tile);
int tileGetYPosition(Tile* tile);