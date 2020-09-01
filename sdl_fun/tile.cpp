#include <stdlib.h>

#include "tile.h"
#include "resources.h"
#include "render.h"
#include "garbage.h"

void tileSetTexture(Board* board, Tile* tile) {
   //hard code this for now
   Garbage* garbage = nullptr;
   switch (tile->type) {
   case tile_empty:
      meshSetTexture(board->game, tile->mesh, Texture_empty);
      break;
   case tile_circle:
      meshSetTexture(board->game, tile->mesh, Texture_circle);
      break;
   case tile_diamond:
      meshSetTexture(board->game, tile->mesh, Texture_diamond);
      break;
   case tile_utriangle:
      meshSetTexture(board->game, tile->mesh, Texture_utriangle);
      break;
   case tile_dtriangle:
      meshSetTexture(board->game, tile->mesh, Texture_dtriangle);
      break;
   case tile_star:
      meshSetTexture(board->game, tile->mesh, Texture_star);
      break;
   case tile_heart:
      meshSetTexture(board->game, tile->mesh, Texture_heart);
      break;
   case tile_silver:
      meshSetTexture(board->game, tile->mesh, Texture_silver);
      break;
   case tile_cleared:
      meshSetTexture(board->game, tile->mesh, Texture_cleared);
      break;
   case tile_garbage:
      if (tile->idGarbage >= 0) {
         garbage = garbageGet(board->pile, tile->idGarbage);
         if (garbage && garbage->metal == true) {
            meshSetTexture(board->game, tile->mesh, Texture_metal);
         }
         else { meshSetTexture(board->game, tile->mesh, Texture_g); }
      }
      else {
         meshSetTexture(board->game, tile->mesh, Texture_g);
      }
      break;
   default:
      meshSetTexture(board->game, tile->mesh, Texture_empty);
   }
}

void tileInit(Board* board, Tile* tile, int row, int col, TileType type, bool firstTime) {
   tile->type = type;
   tile->status = status_normal;
   tile->xpos = col * board->tileWidth;
   tile->ypos = (row - board->startH) * board->tileHeight;

   if (firstTime == true) {
      tile->mesh = meshCreate(board->game);
   }

   tile->idGarbage = -1;
   tile->garbage = nullptr;

   tileSetTexture(board, tile);

   tile->clearTime = 0;
   tile->statusTime = 0;
   tile->falling = false;
   tile->chain = false;
}

void tileUpdate(Board* board, Tile* tile) {
   //todo put something here
}

void tileDraw(Board* board, Tile* tile) {
   Vec2 adj = board->origin;
   textureTransform(board->game, tile->mesh, 0, 0, 32, 32);
   if (meshGetTexture(tile->mesh) != Texture_empty) {
      VisualEffect effect = visual_none;
      if (tile->status == status_disable || tileGetRow(board, tile) == 24) {
         effect = visual_dark;
      }
      meshDraw(board->game, tile->mesh, tile->xpos + adj.x, tile->ypos + adj.y, board->tileWidth, board->tileHeight, effect);
   }
}