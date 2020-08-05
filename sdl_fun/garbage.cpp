
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

	board->pile->garbage[garbage->ID] = garbage;
	board->pile->nextID++;

	return garbage;
}

void garbageDeploy(Board* board, Garbage* garbage) {
	//Get a list of all garbage that is ready to deploy
	//Find total size and start from bottom looking for space
	//Add garbage oldest to latest (id)
	//Anything that won't fit gets set as deployed false

	int start = garbage->layers;

	bool right = true;
	if (garbage->width < 6) {
		bool right = board->game->timer % 2;
	}

	//todo For now always start on the bottom left
	Tile* tile = boardGetTile(board, start, 0);
	garbage->start = tile;  //do this for now to start
	tile->garbage = garbage;

	for (int row = start; row > start - garbage->layers; row--) {  //start at bottom left and go up for each layer
		for (int col = 0; col < garbage->width; col++) {
			Tile* tile = boardGetTile(board, row, col);
			tile->type = tile_garbage;
			tileSetTexture(board, tile);
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
      for (int col = 0; col < garbage->width; col++) {

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

   //todo fix clear once you get the looping right
   garbageClear(board, cleared);
}

static void garbageClear(Board* board, std::map <int, Garbage*> cleared) {

   for (auto&& pair : cleared) {
      Garbage* garbage = pair.second;

      //int row = (garbage->start->ypos + board->tileHeight - 0.01f) / board->tileHeight + board->startH;  //todo check this
      int row = tileGetRow(board, garbage->start);

      uint64_t clearTime = board->game->timer;
      for (int col = 0; col < garbage->width; col++) {  //clear the bottom layer
         Tile* tile = boardGetTile(board, row, col);
         if (tile->garbage && garbage->layers > 1) {
            Tile* newStart = boardGetTile(board, row - 1, col);
            garbage->start = newStart;
            newStart->garbage = tile->garbage;
            tile->garbage = nullptr;
         }
         tile->clearTime = clearTime + (200 * col + 1000);
         tile->statusTime -= 200 * col;
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
   
   for (auto&& pair : board->pile->garbage) {
      Garbage* garbage = pair.second;
      float xpos, ypos;

      xpos = garbage->start->xpos;
      ypos = garbage->start->ypos - (board->tileHeight * (garbage->layers - 1));

      //meshDraw(board->game, garbage->mesh, xpos, ypos, garbage->width * board->tileWidth, garbage->layers * board->tileHeight);
   }
}

void _serializeGarbage(Board* board, FILE* file) {

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
		fwrite(&garbage->falling, sizeof(bool), 1, file);
	}
}

void _deserializeGarbage(Board* board, FILE* file) {

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
		fread(&garbage->falling, sizeof(bool), 1, file);

		board->pile->garbage[garbage->ID] = garbage;
	}
}