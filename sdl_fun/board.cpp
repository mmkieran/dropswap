#include <vector>
#include <time.h>
#include <chrono>

#include "board.h"
#include "resources.h"
#include "tile.h"
#include "cursor.h"
#include "garbage.h"

void _checkClear(std::vector <Tile*> tiles, std::vector <Tile*> &matches);
void boardCheckClear(Board* board, std::vector <Tile*> tileList, bool fallCombo);

Tile* _boardCreateArray(int width, int height) {
   Tile* tiles = (Tile*)malloc(sizeof(Tile) * (height * 2 + 1) * width);
   //memset(tiles, 0, sizeof(Tile) * (height * 2 + 1) * width);
   return tiles;
}

Board* boardCreate(Game* game) {
   Board* board = new Board;
   if (board) {
      Tile* tiles = _boardCreateArray(game->bHeight, game->bWidth);
      if (tiles) {
         board->game = game;

         board->tiles = tiles;

         board->startH = game->bHeight;  //start playing area of the board... everything above is for garbage
         board->endH = game->bHeight * 2;  //end playing area of the board
         board->wBuffer = board->endH + 1;  //Extra row upcoming rows
         board->w = game->bWidth;
         board->offset = 0;

         board->tileHeight = game->tHeight;
         board->tileWidth = game->tWidth;
         board->speed = 0.5;
         board->paused = false;
         board->pauseLength = 0;
         board->bust = false;
         board->score = 0;
         board->combo = 1;

         board->frame = createMesh(board->game);
         board->frame->texture = resourcesGetTexture(game->resources, Texture_frame);

         std::default_random_engine gen(time(0));
         board->generator = gen;

         std::uniform_int_distribution<int> dist (1, 6);
         board->distribution = dist;

         float cursorX = (float)(game->bWidth / 2 - 1) * game->tWidth;
         float cursorY = (float)(game->bHeight / 2 + 1) * game->tHeight;
         board->cursor = cursorCreate(board, cursorX, cursorY);

         return board;
      }
   }
   return nullptr;
}

void boardDestroy(Board* board) {
   for (int row = 0; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         destroyMesh(tile->mesh);
      }
   }

   for (auto&& garbage : board->garbage) {
      garbageDestroy(garbage);
   }

   free(board->tiles);
   //free(board);
   delete board;
}

Tile* boardGetTile(Board* board, int row, int col) {
   if (row < 0 || row > board->wBuffer - 1) {
      return nullptr;
   }
   Tile* tile = &board->tiles[(board->w * row + col)];
   return tile;
}

void boardSetTile(Board* board, Tile tile, int row, int col) {
   if (row < 0 || row > board->wBuffer - 1) {
      return;
   }
   board->tiles[(board->w * row + col)] = tile;
}

void boardRender(Game* game, Board* board) {
   for (int row = board->startH - 1; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         //if (tile->mesh->texture == nullptr || tile->type == tile_garbage) { 
         if (tile->mesh->texture == nullptr) {  //debug for garbage
            continue; 
         }
         tileDraw(board, tile);
      }
   }
   //Does Cursor rendering belong here? It's part of the board...
   cursorDraw(board);

   //debug basic frame
   drawMesh(board->game, board->frame, 0, 0, board->tileWidth * board->game->bWidth, board->tileHeight * board->game->bHeight);
}

//-----Helpful functions----------
//todo: Maybe put these somewhere else later
int yPosToRow(Board* board, float y) {
   return (int)(y - board->offset) / board->tileHeight + board->startH;
}

int xPosToCol(Board* board, float x) {
   return (int)x / board->tileWidth;
}

std::vector <Tile*> boardGetCol(Board* board, int col) {
   std::vector <Tile*> tiles;
   for (int i = board->startH -1; i < board->endH; i++) {
      tiles.push_back(boardGetTile(board, i, col));
   }
   return tiles;
}

std::vector <Tile*> boardGetRow(Board* board, int row) {
   std::vector <Tile*> tiles;
   //Left to right because it doesn't matter
   for (int i = 0; i < board->w; i++) {
      tiles.push_back(boardGetTile(board, row, i));
   }
   return tiles;
}

//End Helper stuff------------------


//todo adjust it so you can't swap a tile under and push things up
void _swapTiles(Tile* tile1, Tile* tile2, bool pos = false, bool fall = false) {

   Tile tmp = *tile2;

   //basic swap
   tile2->type = tile1->type;
   tile2->mesh = tile1->mesh;

   tile1->type = tmp.type;
   tile1->mesh = tmp.mesh;

   if (pos) {  //swap positions
      tile2->ypos = tile1->ypos;
      tile2->xpos = tile1->xpos;

      tile1->ypos = tmp.ypos;
      tile1->xpos = tmp.xpos;
   }

   if (fall) {  //swap and maintain falling status
      tile1->falling = tmp.falling;

      tile2->falling = tile1->falling;
   }
}

void boardSwap(Board* board) {

   //todo add logic for falling blocks...

   int col = xPosToCol(board, cursorGetX(board->cursor));
   int row = yPosToRow(board, cursorGetY(board->cursor));

   Tile* tile1 = boardGetTile(board, row, col);
   Tile* tile2 = boardGetTile(board, row, col + 1);

   if (tile1->type == tile_garbage || tile2->type == tile_garbage) { return; }    //Don't swap garbage
   if (tile1->type == tile_cleared || tile2->type == tile_cleared) { return; }    //Don't swap clears

   if (tile1->type == tile_empty) {
      Tile* above = boardGetTile(board, row - 1, col);
      if (above && above->type != tile_empty && above->type != tile_garbage && above->ypos + board->tileHeight - 0.01 >= tile1->ypos) {
         tile2 = above;
      }
   }

   if (tile2->type == tile_empty) {
      Tile* above = boardGetTile(board, row - 1, col + 1);
      if (above && above->type != tile_empty && above->type != tile_garbage && above->ypos + board->tileHeight - 0.01 >= tile2->ypos) {
         tile1 = above;
      }
   }

   _swapTiles(tile1, tile2);

   std::vector <Tile*> tiles = { tile1, tile2 };
   boardCheckClear(board, tiles, false);

   return;
}

void _checkClear(std::vector <Tile*> tiles, std::vector <Tile*> &matches) {
   int current = 0;

   while (current + 2 < tiles.size()) {
      Tile* t1 = tiles[current];
      Tile* t2 = tiles[current + 1];
      Tile* t3 = tiles[current + 2];

      if (t1->falling || t2->falling || t3->falling) {  // if it's falling, don't match it
         current++;
         continue;
      }

      if (t1->type != tile_empty && t1->type != tile_cleared && t1->type != tile_garbage) {
            if (t1->type == t2->type && t1->type == t3->type) {
               //We have a match... add to match list and move counter ahead looking for more
               matches.push_back(t1);
               matches.push_back(t2);
               matches.push_back(t3);

               current = current + 3;
               while (current < tiles.size()) {  //keep matching
                  t1 = tiles[current];
                  if (t1->type == tiles[current -1]->type) {
                     matches.push_back(t1);
                     current++;
                  }
                  else {
                     current++;
                     break;
                  }
               }
            }
      }
      current++;
   }
}

void boardCheckClear(Board* board, std::vector <Tile*> tileList, bool fallCombo) {
   std::vector <Tile*> matches;

   for (auto&& tile : tileList) {
      std::vector <Tile*> cols = boardGetCol(board, xPosToCol(board, tile->xpos));
      std::vector <Tile*> rows = boardGetRow(board, yPosToRow(board, tile->ypos));

      _checkClear(cols, matches);
      _checkClear(rows, matches);
   }

   if (matches.size() > 0) {
      int clearTime = SDL_GetTicks();
      for (auto&& m : matches) {
         //clear block and set timer
         m->mesh->texture = resourcesGetTexture(board->game->resources, Texture_cleared); 
         m->type = tile_cleared;
         m->clearTime = clearTime;
         m->falling = false;
         board->paused = true;
         board->pauseLength = 3000;
         if (fallCombo && m->chain == true) {
            board->combo += 1;
            fallCombo = false;

            if (board->combo > 1) {  //debug
               //printf("combo count: %d\n", board->combo); //debug
               //todo add some score stuff here?
            }
         }
         m->chain = false;

         //todo add score logic here
      }
   }
}

void boardUpdateFalling(Board* board, float velocity) {
   std::vector <Tile*> tilesToCheck;
   float drop = 1.0f * velocity;

   for (int col = 0; col < board->w; col++) {
      for (int row = board->wBuffer - 1; row >= 0; row--) {
         Tile* tile = boardGetTile(board, row, col);
         tile->falling = false;

         if (tile->type == tile_empty || tile->type == tile_cleared) {  //Don't update empty or cleared tiles
            continue;
         }
         if (row >= board->wBuffer - 1) { //skip the bottom row
            tile->falling = false;
            continue;
         }

         if (tile->type == tile_garbage) {
            //todo add garbage logic here
            continue;
         }

         Tile* below = boardGetTile(board, row + 1, col);

         if (below->type == tile_empty || below->falling == true) {
            tile->falling = true;
            if (tile->ypos + board->tileHeight + drop >= below->ypos && below->falling == true) {  //snap to tile's edge if drop is too much
               tile->ypos = below->ypos - board->tileHeight;
            }
            else { tile->ypos += drop; }  //keep falling
         }

         else if (below->falling == false) {
            if (tile->ypos + board->tileHeight + drop >= below->ypos) {  //if the below tile is not falling, stop at it's edge
               if (below->type == tile_cleared || below->chain == true) { tile->chain = true; }  //If we're stopped by a clear, everything above could be a combo
               tile->ypos = below->ypos - board->tileHeight;
               tile->falling = false;
               tilesToCheck.push_back(tile);
            }
            else {  //fall towards stopped tile
               tile->ypos += drop;
               tile->falling = true;
            }
         }

         else {
            tile->falling = false;
            tile->chain = false;  //Not falling so stop chaining
         }
      }
   }
   if (tilesToCheck.size() > 0) {
      boardCheckClear(board, tilesToCheck, true);
   }
}

void boardRemoveClears(Board* board) {
   int pauseTime = 0;
   int current = SDL_GetTicks();
   for (int row = board->endH -1; row >= board->startH; row--) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_cleared) {
            if (tile->clearTime + 2000 <= current) {
               tile->type = tile_empty;
               tile->mesh->texture = nullptr; //board->game->textures[6];
               //todo flag all blocks above as part of a chain
            }
         }
      }
   }
   return;
}

void boardMoveUp(Board* board, float height) {
   float nudge = height;
   board->offset -= nudge;
   bool newRow = false;

   cursorSetY(board->cursor, cursorGetY(board->cursor) - nudge);

   if (board->offset <= -1 * board->tileHeight) {
      board->offset += board->tileHeight;
      newRow = true;
   }

   std::vector <Tile*> checkTiles;
   for (int row = 0; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);

         if (tile->type == tile_empty) { continue; }  //don't move up empty blocks

         tile->chain = false;  //Whenever the board is moving, the combo is over?
         board->combo = 1;

         if (tile->ypos <= 0.0f && tile->type != tile_empty) { board->bust = false; }  //todo put some logic here

         if (row == board->endH - 1) { checkTiles.push_back(tile); }  //Check the bottom row for clears

         if (tile->falling == false) {  //Only nudge up blocks that aren't falling
            tile->ypos -= nudge;
         }
      }
   }

   boardCheckClear(board, checkTiles, false);

   boardUpdateArray(board, newRow);
}

int boardFillTiles(Board* board) {
   //todo: Might just have pre-made boards

   for (int row = 0; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (row < board->startH + (board->endH - board->startH) / 2) {
            tileInit(board, tile, row, col, tile_empty, true);
            continue;
         }
         else {  
            int current = board->distribution(board->generator);
            TileEnum type = (TileEnum)current;

            //make sure we don't create any matches on startup
            Tile* left = boardGetTile(board, row, col - 1);
            Tile* left2 = boardGetTile(board, row, col - 2);
            Tile* up = boardGetTile(board, row - 1, col);
            Tile* up2 = boardGetTile(board, row - 2, col);

            int total = 6;
            while ((type == left->type && type == left2->type) || (type == up->type && type == up2->type) ) {
               current++;
               if (current > total) {
                  current = current % total;
               }
               type = (TileEnum)current;
            }
            if (col % 2 == 0) {
               tileInit(board, tile, row - board->startH, col, type, true);
            }
            else {
               tileInit(board, tile, row - board->startH + 1, col, type, true);
            }
         }
      }
   }
   
   return 0;
}

void boardUpdateArray(Board* board, bool buffer = false) {

   std::vector <Tile> tileList;

   for (int row = 0; row < board->wBuffer; row++) {  //Loop through all the tiles and save them in a vector
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type != tile_empty) { 
            tileList.push_back(*tile);
         }
         tileInit(board, tile, row, col, tile_empty);  //Set each tile in the array to empty in the starting position
      }
   }

   for (auto&& t : tileList) {  //Take all the tiles and write them back into the array, adjusted for xy position
      int row = (t.ypos + board->tileHeight - 0.01f) / board->tileHeight + board->startH;  //Moving up triggers on last pixel, down on first
      int col = t.xpos / board->tileWidth;

      Tile* current = boardGetTile(board, row, col);
      if (current->type != tile_empty) {
         printf("Two tiles are being written to the same place in the array");  //todo if you got here, it's two tiles writing to the same place... what to do?
      }
      t.mesh = current->mesh;
      *current = t;
      tileSetTexture(board, current);

      if (current->type == tile_garbage && current->garbage != nullptr) {
         current->garbage->start = current;
      }
   }

   //std::vector <Tile*> checkTiles;
   for (int col = 0; col < board->w; col++) {  //Finally, check if the buffer row is empty and fill it
      int row = board->wBuffer - 1;
      Tile* current = boardGetTile(board, row, col);
      if (current->type == tile_empty) {
         tileInit(board, current, row, col, (TileEnum)board->distribution(board->generator));
         current->ypos += board->offset;
         //checkTiles.push_back(boardGetTile(board, row - 1, col));  //Check the new row above for clears
      }
   }
   //boardCheckClear(board, checkTiles, false);
}

std::vector <Tile> boardDebug(Board* board) {
   std::vector <Tile> tileList;

   for (int row = 0; row < board->wBuffer; row++) {  //Loop through all the tiles and save them in a vector
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         tileList.push_back(*tile);
      }
   }
   return tileList;
}