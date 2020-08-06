#include <stdlib.h>

#include "tile.h"
#include "resources.h"
#include "render.h"
#include "garbage.h"

void tileSetTexture(Board* board, Tile* tile) {
   //hard code this for now
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
      meshSetTexture(board->game, tile->mesh, Texture_g);
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
      meshDraw(board->game, tile->mesh, tile->xpos + adj.x, tile->ypos + adj.y, board->tileWidth, board->tileHeight);
   }
}


void _deserializeTileType(Tile* tile, FILE* file) {
   int type;
   fread(&type, sizeof(int), 1, file);
   if (type >= 0 && type < tile_COUNT) {
      tile->type = (TileType)type;
   }
   else { tile->type = tile_empty; }
}

void _serializeTileType(Tile* tile, FILE* file){
   int type = 0;
   if (tile->type) {
      type = (int)tile->type;
   }
   fwrite(&type, sizeof(int), 1, file);
}

void _deserializeTileStatus(Tile* tile, FILE* file) {
   int status;
   fread(&status, sizeof(int), 1, file);
   if (status >= 0 && status < status_COUNT) {
      tile->status = (TileStatus)status;
   }
   else { tile->status = status_normal; }
}

void _serializeTileStatus(Tile* tile, FILE* file) {
   int status = 0;
   if (tile->status) {
      status = (int)tile->status;
   }
   fwrite(&status, sizeof(int), 1, file);
}

void _serializeTileGarbage(Tile* tile, FILE* file) {
	bool start = false;
	if (tile->garbage != nullptr) {
		start = true;
	}
	fwrite(&start, sizeof(bool), 1, file);
}

void _deserializeTileGarbage(Board* board, Tile* tile, FILE* file) {
	bool start = false;
	fread(&start, sizeof(bool), 1, file);
	if (start == true) {
		garbageSetStart(board->pile, tile);
      tile->garbage = garbageGet(board->pile, tile->idGarbage);
	}
}