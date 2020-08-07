
#include "garbage.h"

#include "board.h"
#include "tile.h"
#include "mymath.h"
#include "render.h"
#include "resources.h"

#include <vector>
#include <map>

struct GarbagePile {
   std::map <int, Garbage*> garbage;
   int nextID = 0;
};

GarbagePile* garbagePileCreate() {
   GarbagePile* pile = nullptr;
   pile = new GarbagePile;
   if (pile) {
      return pile;
   }
   return nullptr;
}

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

void garbageClear(Board* board, std::map <int, Garbage*> cleared);

static Garbage* _garbageCreate(Board* board) {
   Garbage* garbage = new Garbage;

   garbage->mesh = meshCreate(board->game);
   meshSetTexture(board->game, garbage->mesh, Texture_garbage);
   //textureParams(garbage->mesh->texture, mirror);  //todo redo texture parameters

   return garbage;
}

Garbage* garbageCreate(Board* board, int width, int layers) {
   
   Garbage* garbage = _garbageCreate(board);
   garbage->ID = board->pile->nextID;
   garbage->width = width;
   garbage->layers = layers;
   garbage->deployTime = board->game->timer + 3000;

   board->pile->garbage[garbage->ID] = garbage;
   board->pile->nextID++;

   return garbage;
}

static int _findEmptySpace(Board* board, Garbage* garbage, int startRow, int startCol) {
   //Returns zero if there's space to deploy garbage, otherwise returns the last row that was blocked

   int failLayer = 0;
   for (int row = startRow; row > startRow - garbage->layers; row--) {
      for (int col = startCol ; col < garbage->width + startCol; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type != tile_empty) {
            failLayer = row;
         }
      }
   }

   return failLayer;
}

void garbageDeploy(Board* board) {

   if (board->paused) { return; } //don't deploy while the board is paused

   int startRow = board->startH - 1;  //Start deploying above visible board
   bool noSpace = false;

   for (auto&& pair : board->pile->garbage) {  //iterating a map gives std::pair (use first and second)
      Garbage* garbage = pair.second;
      if (garbage->deployed == false && garbage->deployTime < board->game->timer) {  //get all garbage ready to deploy

         int col = 0;
         if (garbage->layers == 1 && garbage->width < 6 && garbage->ID % 2 == 0) {  //place on right side of board
            col = board->w - garbage->width; 
         }
 
         bool searching = true; //looking for space to put the garbage
         while (searching == true && noSpace == false) {

            int rowFull = _findEmptySpace(board, garbage, startRow, col);

            if (rowFull == 0) {
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
            else { startRow = rowFull + 1; }
            std::vector <Tile> debug = boardDebug(board);
            if (startRow > 0) {
               noSpace = true;
            }

         }
      }
   }
}

Garbage* garbageDestroy(Garbage* garbage) {
   if (garbage) {
      delete garbage;
   }
   return nullptr;
}


Garbage* garbageGet(GarbagePile* pile, int id) {
   if (pile) {
      Garbage* garbage = pile->garbage[id];
      if (!garbage) { return nullptr; }
      return garbage;
   }
   return nullptr;
}

void garbageSetStart(GarbagePile* pile, Tile* tile) {
   if (tile && pile) {
      pile->garbage[tile->idGarbage]->start = tile;
   }
}

static void _findTouching(Board* board, Garbage* garbage, std::map <int, Garbage*> &cleared, std::vector <Garbage*> &checkList) {

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
                     if (belowGarbage->layers < 2) {
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
                     if (leftGarbage->layers < 2) {
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
                     if (aboveGarbage->layers < 2) {
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
                     if (rightGarbage->layers < 2) {
                        checkList.push_back(rightGarbage);
                     }
                  }
               }
            }
         }
      }
   }
}

void garbageCheckClear(Board* board, Tile* tile) {
   //Check if cleared tiles are touching garbage blocks
   int row = tileGetRow(board, tile);
   int col = tileGetCol(board, tile);

   int indices[8] = { -1, 0, 0, -1, 1, 0, 0, 1 };

   std::map <int, Garbage*> cleared;
   std::vector <Garbage*> checkList;

   for (int i = 0; i < 8; i += 2) {
      Tile* neighbor = boardGetTile(board, row + indices[i], col + indices[i + 1]);
      if (neighbor && neighbor->type == tile_garbage) {
         //check if it touches any other garbage that is <2 layers
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

   garbageClear(board, cleared);
}

static void garbageClear(Board* board, std::map <int, Garbage*> cleared) {

   for (auto&& pair : cleared) {
      Garbage* garbage = pair.second;

      int row = tileGetRow(board, garbage->start);
      int col = tileGetCol(board, garbage->start);

      uint64_t clearTime = board->game->timer;
      for (int c = col; c < garbage->width + col; c++) {  //clear the bottom layer
         Tile* tile = boardGetTile(board, row, c);
         if (tile->garbage && garbage->layers > 1) {
            Tile* newStart = boardGetTile(board, row - 1, c);
            garbage->start = newStart;
            newStart->garbage = tile->garbage;
            tile->garbage = nullptr;
         }
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

void garbageFall(Board* board, float velocity) {

   for (auto&& pair : board->pile->garbage) {  //iterating a map gives std::pair (use first and second)
      Garbage* garbage = pair.second;
      if (garbage->deployed == true) {

         float drop = board->level * velocity;

         int row = tileGetRow(board, garbage->start);
         int col = tileGetCol(board, garbage->start);

         float potentialDrop = drop;
         //Loop through and find out if the bottom layer can fall
         for (int i = col; i < garbage->width + col; i++) {
            Tile* tile = boardGetTile(board, row, i);

            int lookDown = 2;
            Tile* below = boardGetTile(board, row + 1, i);
            while (below && below->type == tile_empty) {
               below = boardGetTile(board, row + lookDown, i);
               lookDown++;
            }

            potentialDrop = below->ypos - (tile->ypos + (float)board->tileHeight);  //check how far we can drop it

            if (potentialDrop < 0) {  //Not sure if this can happen
               garbage->falling = false;
               break;
            }
            else if (potentialDrop == 0) { //It is stopped
               garbage->falling = false;
               break;
            }
            else if (potentialDrop < drop) {  //It can fall a little bit further
               drop = potentialDrop;
               garbage->falling = true;
            }
            else if (potentialDrop >= drop) {  //We can fall as much as we want
               garbage->falling = true;
            }
            else {
               printf("Something bad happened dropping: %d, %f, %f", tile->type, tile->xpos, tile->ypos);
               garbage->falling = false;
               break;
            }
         }

         //If the bottom layer can fall, adjust the ypos with the max drop
         if (garbage->falling == true && drop > 0) {
            for (int r = row; r > row - garbage->layers; r--) {
               for (int c = col; c < garbage->width + col; c++) {
                  Tile* tile = boardGetTile(board, r, c);
                  tile->ypos += drop;
                  //if (drop == velocity) {
                  //   tile->falling = true;
                  //}
                  //else {
                  //   tile->falling = false;
                  //   garbage->falling = false;
                  //}
               }
            }
         }
      }
   }
}

void garbageDraw(Board* board) {  //iterating a map gives std::pair (use first and second)
   
	for (auto&& pair : board->pile->garbage) {
		Garbage* garbage = pair.second;

		if (garbage->deployed == true) {
			Garbage* garbage = pair.second;
			float xpos, ypos;

			xpos = garbage->start->xpos;
			ypos = garbage->start->ypos - (board->tileHeight * (garbage->layers - 1));

			//todo look at rendering two textures on one mesh
			//meshDraw(board->game, garbage->mesh, xpos, ypos, garbage->width * board->tileWidth, garbage->layers * board->tileHeight);
		}
	}
}

void _garbageSerialize(Board* board, FILE* file) {

	fwrite(&board->pile->nextID, sizeof(int), 1, file);
	int count = board->pile->garbage.size();
	fwrite(&count, sizeof(int), 1, file);

	for (auto&& pair : board->pile->garbage) {  //iterating a map gives std::pair (use first and second)
		Garbage* garbage = pair.second;

		fwrite(&garbage->ID, sizeof(int), 1, file);
		fwrite(&garbage->width, sizeof(int), 1, file);
		fwrite(&garbage->layers, sizeof(int), 1, file);
		//   Tile* start;  
		//   Mesh* mesh;
		fwrite(&garbage->deployed, sizeof(bool), 1, file);
      fwrite(&garbage->deployTime, sizeof(uint64_t), 1, file);
		fwrite(&garbage->falling, sizeof(bool), 1, file);
	}
}

void _garbageDeserialize(Board* board, FILE* file) {

	fread(&board->pile->nextID, sizeof(int), 1, file);

	int count = 0;
	fread(&count, sizeof(int), 1, file);

	for (int i = 0; i < count; i++) {  //iterating a map gives std::pair (use first and second)
		Garbage* garbage = _garbageCreate(board);

		fread(&garbage->ID, sizeof(int), 1, file);
		fread(&garbage->width, sizeof(int), 1, file);
		fread(&garbage->layers, sizeof(int), 1, file);
		//   Tile* start;  
		//   Mesh* mesh;
		fread(&garbage->deployed, sizeof(bool), 1, file);
      fread(&garbage->deployTime, sizeof(uint64_t), 1, file);
		fread(&garbage->falling, sizeof(bool), 1, file);

		board->pile->garbage[garbage->ID] = garbage;
	}
}