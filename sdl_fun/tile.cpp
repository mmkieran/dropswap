#include <stdlib.h>
#include "tile.h"
#include "texture_manager.h"


void tileLoadTexture(Board* board, Tile* tile) {
   //hard code this for now
   switch (tile->type) {
   case tile_empty:
      break;
   case tile_circle:
      tile->texture = board->game->textures[0];
      break;
   case tile_diamond:
      tile->texture = board->game->textures[1];
      break;
   case tile_utriangle:
      tile->texture = board->game->textures[2];
      break;
   case tile_dtriangle:
      tile->texture = board->game->textures[3];
      break;
   case tile_star:
      tile->texture = board->game->textures[4];
      break;
   case tile_heart:
      tile->texture = board->game->textures[5];
      break;
   case tile_silver:
      tile->texture = board->game->textures[6];
      break;

   //case tile_empty:
   //   break;
   //case tile_circle:
   //   tile->texture = board->game->textures[tile_circle];
   //   break;
   //case tile_diamond:
   //   tile->texture = board->game->textures[tile_diamond];
   //   break;
   //case tile_utriangle:
   //   tile->texture = board->game->textures[tile_utriangle];
   //   break;
   //case tile_dtriangle:
   //   tile->texture = board->game->textures[tile_dtriangle];
   //   break;
   //case tile_star:
   //   tile->texture = board->game->textures[tile_star];
   //   break;
   //case tile_heart:
   //   tile->texture = board->game->textures[tile_heart];
   //   break;
   //case tile_silver:
   //   tile->texture = board->game->textures[tile_silver];
   //   break;
   }
}

//void tileInit(Board* board, Tile* tile, int row, int col) {
//   tile->type = (TileEnum)(rand() % 6 + 1);
//   tile->xpos = col * board->tileWidth;
//   tile->ypos = row * board->tileHeight;
//   tileLoadTexture(board, tile);
//
//   tile->srcRect.h = 32; //This is the size of the pixel art
//   tile->srcRect.w = 32;
//
//   tile->srcRect.x = 0;
//   tile->srcRect.y = 0;
//
//   tile->destRect.x = tile->xpos;
//   tile->destRect.y = tile->ypos;
//
//   tile->destRect.w = board->tileWidth;
//   tile->destRect.h = board->tileHeight;
//
//   tile->falling = false;
//}

void tileInitWithType(Board* board, Tile* tile, int row, int col, TileEnum type) {
   tile->type = type;
   tile->xpos = col * board->tileWidth;
   tile->ypos = row * board->tileHeight;
   tileLoadTexture(board, tile);

   tile->srcRect.h = 32; //This is the size of the pixel art
   tile->srcRect.w = 32;

   tile->srcRect.x = 0;
   tile->srcRect.y = 0;

   tile->destRect.x = tile->xpos;
   tile->destRect.y = tile->ypos;

   tile->destRect.w = board->tileWidth;
   tile->destRect.h = board->tileHeight;

   tile->clearTime = 0;
   tile->falling = false;
}

void tileUpdate(Board* board, Tile* tile) {

   tile->srcRect.h = 32;
   tile->srcRect.w = 32;

   tile->srcRect.x = 0;
   tile->srcRect.y = 0;

   tile->destRect.x = tile->xpos;
   tile->destRect.y = tile->ypos;

   tile->destRect.w = board->tileWidth;
   tile->destRect.h = board->tileHeight;
}

void tileSetXPosition(Tile* tile, int x) { tile->xpos = x; }
void tileSetYPosition(Tile* tile, int y) { tile->ypos = y; }

int tileGetXPosition(Tile* tile) { return tile->xpos; }
int tileGetYPosition(Tile* tile) { return tile->ypos; }