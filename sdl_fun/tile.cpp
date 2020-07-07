#include <stdlib.h>
#include "tile.h"
#include "resources.h"
#include "render.h"

void tileSetTexture(Board* board, Tile* tile) {
   //hard code this for now
   switch (tile->type) {
   case tile_empty:
      tile->mesh->texture = nullptr;
      break;
   case tile_circle:
      tile->mesh->texture = board->game->resources->textures[0];
      break;
   case tile_diamond:
      tile->mesh->texture = board->game->resources->textures[1];
      break;
   case tile_utriangle:
      tile->mesh->texture = board->game->resources->textures[2];
      break;
   case tile_dtriangle:
      tile->mesh->texture = board->game->resources->textures[3];
      break;
   case tile_star:
      tile->mesh->texture = board->game->resources->textures[4];
      break;
   case tile_heart:
      tile->mesh->texture = board->game->resources->textures[5];
      break;
   case tile_silver:
      tile->mesh->texture = board->game->resources->textures[6];
      break;
   default:
      tile->mesh->texture = nullptr;
   }
}

void tileInit(Board* board, Tile* tile, int row, int col, TileEnum type) {
   tile->type = type;
   tile->xpos = col * board->tileWidth;
   tile->ypos = (row - board->startH) * board->tileHeight;

   tile->mesh = createMesh(board->game);

   tileSetTexture(board, tile);

   tile->clearTime = 0;
   tile->falling = false;
   tile->chain = 0;
}

void tileUpdate(Board* board, Tile* tile) {
   //todo put something here
}

void tileDraw(Board* board, Tile* tile) {
   drawMesh(board->game, tile->mesh, tile->xpos, tile->ypos, board->tileWidth, board->tileHeight);
}

