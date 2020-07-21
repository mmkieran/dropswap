
#include "garbage.h"

#include "board.h"
#include "tile.h"
#include "mymath.h"
#include "render.h"
#include "resources.h"

#include <vector>

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
   id++;

   return garbage;
}

void garbageDestroy(Garbage* garbage) {
   delete garbage;
}

void garbageCheckClear(Board* board, Tile* tile) {
   int row = yPosToRow(board, tile->ypos);
   int col = xPosToCol(board, tile->xpos);

   int indices[8] = { -1, 0, 0, -1, 1, 0, 0, 1 };

   for (int i = 0; i < 4; i += 2) {
      Tile* tile = boardGetTile(board, row + indices[i], col + indices[i + 1]);
      if (tile && tile->type == tile_garbage) {
         //check if it touches any other garbage that is <2 layers
         garbageClear(board, tile);
      }
   }
}

void garbageClear(Board* board, Tile* tile) {
   for (auto&& garbage : board->garbage) {
      if (garbage->ID == tile->idGarbage) {  //todo maybe make a lookup table instead

         int row = (garbage->start->ypos + board->tileHeight - 0.01f) / board->tileHeight + board->startH;

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
}

void garbageFall(Board* board, float velocity) {

   for (auto&& garbage : board->garbage) {

      float drop = 1.0f * velocity;

      garbage->falling = true;

      //int row = (garbage->start->ypos + board->tileHeight - 0.01f) / board->tileHeight + board->startH;
      int row = (garbage->start - board->tiles) / board->w;
      int col = xPosToCol(board, garbage->start->xpos);

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

void garbageDraw(Board* board) {
   
   for (auto&& garbage : board->garbage) {
      float xpos, ypos;

      xpos = garbage->start->xpos;
      ypos = garbage->start->ypos - (board->tileHeight * (garbage->layers - 1));

      //drawMesh(board->game, garbage->mesh, xpos, ypos, garbage->width * board->tileWidth, garbage->layers * board->tileHeight);
   }
}

