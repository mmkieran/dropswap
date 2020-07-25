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

         board->tileHeight = game->tHeight;
         board->tileWidth = game->tWidth;

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

   for (auto&& pair : board->garbage) {
      garbageDestroy(pair.second);
   }

   free(board->tiles);
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
   for (int row = board->startH; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         //if (tile->mesh->texture == nullptr || tile->type == tile_garbage) { 
         if (tile->mesh->texture == nullptr) {  //debug for garbage
            continue; 
         }
         tileDraw(board, tile);
      }
   }
   cursorDraw(board);

   //debug basic frame
   drawMesh(board->game, board->frame, board->origin.x, board->origin.y, board->tileWidth * board->game->bWidth, board->tileHeight * board->game->bHeight);
}

//Calculates the row based on the pointer difference in the array
int tileGetRow(Board* board, Tile* tile) {
   int out = (tile - board->tiles) / board->w;
   return out;
}

//Calculates the col based on the pointer difference in the array
int tileGetCol(Board* board, Tile* tile) {
   return (tile - board->tiles) % board->w;
}

std::vector <Tile*> boardGetAllTilesInCol(Board* board, int col) {
   std::vector <Tile*> tiles;
   for (int i = board->startH -1; i < board->endH; i++) {
      tiles.push_back(boardGetTile(board, i, col));
   }
   return tiles;
}

std::vector <Tile*> boardGetAllTilesInRow(Board* board, int row) {
   std::vector <Tile*> tiles;
   //Left to right because it doesn't matter
   for (int i = 0; i < board->w; i++) {
      tiles.push_back(boardGetTile(board, row, i));
   }
   return tiles;
}

static void _swapTiles(Tile* tile1, Tile* tile2) {

   Tile tmp = *tile2;

   //basic swap
   tile2->type = tile1->type;
   tile2->mesh = tile1->mesh;

   tile1->type = tmp.type;
   tile1->mesh = tmp.mesh;
}

void boardSwap(Board* board) {

   //todo add logic for falling blocks...

   float xCursor = cursorGetX(board->cursor);
   float yCursor = cursorGetY(board->cursor);

   int col = cursorGetCol(board);
   int row = cursorGetRow(board);

   Tile* tile1 = boardGetTile(board, row, col);
   Tile* tile2 = boardGetTile(board, row, col + 1);

   Tile* below1 = boardGetTile(board, row + 1, col);
   Tile* below2 = boardGetTile(board, row + 1, col + 1);

   if (tile1->type == tile_garbage || tile2->type == tile_garbage) { return; }    //Don't swap garbage
   if (tile1->type == tile_cleared || tile2->type == tile_cleared) { return; }    //Don't swap clears
   if (tile1->status == status_disable || tile2->status == status_disable) { return; }    //Don't swap disabled tiles

   if (tile1->type == tile_empty && tile2->type != tile_empty) {  
      if (tile2->falling = true && tile2->ypos > yCursor + 1) {  //Don't swap non-empty if it's already falling below
         return;
      }
      else {
         _swapTiles(tile1, tile2);
         tile1->ypos = tile2->ypos;  //When swapping an empty tile, maintain ypos
      }
   }
   else if (tile2->type == tile_empty && tile1->type != tile_empty) {
      if (tile1->falling = true && tile1->ypos > yCursor + 1) {  //Don't swap non-empty if it's already falling below
         return;
      }
      else { 
         _swapTiles(tile1, tile2);
         tile2->ypos = tile1->ypos;  //When swapping an empty tile, maintain ypos
      }
   }
   else {
      _swapTiles(tile1, tile2);
   }

   std::vector <Tile*> tiles;
   //Check if after we swapped them, either tile is falling... these don't get cleared
   if (below1 && (below1->type == tile_empty || below1->falling == true)) {
      tile1->falling = true;
   }
   else { tiles.push_back(tile2); }
   if (below2 && (below2->type == tile_empty || below2->falling == true)) {
      tile2->falling = true;
   }
   else { tiles.push_back(tile1); }

   boardCheckClear(board, tiles, false);

   return;
}

static void _checkClear(std::vector <Tile*> tiles, std::vector <Tile*> &matches) {
   int current = 0;

   while (current + 2 < tiles.size()) {
      Tile* t1 = tiles[current];
      Tile* t2 = tiles[current + 1];
      Tile* t3 = tiles[current + 2];

      if (t1->falling || t2->falling || t3->falling) {  // if it's falling, don't match it
         current++;
         continue;
      }

      if (t1->status == status_disable || t2->status == status_disable || t3->status == status_disable) {  // if it's disabled, don't match it
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
      std::vector <Tile*> cols = boardGetAllTilesInCol(board, tileGetCol(board, tile));
      std::vector <Tile*> rows = boardGetAllTilesInRow(board, tileGetRow(board, tile));

      _checkClear(cols, matches);
      _checkClear(rows, matches);
   }

   if (matches.size() > 0) {
      int clearTime = SDL_GetTicks();
      for (auto&& m : matches) {

         garbageCheckClear(board, m);
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
         }
         m->chain = false;

         //todo add score logic here
      }
   }
}

void boardUpdateFalling(Board* board, float velocity) {
   std::vector <Tile*> tilesToCheck;
   float drop = board->level * velocity;

   for (int col = 0; col < board->w; col++) {
      for (int row = board->wBuffer - 1; row >= 0; row--) {
         Tile* tile = boardGetTile(board, row, col);
         tile->falling = false;

         if (tile->type == tile_empty || tile->type == tile_cleared) {  //Don't update empty or cleared tiles
            continue;
         }
         if (tile->status == status_disable) { continue; }  //Don't fall if disabled

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

static TileType _tileGenType(Board* board, Tile* tile) {
   int current = board->distribution(board->generator);
   TileType type = (TileType)current;

   int row = tileGetRow(board, tile);
   int col = tileGetCol(board, tile);

   //make sure we don't create any matches in new tiles
   Tile* left = boardGetTile(board, row, col - 1);
   Tile* left2 = boardGetTile(board, row, col - 2);
   Tile* up = boardGetTile(board, row - 1, col);
   Tile* up2 = boardGetTile(board, row - 2, col);

   int total = 6;
   while ((type == left->type && type == left2->type) || (type == up->type && type == up2->type)) {
      current++;
      if (current > total) {
         current = current % total;
      }
      type = (TileType)current;
   }
   return type;
}

void boardRemoveClears(Board* board) {
   int pauseTime = 0;
   int current = SDL_GetTicks();
   for (int row = board->endH -1; row >= board->startH; row--) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_cleared) {
            if (tile->idGarbage >= 0 && tile->clearTime <= current) {

               tile->type = _tileGenType(board, tile);
               tileSetTexture(board, tile);

               tile->falling = true;
               tile->chain = true;
               tile->idGarbage = -1;
            }

            else if (tile->clearTime + 2000 <= current) {
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

         tile->ypos -= nudge;  //debug let's try moving all blocks
         //if (tile->falling == false) {  //Only nudge up blocks that aren't falling
         //   tile->ypos -= nudge;
         //}
      }
   }

   boardCheckClear(board, checkTiles, false);

   //boardUpdateArray(board, newRow);
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
         
         TileType type = _tileGenType(board, tile);
         if (col % 2 == 0) {
            tileInit(board, tile, row - board->startH, col, type, true);
         }
         else {
            tileInit(board, tile, row - board->startH + 1, col, type, true);
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

   std::vector <Tile> conflicts;
   for (auto&& t : tileList) {  //Take all the tiles and write them back into the array, adjusted for xy position
      int row = (t.ypos + board->tileHeight - 0.00001) / board->tileHeight + board->startH;  //Moving up triggers on last pixel, down on first
      int col = t.xpos / board->tileWidth;

      Tile* current = boardGetTile(board, row, col);
      if (current->type != tile_empty) {
         printf("Two tiles are being written to the same place in the array\n");  //todo if you got here, it's two tiles writing to the same place... what to do?
         conflicts.push_back(t);
         continue;
      }
      t.mesh = current->mesh;
      *current = t;
      tileSetTexture(board, current);

      if (current->type == tile_garbage && current->garbage != nullptr) {  //if the start tile moves, we need to tell the garbage
         garbageSetStart(board, current);
      }
   }

   //todo should be smarter about giving information about the tile it conflicted with (y position especially)
   for (auto&& conflict : conflicts) {
      //try our best
      int row = (conflict.ypos + board->tileHeight - 0.00001) / board->tileHeight + board->startH;  //Moving up triggers on last pixel, down on first
      int col = conflict.xpos / board->tileWidth;

      Tile* left = boardGetTile(board, row, col - 1);
      Tile* right = boardGetTile(board, row, col + 1);
      Tile* up = boardGetTile(board, row - 1, col);
      Tile* down = boardGetTile(board, row + 1, col);

      Tile* tiles[4] = { up, down, left, right };

      for (int i = 0; i < 4; i++) {
         if (tiles[i] && tiles[i]->type == tile_empty) {
            destroyMesh(conflict.mesh);
            conflict.mesh = tiles[i]->mesh;
            *tiles[i] = conflict;
            tileSetTexture(board, tiles[i]);
            break;
         }
      }
   }

   //std::vector <Tile*> checkTiles;
   for (int col = 0; col < board->w; col++) {  //Finally, check if the buffer row is empty and fill it
      int row = board->wBuffer - 1;
      Tile* current = boardGetTile(board, row, col);
      if (current->type == tile_empty) {
         tileInit(board, current, row, col, (TileType)board->distribution(board->generator));
         current->ypos += board->offset;
         //checkTiles.push_back(boardGetTile(board, row - 1, col));  //Check the new row above for clears
      }
   }
   //boardCheckClear(board, checkTiles, false);
}

//This is copied from Fill Tiles
void makeItRain(Board* board) {
   int row = 0;
   for (int col = 0; col < board->w; col++) {
      Tile* tile = boardGetTile(board, row, col);
      if (tile->type != tile_empty) { continue; }

      int current = board->distribution(board->generator);
      TileType type = (TileType)current;

      //make sure we don't create any matches on startup
      Tile* left = boardGetTile(board, row, col - 1);
      Tile* left2 = boardGetTile(board, row, col - 2);

      int total = 6;
      while ((type == left->type && type == left2->type) ) {
         current++;
         if (current > total) {
            current = current % total;
         }
         type = (TileType)current;
      }
      if (col % 2 == 0) {
         tileInit(board, tile, row, col, type, true);
      }
      else {
         tileInit(board, tile, row + 1, col, type, true);
      }
   }
}

void boardClear(Board* board) {
   for (auto&& pair : board->garbage) {
      garbageDestroy(pair.second);
   }
   board->garbage.clear();

   for (int row = 0; row < board->wBuffer; row++) {  //Loop through all the tiles and save them in a vector
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         tileInit(board, tile, row, col, tile_empty);
         tile->idGarbage = -1;
         tile->garbage = nullptr;
      }
   }
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