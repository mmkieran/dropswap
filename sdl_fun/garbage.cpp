
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
};

void garbageCreate(Board* board, int width, int layers) {

   Garbage* garbage = new Garbage;

   static int id = 0;

   garbage->mesh = createMesh(board->game);
   garbage->mesh->texture = resourcesGetTexture(board->game->resources, Texture_garbage);
   garbage->ID = id;
   garbage->width = width;
   garbage->layers = layers;

   //todo For now always start on the top left
   Tile* tile = boardGetTile(board, 0, 0);
   garbage->start = tile;  //do this for now to start

   if (tile->type != tile_empty) {
      board->bust = true;  //todo add bust logic
   }

   for (int row = 0; row < layers; row++) {
      for (int col = 0; col < width; col++) {
         Tile* tile = boardGetTile(board, row, col);
         tile->type = tile_garbage;
         tile->garbage = garbage;
      }
   }
   id++;
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

void garbageFall(Board* board, Garbage* garbage, float velocity) {
   /*The garbage tiles need to fall together
   This will require special logic in UpdateFalling
   */

   float drop = 1.0f * velocity;
   
   bool letFall = true;

   int row = yPosToRow(board, garbage->start->ypos);
   int col = xPosToCol(board, garbage->start->xpos);

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

   if (letFall == true && drop > 0) {  //now adjust the ypos with the max drop
      for (int row = 0; row < garbage->layers; row--) {
         for (int col = 0; col < garbage->width; col++) {
            Tile* tile = boardGetTile(board, row, col);
            tile->ypos += drop;
         }
      }
   }
}

void garbageDraw() {

}

