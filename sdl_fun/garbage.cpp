
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

   //todo For now always start on the top left
   Tile* tile = boardGetTile(board, 12, 0);
   garbage->start = tile;  //do this for now to start
   tile->garbage = garbage;

   if (tile->type != tile_empty) {
      board->bust = false;  //todo add bust logic
   }

   for (int row = 12; row >= 12 - layers; row--) {
      for (int col = 0; col < width; col++) {
         Tile* tile = boardGetTile(board, row, col);
         tile->type = tile_garbage;
      }
   }
   id++;

   return garbage;
}

void garbageDestroy(Garbage* garbage) {
   delete garbage;
}

void garbageClear() {
   /*Find all related tiles
turn them into ordinary, random tiles
stop the board during this process
Make the new tiles part of a chain
Let them fall
*/

}

void garbageFall(Board* board, float velocity) {

   for (auto&& garbage : board->garbage) {

      float drop = 1.0f * velocity;

      bool letFall = true;

      int row = yPosToRow(board, garbage->start->ypos);
      int col = xPosToCol(board, garbage->start->xpos);

      //Loop through and find out if the bottom layer can fall
      for (int i = col; i < garbage->width; i++) {
         Tile* tile = boardGetTile(board, row, i);
         if (row < board->endH) {
            Tile* below = boardGetTile(board, row + 1, i);

            if (below->type == tile_empty || below->falling == true) {
               if (tile->ypos + board->tileHeight + drop >= below->ypos && below->falling == true) {  //snap to tile's edge if drop is too much
                  float potentialDrop = below->ypos - (tile->ypos + board->tileHeight);  //check how far we can drop it
                  if (potentialDrop < drop) { drop = potentialDrop; }  //if this tile can't fall as far as others, adjust the max drop for all
               }
            }

            else if (below->falling == false) {
               if (tile->ypos + board->tileHeight + drop >= below->ypos) {  //if the below tile is not falling, stop at it's edge
                  float potentialDrop = below->ypos - (tile->ypos + board->tileHeight);  //check how far we can drop it
                  if (potentialDrop < drop) { drop = potentialDrop; }  //if this tile can't fall as far as others, adjust the max drop for all
               }
            }

            else {
               letFall = false;
            }

         }
      }

      //If the bottom layer can fall, adjust the ypos with the max drop
      if (letFall == true && drop > 0) {
         for (int r = row; r >= row - garbage->layers; r--) {
            for (int c = 0; c < garbage->width; c++) {
               Tile* tile = boardGetTile(board, r, c);
               tile->ypos += drop;
            }
         }
      }
   }
}

void garbageDraw(Board* board) {
   
   for (auto&& garbage : board->garbage) {
      float xpos, ypos;

      xpos = garbage->start->xpos;
      ypos = garbage->start->ypos; //- (board->tileHeight * garbage->layers );

      drawMesh(board->game, garbage->mesh, xpos, ypos, garbage->width * board->tileWidth, garbage->layers * board->tileHeight);
   }
}

