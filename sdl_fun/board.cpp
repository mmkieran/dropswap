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
int posYToRow(Board* board, int y) {
   return (y - board->offset) / board->tileHeight + board->startH;
}

int posXToCol(Board* board, int x) {
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
   int col = posXToCol(board, cursor->GetXPosition());
   int row = posYToRow(board, cursor->GetYPosition());

   Tile* tile1 = boardGetTile(board, row, col);
   Tile* tile2 = boardGetTile(board, row, col + 1);

   if (tile1->type == tile_garbage || tile2->type == tile_garbage) { return; }    //Don't swap garbage
   if (tile1->type == tile_cleared || tile2->type == tile_cleared) { return; }

   _swapTiles(tile1, tile2);

   std::vector <Tile*> tiles = { tile1, tile2 };
   boardCheckClear(board, tiles, false);

   return;
}

void _checkClear(std::vector <Tile*> tiles, std::vector <Tile*> &matches) {
   int current = 0;

   while (current < tiles.size()) {
      if (tiles[current]->type != tile_empty && tiles[current]->type != tile_cleared && tiles[current]->type != tile_garbage) {
         if (current + 2 < tiles.size()) {
            if (tiles[current]->type == tiles[current + 1]->type && tiles[current]->type == tiles[current + 2]->type) {
               //We have a match... add to match list and move counter ahead looking for more
               matches.push_back(tiles[current] );
               matches.push_back(tiles[current + 1] );
               matches.push_back(tiles[current + 2] );

               current = current + 3;
               while (current < tiles.size()) {  //keep matching
                  if (tiles[current]->type == matches[0]->type) {
                     matches.push_back(tiles[current]);
                  }
                  current++;
               }
            }
         }
      }
      current++;
   }
}

std::vector <Tile*> _uniqueVector(std::vector <Tile*> tileList) {
   std::vector <Tile*> unique;
   for (int i = 0; i < tileList.size(); i++) {
      for (int j = 0; j < unique.size(); j++) {
         if (tileList[i] == unique[j]) { continue; }
      }
   }
   return unique;
}


void boardCheckClear(Board* board, std::vector <Tile*> tileList, bool fallCombo) {
   std::vector <Tile*> matches;

   for (auto&& tile : tileList) {
      std::vector <Tile*> cols = boardGetCol(board, posXToCol(board, tile->xpos));
      std::vector <Tile*> rows = boardGetRow(board, posYToRow(board, tile->ypos));

      _checkClear(cols, matches);
      _checkClear(rows, matches);
   }

   //matches = _uniqueVector(matches);
   if (matches.size() > 0) {
      int clearTime = SDL_GetTicks();
      for (auto&& m : matches) {
         //clear block and set timer
         m->texture = board->game->textures[7];
         m->type = tile_cleared;
         m->clearTime = clearTime;
         board->paused = true;
         board->pauseLength = 2000;
         if (fallCombo && m->chain) {
            board->combo += 1;
            fallCombo = false;
         }

         //todo add score logic here
      }
   }
}

//void boardUpdateFalling(Board* board, int velocity) {
//   std::vector <Tile*> tilesToCheck;
//   for (int col = 0; col < board->w; col++) {
//      for (int row = board->endH -1; row >= 0; row--) {
//         Tile* tile = boardGetTile(board, row, col);
//         if (tile->type == tile_empty || tile->type == tile_cleared) {
//            continue;
//         }
//         if (row >= board->endH - 1) { //skip the bottom row
//            tile->falling = false; 
//            continue;
//         }
//
//         //todo: add garbage falling logic here
//
//         Tile* below = boardGetTile(board, row + 1, col);
//
//         if (below->type == tile_empty || below->falling == true) {  //If below is empty or falling start falling
//            tile->falling = true;
//            int drop = 8 * velocity;
//
//
//            if (below->type == tile_empty){
//               if (tile->ypos + drop >= below->ypos) {  //We nearly overlap an empty tile
//                  int diff = tile->ypos + drop - below->ypos;
//                  _swapTiles(tile, below);  //swap the empty tile up
//                  below->ypos += diff;  //continue falling
//                  tile->ypos = below->ypos - board->tileHeight;
//               }
//               else {
//                  tile->ypos += drop;
//               }
//            }
//            else if (below->falling == false) {
//               if (tile->ypos + board->tileHeight + drop >= below->ypos) {  //If the below tile is not falling, stop at it's edge
//                  tile->ypos = below->ypos - board->tileHeight;
//                  tile->falling = false;
//                  tilesToCheck.push_back(tile);
//               }
//               else {
//                  tile->ypos += drop;
//               }
//            }
//            else if (below->falling == true) {  //Fall with your friends
//               tile->ypos += drop;
//            }
//         }
//         else if (below->type == tile_cleared || below->chain == true) {  //if it stops on a clear, it's a chain
//            tile->chain = true;
//            tile->falling = false;
//         }
//         else {
//            tile->falling = false;
//            tilesToCheck.push_back(tile);
//         }
//         tileUpdate(board, tile);
//      }
//   }
//   if (tilesToCheck.size() > 0) {
//      boardCheckClear(board, tilesToCheck, true);
//   }
//}


void boardUpdateFalling(Board* board) {
   std::vector <Tile*> tilesToCheck;
   for (int row = board->endH - 2; row >= 0; row--) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_empty || tile->type == tile_cleared)  {
            continue;
         }
         //todo: add garbage falling logic here

         Tile* below = boardGetTile(board, row + 1, col);
         if (below->type == tile_empty || below->falling == true) {

            _swapTiles(tile, below); 

            tile = below;  //now that they are swapped, check if it is still falling
            if (row + 1 == board->endH - 1) {  //hit the bottom
               tile->falling = false;
               tilesToCheck.push_back(tile);
            }
            else {  //not at the bottom
               below = boardGetTile(board, row + 2, col);
               if (below->type == tile_empty || below->falling == true) {
                  tile->falling = true;
               }
               else {
                  tile->falling = false;
                  tilesToCheck.push_back(tile);
               }
            }
         }
         else {
            tile->falling = false;
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
         std::vector <Tile*> debug;
         Tile* tile = boardGetTile(board, row, col);
         debug.push_back(tile);
         if (tile->type == tile_cleared) {
            if (tile->clearTime + 2000 <= current) {
               tile->type = tile_empty;
               tile->texture = nullptr;
               tile->chain = true;
            }
         }
      }
   }
   return;
}

void boardMoveUp(Board* board, int height) {
   int nudge = height;
   bool updateArray = false;
   board->offset -= nudge;

   if (board->offset <= -1 * board->tileHeight) {
      board->offset += board->tileHeight;
      updateArray = true;
   } 

   std::vector <Tile*> checkTiles;
   for (int row = board->startH -1; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);

         if (tile->ypos <= 0 && tile->type != tile_empty) { board->bust = false; }  //todo put some logic here

         if (updateArray) {

            if (row == board->wBuffer - 1) {
               tile->type = (TileEnum)board->distribution(board->generator);   //create new tiles here
               tileLoadTexture(board, tile);
               tile->ypos = (row - board->startH) * board->tileHeight;  //push buffer layer one tile back down below frame
               tile->falling = false;
               tile->chain = 0;

            }
            else {  //swap tiles that aren't at the bottom
               //todo make a universal swap function
               Tile* below = boardGetTile(board, row + 1, col);
               if (below->falling == true) {  
                  tile->falling = below->falling;
                  tile->ypos = below->ypos;
                  tile->chain = below->chain;
               }
               tile->type = below->type;
               tile->texture = below->texture;
               tile->ypos = (row - board->startH) * board->tileHeight;
            }

            tile->ypos += board->offset;
            if (row == board->endH - 1) { checkTiles.push_back(tile); }
         }
         else {
            tile->ypos -= nudge;
         }
         tileUpdate(board, tile);

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
      std::vector <Tile*> cols = boardGetCol(board, posXToCol(board, ct->xpos));
      std::vector <Tile*> rows = boardGetRow(board, posYToRow(board, ct->ypos));

      _checkClear(cols, matches);
      _checkClear(rows, matches);
   }

   if (matches.size() > 0) {
      for (auto&& m : matches) {
         m->texture = nullptr;
         m->type = tile_empty;
      }
   }
   
   return 0;
}

void boardResetChain(Board* board) {
   bool contCombo = false;

   for (int col = 0; col < board->w; col++) {
      for (int row = board->endH - 1; row >= 0; row--) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_cleared) {
            for (int i = row; i >= board->startH; i--) {
               tile->chain = true;
               contCombo = true;
            }
            break;
         }
      }
   }

   if (!contCombo) {
      board->combo = 0;
   }
   else {
      printf("combo count: %d", board->combo);
   }
}