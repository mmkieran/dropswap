
#include "garbage.h"

#include "board.h"
#include "tile.h"

#include <vector>

struct Garbage {
   std::vector <Tile*> chunk;
};

void garbageCreate(Board* board, int size) {
   /*Pick a side
   Mark tiles at the top of the spare board
   If you can't mark any, then bust (this means garbage needs to have a max size?)
   Let the tiles fall naturally
   */
   int number = 1;
   size = 3;

   Tile* tile = boardGetTile(board, 0, 0);
   if (tile->type != tile_empty) {
      board->bust = true;  //todo add bust logic
   }

   for (int row = 0; row < number; row++) {
      for (int col = 0; col < size; col++) {
         Tile* tile = boardGetTile(board, row, col);
         tile->type = tile_garbage;
         //set the pointer to the garbage here
      }
   }

}

void garbageClear() {
   /*Find all related tiles
turn them into ordinary, random tiles
Make the new tiles part of a chain
Let them fall
*/

}

void garbageFall() {
   /*The garbage tiles need to fall together
   This will require special logic in UpdateFalling
   */

}

