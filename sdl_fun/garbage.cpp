
#include "garbage.h"

#include "board.h"
#include "tile.h"
#include "mymath.h"
#include "render.h"
#include "resources.h"

#include <assert.h>

#define SHAKETIME 100

void garbageClear(Board* board, std::map <int, Garbage*> cleared);

//Garbage Pile is a hash map of the pieces of Garbage
GarbagePile* garbagePileCreate() {
   GarbagePile* pile = nullptr;
   pile = new GarbagePile;
   if (pile) {
      return pile;
   }
   return nullptr;
}

void garbagePileEmpty(GarbagePile* pile) {
   if (pile) {
      for (auto&& pair : pile->garbage) {
         if (pair.second) {
            garbageDestroy(pair.second);
         }
      }
      pile->nextID = 0;
      pile->garbage.clear();
   }
}

//Frees the memory for a Garbage Pile
GarbagePile* garbagePileDestroy(GarbagePile* pile) {
   if (pile) {
      for (auto&& pair : pile->garbage) {
         if (pair.second) {
            garbageDestroy(pair.second);
         }
      }
      delete pile;
   }
   return nullptr;
}

//Allocate memory for a piece of garbage... for serializing
Garbage* garbageCreateEmpty(Board* board) {
   Garbage* garbage = new Garbage;

   return garbage;
}

//This is typical way to create Garbage of a certain size
Garbage* garbageCreate(Board* board, int width, int layers, bool metal) {
   
   Garbage* garbage = garbageCreateEmpty(board);
   garbage->ID = board->pile->nextID;
   garbage->width = width;
   garbage->layers = layers;
   garbage->deployTime = board->game->timer + board->game->timings.deployTime[0];
   if (metal == true) { garbage->metal = true; }

   board->pile->garbage[garbage->ID] = garbage;
   board->pile->nextID++;

   return garbage;
}

//Returns -1 if there's space to deploy garbage, otherwise returns the last row that was blocked
static int _findEmptySpace(Board* board, Garbage* garbage, int startRow, int startCol) {
   int failLayer = -1;
   for (int row = startRow; row > startRow - garbage->layers; row--) {
      for (int col = startCol ; col < garbage->width + startCol; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile == nullptr) {
            failLayer = 0;
            return failLayer;
         }
         else if (tile->type != tile_empty) {
            failLayer = row;
         }
      }
   }

   return failLayer;
}

//Deploys garbage above the visible board if there is space
void garbageDeploy(Board* board) {
   int startRow = board->startH - 1;  //Start deploying above visible board
   bool noSpace = false;

   for (auto&& pair : board->pile->garbage) {  //iterating a map gives std::pair (use first and second)
      Garbage* garbage = pair.second;
      if (garbage == nullptr) { continue; }

      if (garbage->deployed == false && garbage->deployTime < board->game->timer) {  //get all garbage ready to deploy

         int col = 0;
         if (garbage->layers == 1 && garbage->width < 6 && garbage->ID % 2 == 0) {  //place on right side of board
            col = board->w - garbage->width; 
         }
 
         bool searching = true; //looking for space to put the garbage
         while (searching == true && noSpace == false) {

            int rowFull = _findEmptySpace(board, garbage, startRow, col);

            if (rowFull == -1) {
               for (int row = startRow; row > startRow - garbage->layers; row--) {
                  for (int c = col ; c < garbage->width + col; c++) {
                     Tile* tile = boardGetTile(board, row, c);
                     tile->type = tile_garbage;
                     tileSetTexture(board, tile);
                     tile->idGarbage = garbage->ID;

                     if (row == startRow && c == col) {
                        garbage->start = tile;
                        tile->garbage = garbage;
                     }
                  }
               }
               garbage->deployed = true;
               garbage->deployTime = 0;
            }
            else { startRow = rowFull -1; }  
            if (startRow > 0) {
               noSpace = true;
            }

         }
      }
   }
}

//Frees the memory of a pointer to a piece of Garbage
Garbage* garbageDestroy(Garbage* garbage) {
   if (garbage) {
      delete garbage;
   }
   return nullptr;
}

//Get a pointer to a piece of garbage in the pile using it's ID
Garbage* garbageGet(GarbagePile* pile, int id) {
   if (pile && id >= 0) {
      Garbage* garbage = pile->garbage[id];
      if (!garbage) { return nullptr; }
      return garbage;
   }
   return nullptr;
}

//Assigns a tile as the start of a piece of Garbage (bottom left corner)
void garbageSetStart(GarbagePile* pile, Tile* tile) {
   if (tile && pile) {
      pile->garbage[tile->idGarbage]->start = tile;
   }
}

//static bool _addTouching(Board* board, Tile* tile,std::map <int, Garbage*>& cleared, std::vector <Garbage*>& checkList) {
//   if (tile && tile->type == tile_garbage) {  //found more garbage
//      Garbage* belowGarbage = garbageGet(board->pile, tile->idGarbage);
//      if (belowGarbage) {
//         if (cleared[belowGarbage->ID]) { return false; }
//         else {
//            cleared[belowGarbage->ID] = belowGarbage;
//            if (belowGarbage->layers < 2 && belowGarbage->metal == false) {
//               checkList.push_back(belowGarbage);
//               return true;
//            }
//         }
//      }
//   }
//}

//Checks all the tiles around a piece of Garbage to see if more garbage should be cleared
static void _findTouching(Board* board, Garbage* garbage, std::map <int, Garbage*> &cleared, std::vector <Garbage*> &checkList) {

   if (garbage->metal == true) { return; }

   int startRow = tileGetRow(board, garbage->start);
   int endRow = startRow - (garbage->layers - 1);

   int startCol = tileGetCol(board, garbage->start);
   int endCol = garbage->width - 1;

   for (int row = startRow; row > startRow - garbage->layers; row--) {  //start at bottom left and go up for each layer
      for (int col = startCol; col < garbage->width + startCol; col++) {

		 //todo make this one function instead of 4 chunk, lol

         if (row == startRow) {  //check below for more garbage
            Tile* below = boardGetTile(board, row + 1, col);
            if (below && below->type == tile_garbage) {  //found more garbage
               Garbage* belowGarbage = garbageGet(board->pile, below->idGarbage);
               if (belowGarbage) {
                  if (cleared[belowGarbage->ID]) { continue; }
                  else {
                     cleared[belowGarbage->ID] = belowGarbage;
                     if (belowGarbage->layers < 2 && belowGarbage->metal == false) {
                        checkList.push_back(belowGarbage);
                     }
                  }
               }
            }
         }

         if (col == startCol) {  //Check left
            Tile* left = boardGetTile(board, row, col - 1);
            if (left && left->type == tile_garbage) {  //found more garbage
               Garbage* leftGarbage = garbageGet(board->pile, left->idGarbage);
               if (leftGarbage) {
                  if (cleared[leftGarbage->ID]) { continue; }
                  else {
                     cleared[leftGarbage->ID] = leftGarbage;
                     if (leftGarbage->layers < 2 && leftGarbage->metal == false) {
                        checkList.push_back(leftGarbage);
                     }
                  }
               }
            }
         }

         if (row == endRow && endRow != startRow) {  //check above
            Tile* above = boardGetTile(board, row - 1, col);
            if (above && above->type == tile_garbage) {  //found more garbage
               Garbage* aboveGarbage = garbageGet(board->pile, above->idGarbage);
               if (aboveGarbage) {
                  if (cleared[aboveGarbage->ID]) { continue; }
                  else {
                     cleared[aboveGarbage->ID] = aboveGarbage;
                     if (aboveGarbage->layers < 2 && aboveGarbage->metal == false) {
                        checkList.push_back(aboveGarbage);
                     }
                  }
               }
            }
         }

         if (col == endCol && endCol != startCol) {  //check right
            Tile* right = boardGetTile(board, row, col + 1);
            if (right && right->type == tile_garbage) {  //found more garbage
               Garbage* rightGarbage = garbageGet(board->pile, right->idGarbage);
               if (rightGarbage) {
                  if (cleared[rightGarbage->ID]) { continue; }
                  else {
                     cleared[rightGarbage->ID] = rightGarbage;
                     if (rightGarbage->layers < 2 && rightGarbage->metal == false) {
                        checkList.push_back(rightGarbage);
                     }
                  }
               }
            }
         }
      }
   }
}

//Checks around cleared tiles for Garbage to be cleared
void garbageCheckClear(Board* board, Tile* tile) {
   if (tile == nullptr) { return; }
   int row = tileGetRow(board, tile);
   int col = tileGetCol(board, tile);

   int indices[8] = { -1, 0, 0, -1, 1, 0, 0, 1 };

   std::map <int, Garbage*> cleared;
   std::vector <Garbage*> checkList;

   for (int i = 0; i < 8; i += 2) {
      Tile* neighbor = boardGetTile(board, row + indices[i], col + indices[i + 1]);
      if (neighbor && neighbor->type == tile_garbage) {
         Garbage* garbage = garbageGet(board->pile, neighbor->idGarbage);
         if (cleared[garbage->ID]) { continue; }
         else { 
            cleared[garbage->ID] = garbage; 
            checkList.push_back(garbage);
         }
      }
   }

   for (int i = 0; i < checkList.size(); i++) {  
      _findTouching(board, checkList[i], cleared, checkList);
   }

   board->boardStats.garbageCrushed += cleared.size();  //Board Stats
   garbageClear(board, cleared);
}

//Takes a map of cleared garbage and removes the bottom layer from each
static void garbageClear(Board* board, std::map <int, Garbage*> cleared) {

   for (auto&& pair : cleared) {
      Garbage* garbage = pair.second;
      if (garbage == nullptr) { continue; }

      int row = tileGetRow(board, garbage->start);
      int col = tileGetCol(board, garbage->start);

      uint64_t clearTime = board->game->timer;
      for (int c = col; c < garbage->width + col; c++) {  //clear the bottom layer
         Tile* tile = boardGetTile(board, row, c);
         if (tile->garbage && garbage->layers > 1) {
            Tile* newStart = boardGetTile(board, row - 1, c);
            garbage->start = newStart;
            newStart->garbage = garbage;
         }
         tile->garbage = nullptr;
         tile->clearTime = clearTime + (200 * c + 1000);
         tile->statusTime -= 200 * c;
         tile->type = tile_cleared;
         tile->falling = false;
      }

      garbage->layers -= 1;
      if (garbage->layers == 0) {
         garbage = garbageDestroy(garbage);
         board->pile->garbage.erase(pair.first);
      }
   }
}

//Checks the bottom layer of the garbage to see if it can fall
void garbageFall(Board* board, double velocity) {

   for (auto&& pair : board->pile->garbage) {  //iterating a map gives std::pair (use first and second)
      Garbage* garbage = pair.second;
      if (garbage == nullptr) { continue; }
      if (garbage->deployed == true) {

         double drop = velocity;

         assert(garbage->start != nullptr);
         int row = tileGetRow(board, garbage->start);
         int col = tileGetCol(board, garbage->start);

         double potentialDrop = 0;
         for (int i = col; i < garbage->width + col; i++) {  //Find out if the bottom layer can fall
            Tile* tile = boardGetTile(board, row, i);

            int lookDown = 2;  //search underneath for non-empty tiles
            Tile* below = boardGetTile(board, row + 1, i);
            while (below && below->type == tile_empty) {
               below = boardGetTile(board, row + lookDown, i);
               lookDown++;
            }

            //Figure out how far each tile in the garbage could fall
            potentialDrop = below->ypos - (tile->ypos + (float)board->tileHeight); 

            if (potentialDrop <= 0) {  
               drop = 0;
               below = boardGetTile(board, row + 1, i);
               if (below && below->falling == false) { garbage->falling = false; }
               break;
            }
            else if (potentialDrop < drop) {  //It can fall a little bit further
               drop = potentialDrop;
            }
         }

         //If the bottom layer can fall, adjust the ypos with the max drop
         if (drop > 0) {
            garbage->falling = true;
            garbage->totalFall += drop;

            for (int r = row; r > row - garbage->layers; r--) {
               for (int c = col; c < garbage->width + col; c++) {
                  Tile* tile = boardGetTile(board, r, c);
                  tile->ypos += drop;
               }
            }
         }
         if (garbage->falling == false) {  
            if (garbage->totalFall > board->tileHeight * 2 - 0.00001) {  //Landing
               board->game->soundToggles[sound_crashland] = true;
               boardPauseTime(board, pause_crashland);

               boardEnableVisual(board, visual_shake, SHAKETIME);
            }
            garbage->totalFall = 0;
         }
      }
   }
}

//Render a special texture for the garbage
void garbageDraw(Board* board) {  //iterating a map gives std::pair (use first and second)
   
	for (auto&& pair : board->pile->garbage) {
		Garbage* garbage = pair.second;
      if (garbage == nullptr) { continue; }

		if (garbage->deployed == true) {
			Garbage* garbage = pair.second;
			float xpos, ypos;

			xpos = garbage->start->xpos;
			ypos = garbage->start->ypos - (board->tileHeight * (garbage->layers - 1));

			//todo look at rendering two textures on one mesh
			//meshDraw(board, garbage->mesh, xpos, ypos, garbage->width * board->tileWidth, garbage->layers * board->tileHeight);
		}
	}
}