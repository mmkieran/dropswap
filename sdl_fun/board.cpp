#include "tile.cpp"
#include <stdlib.h>
#include <random>

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

Board* boardCreate(int height, int width) {
   Board* board = (Board*)malloc(sizeof(Board));
   if (board) {
      Tile* tiles = _boardCreateArray(height, width);
      if (tiles) {
         //fillTiles();
         board->h = height;
         board->w = width;
         board->time = 0;
         board->speed = 1;
         board->bust = false;
         std::uniform_int_distribution<int> dist (1, 6);
         board->distribution = dist;
      }
   }
   return nullptr;
}

Board* boardDestroy(Board* board) {
   free(board->tiles);
   free(board);
}

Tile* _boardCreateArray(int height, int width){
   Tile* tiles = (Tile*)malloc(sizeof(Tile) * height * width);
   return tiles;
}

Tile* boardGetTile(Board* board, int row, int col) {
   Tile* tile = &board->tiles[board->w * row + col];
}

//void boardSetTileType(Board* board, int row, int col, TileEnum type) {
//   Tile* tile = boardGetTile(board, row, col);
//   tile->type = type;
//}


int fillTiles(Board* board) {
   //Need special logic to make sure blocks don't create chains at the start
   //Might just have pre-made boards
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         tile->type = (TileEnum)board->distribution(board->generator);
         tile->xpos = row * 64;
         tile->ypos = col * 64;
         tileLoadTexture(tile, "assets/dirt.png");
         return 0;
      }
   }

   return 1;
}

