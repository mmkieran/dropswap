
#include "garbage.h"

#include "board.h"
#include "tile.h"
#include "mymath.h"
#include "render.h"
#include "resources.h"

#include <vector>

struct Garbage {
   int ID;

   int width;
   int layers;

   Tile* start;  //top left of garbage
   Mesh* mesh;

   bool falling;
};


Garbage* garbageCreate(Board* board, int width, int layers) {

   Garbage* garbage = new Garbage;

   static int id = 0;

   garbage->mesh = createMesh(board->game);
   garbage->mesh->texture = resourcesGetTexture(board->game->resources, Texture_garbage);
   changeTexParams(garbage->mesh->texture, mirror);
   garbage->ID = id;
   garbage->width = width;
   garbage->layers = layers;
   garbage->falling = true;

   int start = layers;

   //todo For now always start on the bottom left
   Tile* tile = boardGetTile(board, start, 0);
   garbage->start = tile;  //do this for now to start
   tile->garbage = garbage;

   if (tile->type != tile_empty) {
      board->bust = false;  //todo add bust logic
   }

   for (int row = start; row > start - layers; row--) {  //start at bottom left and go up for each layer
      for (int col = 0; col < width; col++) {
         Tile* tile = boardGetTile(board, row, col);
         tile->type = tile_garbage;
         tile->idGarbage = id;
         tileSetTexture(board, tile);
      }
   }
   board->garbage[garbage->ID] = garbage;
   id++;

   return garbage;
}

void garbageDestroy(Garbage* garbage) {
   delete garbage;
}

static void _checkClear(Board* board, Garbage* garbage) {

}

void garbageCheckClear(Board* board, Tile* tile) {
   
   int row = tileGetRow(board, tile);
   int col = tileGetCol(board, tile);

   int indices[8] = { -1, 0, 0, -1, 1, 0, 0, 1 };

   std::vector <int> keys;
   std::vector <Tile*> cleared;

   for (int i = 0; i < 8; i += 2) {
      Tile* tile = boardGetTile(board, row + indices[i], col + indices[i + 1]);
      if (tile && tile->type == tile_garbage) {
         //check if it touches any other garbage that is <2 layers
         Garbage* garbage = garbageGet(board, tile->idGarbage);
         
         int startRow = tileGetRow(board, garbage->start);
         int endRow = startRow + garbage->layers - 1;

         int startCol = tileGetCol(board, garbage->start);
         int endCol = garbage->width - 1;

         for (int row = startRow; row > startRow - garbage->layers; row--) {  //start at bottom left and go up for each layer
            for (int col = 0; col < garbage->width; col++) {
               if (row == startRow) {
                  //get the tile below
               }
               if (col == startCol) {
                  //get the tile to the left
               }
               if (row == endRow && endRow != startRow) {
                  //check above
               }
               if (col == endCol && endCol != startCol) {
                  //check the tile to the right
               }
            }
         }

      }
   }

   //todo fix clear once you get the looping right
   garbageClear(board, tile);
}

void garbageClear(Board* board, Tile* tile) {

   Garbage* garbage = garbageGet(board, tile->idGarbage);
   if (garbage) {

      int row = (garbage->start->ypos + board->tileHeight - 0.01f) / board->tileHeight + board->startH;  //todo check this

      uint64_t clearTime = SDL_GetTicks();
      for (int col = 0; col < garbage->width; col++) {  //clear the bottom layer
         Tile* tile = boardGetTile(board, row, col);
         if (tile->garbage && garbage->layers > 1) {
            Tile* newStart = boardGetTile(board, row - 1, col);
            garbage->start = newStart;
            newStart->garbage = tile->garbage;
            tile->garbage = nullptr;
         }
         tile->clearTime = clearTime + (200 * col + 1000);
         tile->type = tile_cleared;
         tile->falling = false;
      }
      garbage->layers -= 1;
   }
}

void garbageFall(Board* board, float velocity) {

   for (auto&& pair : board->garbage) {  //iterating a map gives std::pair (use first and second)
      Garbage* garbage = pair.second;

      float drop = board->level * velocity;

      garbage->falling = true;

      //int row = (garbage->start->ypos + board->tileHeight - 0.01f) / board->tileHeight + board->startH;
      int row = tileGetRow(board, garbage->start);
      int col = tileGetCol(board, garbage->start);

      //Loop through and find out if the bottom layer can fall
      for (int i = 0; i < garbage->width; i++) {
         Tile* tile = boardGetTile(board, row, i);
         if (row < board->endH) {
            Tile* below = boardGetTile(board, row + 1, i);

            if (below->type == tile_empty || below->falling == true) {
               if (tile->ypos + board->tileHeight + drop >= below->ypos && below->falling == true) {  //snap to tile's edge if drop is too much
                  float potentialDrop = below->ypos - (tile->ypos + (float)board->tileHeight);  //check how far we can drop it
                  if (potentialDrop <= 0) { 
                     potentialDrop = 0; 
                     garbage->falling = false;
                  }
                  else if (potentialDrop < drop) { 
                     drop = potentialDrop; 
                  }  //if this tile can't fall as far as others, adjust the max drop for all
               }
            }

            else if (below->falling == false) {
               if (tile->ypos + board->tileHeight + drop >= below->ypos) {  //if the below tile is not falling, stop at it's edge
                  float potentialDrop = below->ypos - (tile->ypos + (float)board->tileHeight);  //check how far we can drop it
                  if (potentialDrop <= 0) { 
                     potentialDrop = 0; 
                     garbage->falling = false;
                  }
                  else if (potentialDrop < drop) {
                     drop = potentialDrop; 
                  }  //if this tile can't fall as far as others, adjust the max drop for all
               }
            }

            else {
               garbage->falling = false;
            }
         }
      }

      //If the bottom layer can fall, adjust the ypos with the max drop
      if (garbage->falling == true && drop > 0) {
         for (int r = row; r > row - garbage->layers; r--) {
            for (int c = 0; c < garbage->width; c++) {
               Tile* tile = boardGetTile(board, r, c);
               tile->ypos += drop;
               if (drop == velocity) {
                  tile->falling = true;
               }
               else { 
                  tile->falling = false; 
                  garbage->falling = false;
               }
            }
         }
      }
   }
}

void garbageDraw(Board* board) {  //iterating a map gives std::pair (use first and second)
   
   for (auto&& pair : board->garbage) {
      Garbage* garbage = pair.second;
      float xpos, ypos;

      xpos = garbage->start->xpos;
      ypos = garbage->start->ypos - (board->tileHeight * (garbage->layers - 1));

      //drawMesh(board->game, garbage->mesh, xpos, ypos, garbage->width * board->tileWidth, garbage->layers * board->tileHeight);
   }
}
Garbage* garbageGet(Board* board, int id) {
   Garbage* garbage = board->garbage[id];
   if (!garbage) { return nullptr; }
   return garbage;
}

void garbageSetStart(Board* board, Tile* tile) {
   board->garbage[tile->idGarbage]->start = tile;
}
