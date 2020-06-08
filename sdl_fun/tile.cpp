#include <stdlib.h>
#include "tile.h"


void tileLoadTexture(Tile* tile, const char* path) {
   //hard code this for now
   switch (tile->type) {
   case tile_empty:
      break;
   case tile_circle:
      tile->texture = TextureManager::LoadTexture("assets/circle.png");
      break;
   case tile_diamond:
      tile->texture = TextureManager::LoadTexture("assets/diamond.png");
      break;
   case tile_utriangle:
      tile->texture = TextureManager::LoadTexture("assets/utriangle.png");
      break;
   case tile_dtriangle:
      tile->texture = TextureManager::LoadTexture("assets/dtriangle.png");
      break;
   case tile_star:
      tile->texture = TextureManager::LoadTexture("assets/star.png");
      break;
   case tile_heart:
      tile->texture = TextureManager::LoadTexture("assets/heart.png");
      break;
   case tile_silver:
      tile->texture = TextureManager::LoadTexture("assets/grass.png");
      break;
   }
}

void tileInit(Tile* tile, int row, int col) {
   tile->type = (TileEnum)(rand() % 6 + 1);
   tile->xpos = col * 64;
   tile->ypos = row * 64;
   tileLoadTexture(tile, "assets/dirt.png");

   tile->srcRect.h = 32;
   tile->srcRect.w = 32;

   tile->srcRect.x = 0;
   tile->srcRect.y = 0;

   tile->destRect.x = tile->xpos;
   tile->destRect.y = tile->ypos;

   tile->destRect.w = tile->srcRect.w * 2;
   tile->destRect.h = tile->srcRect.h * 2;
}

void tileInitWithType(Tile* tile, int row, int col, TileEnum type) {
   tile->type = type;
   tile->xpos = col * 64;
   tile->ypos = row * 64;
   tileLoadTexture(tile, "assets/dirt.png");

   tile->srcRect.h = 32;
   tile->srcRect.w = 32;

   tile->srcRect.x = 0;
   tile->srcRect.y = 0;

   tile->destRect.x = tile->xpos;
   tile->destRect.y = tile->ypos;

   tile->destRect.w = tile->srcRect.w * 2;
   tile->destRect.h = tile->srcRect.h * 2;
}

void tileSetXPosition(Tile* tile, int x) { tile->xpos = x; }
void tileSetYPosition(Tile* tile, int y) { tile->ypos = y; }

int tileGetXPosition(Tile* tile) { return tile->xpos; }
int tileGetYPosition(Tile* tile) { return tile->ypos; }