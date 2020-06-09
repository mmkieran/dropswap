#pragma once

#include <SDL.h>
#include "texture_manager.h"

enum TileEnum {
   tile_empty = 0,
   tile_circle,
   tile_diamond,
   tile_utriangle,
   tile_dtriangle,
   tile_star,
   tile_heart,
   tile_silver,
   tile_garbage
};

struct Tile {

   TileEnum type;

   int xpos;
   int ypos;

   SDL_Texture* texture;   //tile image
   SDL_Rect srcRect;          //What part of the texture are we using
   SDL_Rect destRect;         //Where are we going to render it

   bool falling;
};

void tileLoadTexture(Tile* tile, const char* path);
void tileInit(Tile* tile, int row, int col, int tileWidth, int tileHeight);
void tileInitWithType(Tile* tile, int row, int col, int tileWidth, int tileHeight, TileEnum type);

void tileSetXPosition(Tile* tile, int x);
void tileSetYPosition(Tile* tile, int y);

int tileGetXPosition(Tile* tile);
int tileGetYPosition(Tile* tile);