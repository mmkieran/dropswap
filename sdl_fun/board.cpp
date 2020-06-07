#include "tile.h"
#include <stdlib.h>
#include <random>
#include "game.h"

/*
2D array that is 6 x 12
Needs to have some kind of buffer for off the top

should have speed and time

create a new board
reset board

*/

struct Board {
   int h = 12;
   int w = 6;

   Tile* tiles;

   double speed;
   int time;

   double score;
   bool bust;

   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution;

};

Tile* _boardCreateArray(int height, int width) {
   Tile* tiles = (Tile*)malloc(sizeof(Tile) * height * width);
   memset(tiles, 0, sizeof(Tile) * height * width);
   return tiles;
}

Board* boardCreate(int height, int width) {
   Board* board = (Board*)malloc(sizeof(Board));
   if (board) {
      Tile* tiles = _boardCreateArray(height, width);
      if (tiles) {
         //fillTiles();
         board->tiles = tiles;
         board->h = height;
         board->w = width;
         board->time = 0;
         board->speed = 1;
         board->bust = false;
         std::uniform_int_distribution<int> dist (1, 6);
         board->distribution = dist;
         return board;
      }
   }
   return nullptr;
}

void boardDestroy(Board* board) {
   free(board->tiles);
   free(board);
}


Tile* boardGetTile(Board* board, int row, int col) {
   Tile* tile = &board->tiles[(board->w * row + col)];
   return tile;
}

//void boardSetTileType(Board* board, int row, int col, TileEnum type) {
//   Tile* tile = boardGetTile(board, row, col);
//   tile->type = type;
//}


int boardPrint(Board* board) {
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         printf("xpos: %d, ypos: %d, type: %d\n", tile->xpos, tile->ypos, tile->type);
      }
   }

   return 0;
}



int boardFillTiles(Board* board) {
   //Need special logic to make sure blocks don't create chains at the start
   //Might just have pre-made boards
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
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
   }
   //boardPrint(board);
   return 0;
}

void boardUpdate(Board* board) {
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);

         tile->ypos -= 1;
         tile->destRect.y = tile->ypos;
      }
   }
}

void boardRender(Board* board) {
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         SDL_RenderCopy(Game::renderer, tile->texture, &tile->srcRect, &tile->destRect);
      }
   }
}