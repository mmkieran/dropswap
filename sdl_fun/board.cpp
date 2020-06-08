#include "tile.h"
#include <random>
#include "game.h"
#include "cursor.h"

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
         if (row < 5) { 
            tileInitWithType(tile, row, col, tile_empty);
            continue; 
         }
         if (col == 2) { 
            tileInit(tile, row, col);
            continue; 
         }

         tileInit(tile, row, col);

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

void boardMoveUp(Board* board) {
   static bool top_warn = false;
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         //here we should collect a list of top blocks to clear
         if (row == 0 ){
            //if (tile->type != tile_empty && !top_warn) {
            //printf("Harry! I've reached the top");
            //top_warn = true;
            //}
            continue;
         }
         else {
            //Set x and y position here? Or just change tile? If I change the tile, I need to update the textures...
            //Probably just loop through board and render texture based on tile type
            Tile* above = boardGetTile(board, row - 1, col);
            above->type = tile->type;
            above->texture = tile->texture;
            if (row == board->h - 1) {
               tileInit(tile, row, col);

               //create new tiles here
            }
         }

      }
   }
}

int posYToRow(int y) {
   if (y % 64 != 0) {
      int remain = y % 64;
      return (y + (64 - remain)) / 64;
   }
   return y / 64;
}

int posXToCol(int x) {
   return x / 64;
}

void boardSwap(Board* board, Cursor* cursor) {
   int col = posXToCol(cursor->GetXPosition());
   int row = posYToRow(cursor->GetYPosition());

   Tile* tile1 = boardGetTile(board, row, col);
   Tile* tile2 = boardGetTile(board, row, col + 1);

   TileEnum tmpEnum = tile2->type;
   SDL_Texture* tmpTexture = tile2->texture;

   tile2->type = tile1->type;
   tile2->texture = tile1->texture;

   tile1->type = tmpEnum;
   tile1->texture = tmpTexture;

   return;
}

void boardCheckTiles() {
   //triggers for matches
   //swaps, board moving
   //falling blocks

   return;
}