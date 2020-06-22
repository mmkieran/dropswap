#include <vector>
#include <time.h>
#include <chrono>

#include "board.h"
#include "texture_manager.h"
#include "tile.h"
#include "cursor.h"

void _checkClear(std::vector <Tile*> tiles, std::vector <Tile*> &matches);
void boardCheckClear(Board* board, std::vector <Tile*> tileList, bool fallCombo);

Tile* _boardCreateArray(int width, int height) {
   Tile* tiles = (Tile*)malloc(sizeof(Tile) * (height * 2 + 1) * width);
   //memset(tiles, 0, sizeof(Tile) * (height * 2 + 1) * width);
   return tiles;
}

Board* boardCreate(Game* game) {
   Board* board = (Board*)malloc(sizeof(Board));
   if (board) {
      Tile* tiles = _boardCreateArray(game->bHeight, game->bWidth);
      if (tiles) {
         board->tiles = tiles;

         board->startH = game->bHeight;  //start playing area of the board... everything above is for garbage
         board->endH = game->bHeight * 2;  //end playing area of the board
         board->wBuffer = board->endH + 1;  //Extra row upcoming rows
         board->w = game->bWidth;
         board->offset = 0;

         board->tileHeight = game->tHeight;
         board->tileWidth = game->tWidth;
         board->speed = 1;
         board->moveTimer = 0;
         board->fallTimer = 0;
         board->paused = false;
         board->pauseLength = 0;
         board->bust = false;
         board->score = 0;
         board->combo = 0;

         std::default_random_engine gen(time(0));
         board->generator = gen;

         std::uniform_int_distribution<int> dist (1, 6);
         board->distribution = dist;

         return board;
      }
   }
   return nullptr;
}

void boardDestroy(Board* board) {
   free(board->tiles);
   free(board);
}

Tile* boardGetTile(Board* board, int row, int col) {
   Tile* tile = &board->tiles[(board->w * row + col)];
   return tile;
}

void boardSetTile(Board* board, Tile tile, int row, int col) {
   board->tiles[(board->w * row + col)] = tile;
}

void boardRender(Game* game, Board* board) {
   for (int row = board->startH - 1; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->texture == nullptr) { continue; }
         SDL_RenderCopy(game->renderer, tile->texture, &tile->srcRect, &tile->destRect);
      }
   }
   //Does Cursor rendering belong here? It's part of the board
   game->board->cursor->Render(game);
}

//-----Helpful functions----------
//todo: Maybe put these somewhere else later
int yPosToRow(Board* board, int y) {
   return (y - board->offset) / board->tileHeight + board->startH;
}

int xPosToCol(Board* board, int x) {
   return x / board->tileWidth;
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

//------------------

void _swapTiles(Tile* tile1, Tile* tile2, bool pos = false, bool fall = false) {
   Tile tmp = *tile2;

   //basic swap
   tile2->type = tile1->type;
   tile2->texture = tile1->texture;

   tile1->type = tmp.type;
   tile1->texture = tmp.texture;

   if (pos) {  //swap positions
      tile2->ypos = tile1->ypos;
      tile2->xpos = tile1->xpos;

      tile1->ypos = tile2->ypos;
      tile1->xpos = tile2->xpos;
   }

   if (fall) {  //swap and maintain falling status
      tile1->falling = tmp.falling;

      tile2->falling = tile1->falling;
   }
}

void boardSwap(Board* board, Cursor* cursor) {

   //todo add logic for falling blocks...

   int col = xPosToCol(board, cursor->GetXPosition());
   int row = yPosToRow(board, cursor->GetYPosition());

   Tile* tile1 = boardGetTile(board, row, col);
   Tile* tile2 = boardGetTile(board, row, col + 1);

   if (tile1->type == tile_garbage || tile2->type == tile_garbage) { return; }    //Don't swap garbage
   if (tile1->type == tile_cleared || tile2->type == tile_cleared) { return; }    //Don't swap clears

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
         m->texture = board->game->textures[7];
         m->type = tile_cleared;
         m->clearTime = clearTime;
         m->falling = false;
         board->paused = true;
         board->pauseLength = 3000;
         if (fallCombo && m->chain == true) {
              board->combo += 1;
            fallCombo = false;
         }

         //todo add score logic here
      }
   }
}

void boardUpdateFalling(Board* board, int velocity) {
   std::vector <Tile*> tilesToCheck;
   int drop = 1 * velocity;
   std::vector <Tile> debug = boardDebug(board);

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

         Tile* below = boardGetTile(board, row + 1, col);

         //Probably do a while loop here with the below tile type?
         if (below->type == tile_empty || below->falling == true) {
            tile->falling = true;
            if (tile->ypos + board->tileHeight + drop >= below->ypos && below->falling == true) {  //snap to tile's edge if drop is too much
               tile->ypos = below->ypos - board->tileHeight;
            }
            else { tile->ypos += drop; }
         }

         else if (below->falling == false) {
            if (tile->ypos + board->tileHeight + drop >= below->ypos) {  //if the below tile is not falling, stop at it's edge
               tile->ypos = below->ypos - board->tileHeight;
               tile->falling = false;
               tilesToCheck.push_back(tile);
            }
            else {
               tile->ypos += drop;
               tile->falling = true;
            }
         }

         else {
            tile->falling = false;
         }

         tileUpdate(board, tile);
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
               tile->texture = nullptr; //board->game->textures[6];
            }
         }
      }
   }
   return;
}

void boardMoveUp(Board* board, int height) {
   int nudge = height;
   board->offset -= nudge;

   if (board->offset <= -1 * board->tileHeight) {
      board->offset += board->tileHeight;
   }

   std::vector <Tile*> checkTiles;
   for (int row = 0; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);

         if (tile->type == tile_empty) { continue; }  //don't move up empty blocks

         if (board->combo > 0) {  //debug
            printf("combo count: %d", board->combo);
         }

         tile->chain = false;  //Whenever the board is moving, the combo is over?
         board->combo = 0;

         if (tile->ypos <= 0 && tile->type != tile_empty) { board->bust = false; }  //todo put some logic here

         if (row == board->endH - 1) { checkTiles.push_back(tile); }  //Check the bottom row for clears

         if (tile->falling == false) {  //Only nudge up blocks that aren't falling
            tile->ypos -= nudge;
            tileUpdate(board, tile);
         }
      }
   }

   board->cursor->SetYPosition(board->cursor->GetYPosition() - nudge);
   boardCheckClear(board, checkTiles, false);
}

int boardFillTiles(Board* board) {
   //todo: Might just have pre-made boards
   std::vector <Tile*> checkTiles;
   std::vector <Tile*> matches;

   for (int row = 0; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (row < board->startH + (board->endH - board->startH) / 2) {
            tileInitWithType(board, tile, row, col, tile_empty);
            continue;
         }
         else {  //todo: add a better algorithm here
            tileInitWithType(board, tile, row, col, (TileEnum)board->distribution(board->generator));
            if (row != board->endH) {
               checkTiles.push_back(tile);
            }
         }
      }
   }

   for (auto&& ct : checkTiles) {
      std::vector <Tile*> cols = boardGetCol(board, xPosToCol(board, ct->xpos));
      std::vector <Tile*> rows = boardGetRow(board, yPosToRow(board, ct->ypos));

      _checkClear(cols, matches);
      _checkClear(rows, matches);
   }

   if (matches.size() > 0) {
      for (auto&& m : matches) {
         m->texture = nullptr; //board->game->textures[6];  //debug texture
         m->type = tile_empty;
      }
   }
   
   return 0;
}

void boardUpdateArray(Board* board, bool buffer = false) {
   std::vector <Tile> tileList;

   for (int row = 0; row < board->wBuffer; row++) {  //Loop through all the tiles and save them in a vector
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_empty) { 
            continue;  
         }
         else {
            tileList.push_back(*tile);
         }
         tileInitWithType(board, tile, row, col, tile_empty);  //Set each tile in the array to empty in the starting position
      }
   }

   for (auto&& t : tileList) {  //Take all the tiles and write them back into the array, adjusted for xy position
      int row = (t.ypos + board->tileHeight - 1) / board->tileHeight + board->startH;  //Moving up triggers on last pixel, down on first
      int col = t.xpos / board->tileWidth;

      boardSetTile(board, t, row, col);
      tileUpdate(board, boardGetTile(board, row, col) );
   }

   std::vector <Tile*> checkTiles;
   for (int col = 0; col < board->w; col++) {  //Finally, check if the buffer row is empty and fill it
      int row = board->wBuffer - 1;
      Tile* current = boardGetTile(board, row, col);
      if (current->type == tile_empty) {
         Tile newTile;
         tileInitWithType(board, &newTile, row, col, (TileEnum)board->distribution(board->generator));   //create new tiles here
         newTile.ypos += board->offset;  //need to take board offset into account
         boardSetTile(board, newTile, row, col);
         tileUpdate(board, current);
         checkTiles.push_back(boardGetTile(board, row - 1, col));  //Check the new row above for clears
      }
   }
   boardCheckClear(board, checkTiles, false);
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