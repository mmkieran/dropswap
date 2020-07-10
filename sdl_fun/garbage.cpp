#include "board.h"
#include <vector>

struct Garbage {
   std::vector <Tile*> chunk;
};

void garbageCreate() {
   /*Pick a side
   Mark tiles at the top of the spare board
   If you can't mark any, then bust (this means garbage needs to have a max size?)
   Let the tiles fall naturally
   */

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

