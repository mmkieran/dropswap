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

   //todo add logic for falling blocks...

   int col = posXToCol(board, cursor->GetXPosition());
   int row = posYToRow(board, cursor->GetYPosition());

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
      std::vector <Tile*> cols = boardGetCol(board, posXToCol(board, tile->xpos));
      std::vector <Tile*> rows = boardGetRow(board, posYToRow(board, tile->ypos));

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
   for (int col = 0; col < board->w; col++) {
      for (int row = board->endH -1; row >= 0; row--) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_empty || tile->type == tile_cleared) {
            continue;
         }
         if (row >= board->endH - 1) { //skip the bottom row
            tile->falling = false; 
            continue;
         }

         //todo: add garbage falling logic here

         Tile* below = boardGetTile(board, row + 1, col);

         if (below->type == tile_empty || below->falling == true) {  //If below is empty or falling start falling
            tile->falling = true;
            int drop = 1 * velocity;


            if (below->type == tile_empty){
               if (tile->ypos + drop >= below->ypos) {  //We halfway overlap an empty tile
                  int diff = tile->ypos + drop - below->ypos;
                  _swapTiles(tile, below);  //swap the empty tile up
                  below->ypos += diff;  //continue falling
                  below->falling = true;
                  tile->ypos = below->ypos - board->tileHeight;  
                  
                  if (row + 2 <= board->endH - 1) {  //is there another tile below this
                     tile = below;  //Now that they are swapped, check if it is still falling
                     below = boardGetTile(board, row + 2, col);
                     if (below->type == tile_empty || below->falling == true) { tile->falling = true; }
                     else {tile->falling = false;}
                  }
               }
               else {
                  tile->ypos += drop;
               }
            }
            else if (below->falling == false) {
               if (tile->ypos + board->tileHeight + drop >= below->ypos) {  //If the below tile is not falling, stop at it's edge
                  tile->ypos = below->ypos - board->tileHeight;
                  tile->falling = false;
                  tilesToCheck.push_back(tile);
               }
               else {
                  tile->ypos += drop;
               }
            }
            else if (below->falling == true) {  //Fall with your friends
               tile->ypos += drop;
            }
         }
         else if (below->type == tile_cleared || below->chain == true) {  //if it stops on a clear, it's potentially a chain
            tile->chain = true;
            tile->falling = false;
         }
         else { 
            tile->falling = false;
            tilesToCheck.push_back(tile);
         }
         tileUpdate(board, tile);
      }
   }
   if (tilesToCheck.size() > 0) {
      boardCheckClear(board, tilesToCheck, true);
   }
}


//void boardUpdateFalling(Board* board) {
//   std::vector <Tile*> tilesToCheck;
//   for (int col = 0; col < board->w; col++) {
//      for (int row = board->endH - 2; row >= 0; row--) {
//         Tile* tile = boardGetTile(board, row, col);
//         //tile->falling = false;
//
//         if (tile->type == tile_empty || tile->type == tile_cleared)  {
//            continue;
//         }
//         //todo: add garbage falling logic here
//
//         Tile* below = boardGetTile(board, row + 1, col);
//         if (below->type == tile_empty) {  //nothing below or it's falling too
//            tile->falling = true;
//            _swapTiles(tile, below, false, true); 
//         }
//         else if (below->type == tile_cleared) {  //flag all tiles above the clear as potentially part of a chain
//            tile->falling = true;
//
//            for (int i = row; i >= 0; i--) {  //flag all tiles above the clear as potentially part of a chain
//               Tile* above = boardGetTile(board, i, col);
//               tile->chain = true;
//            }
//         }
//         else {  //Regular tile below it
//            tile->falling = false;
//            tilesToCheck.push_back(tile);
//         }
//         tile = boardGetTile(board, row, col);
//         if (tile->falling = false) { tile->chain = false; }  //If at the end the block is not falling, then it's not part of a chain
//      }
//   }
//   if (tilesToCheck.size() > 0) {
//      boardCheckClear(board, tilesToCheck, true);
//   }
//}

void boardRemoveClears(Board* board) {
   int pauseTime = 0;
   int current = SDL_GetTicks();
   for (int row = board->endH -1; row >= board->startH; row--) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_cleared) {
            if (tile->clearTime + 2000 <= current) {
               tile->type = tile_empty;
               tile->texture = nullptr;
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

         if (board->combo > 0) {
            printf("combo count: %d", board->combo);
         }
         tile->chain = false;  //Whenever the board is moving, the combo is over
         board->combo = 0;

         if (tile->ypos <= 0 && tile->type != tile_empty) { board->bust = false; }  //todo put some logic here

         if (updateArray) {

            if (row == board->wBuffer - 1) {
               tile->type = (TileEnum)board->distribution(board->generator);   //create new tiles here
               tileLoadTexture(board, tile);
               tile->ypos = (row - board->startH) * board->tileHeight;  //push buffer layer one tile back down below frame
               tile->falling = false;

            }
            else {  //swap tiles that aren't at the bottom
               //todo make a universal swap function
               Tile* below = boardGetTile(board, row + 1, col);
               if (below->falling == true) {  
                  tile->falling = below->falling;
                  tile->chain = below->chain;
               }
               //todo fix moving board up and falling down
               tile->type = below->type;
               tile->texture = below->texture;
               tile->ypos = (row - board->startH) * board->tileHeight;  //push non-falling tiles down one tile after swap

            }

            if (tile->falling == false) {
               tile->ypos += board->offset;  //add extra offset if more than one tile
            }

            if (row == board->endH - 1) { checkTiles.push_back(tile); }  //check for clears in the new bottom row
         }
         else {  //Move the board up, no need to adjust array
            if (tile->falling == false) {
               tile->ypos -= nudge;
            }
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