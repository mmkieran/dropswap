#include <vector>
#include <time.h>
#include <thread>
#include <chrono>

#include "board.h"
#include "texture_manager.h"
#include "tile.h"
#include "cursor.h"

void boardCheckClear(Board* board, std::vector <Tile*> tileList);

Tile* _boardCreateArray(int height, int width) {
   Tile* tiles = (Tile*)malloc(sizeof(Tile) * height * width);
   memset(tiles, 0, sizeof(Tile) * height * width);
   return tiles;
}

Board* boardCreate(Game* game) {
   Board* board = (Board*)malloc(sizeof(Board));
   if (board) {
      Tile* tiles = _boardCreateArray(game->bHeight, game->bWidth);
      if (tiles) {

         board->tiles = tiles;
         board->h = game->bHeight;
         board->w = game->bWidth;
         board->tileHeight = game->tHeight;
         board->tileWidth = game->tWidth;
         board->time = 0;
         board->speed = 1;
         board->bust = false;

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

int boardPrint(Board* board) {
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         printf("xpos: %d, ypos: %d, type: %d\n", tile->xpos, tile->ypos, tile->type);
      }
   }

   return 0;
}

int boardFillTiles(Board* board) {
   //todo: Might just have pre-made boards
   std::vector <Tile*> checkTiles;
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (row < 5) { 
            tileInitWithType(board, tile, row, col, tile_empty);
            continue; 
         }
         else {  //todo: add a better algorithm here
            tileInitWithType(board, tile, row, col, (TileEnum)board->distribution(board->generator));
            checkTiles.push_back(tile);
         }
      }
   }
   boardCheckClear(board, checkTiles); 
   return 0;
}

void boardUpdate(Board* board) {
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);

         tile->ypos -= 1;
         tile->destRect.y = tile->ypos;
      }
   }
}

void boardRender(Game* game, Board* board) {
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         SDL_RenderCopy(game->renderer, tile->texture, &tile->srcRect, &tile->destRect);
      }
   }
}

//-----Helpful functions----------
//todo: Maybe put these somewhere else later
int posYToRow(Board* board, int y) {
   int tile_height = board->tileHeight;
   if (y % tile_height != 0) {
      int remain = y % tile_height;
      return (y + (tile_height - remain)) / tile_height;
   }
   return y / tile_height;
}

int posXToCol(Board* board, int x) {
   return x / board->tileWidth;
}

std::vector <Tile*> boardGetCol(Board* board, int col) {
   std::vector <Tile*> tiles;
   //We want to go bottom to top (This will allow us to tag blocks above clears for falling)
   for (int i = 0; i < board->h; i++) {
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

void boardMoveUp(Board* board, Cursor* cursor) {
   static bool top_warn = false;
   for (int row = 0; row < board->h; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         //here we should collect a list of top blocks to clear
         if (row == 0 ){
            //todo: end game when board reaches the top
            //if (tile->type != tile_empty && !top_warn) {
            //printf("Harry! I've reached the top");
            //top_warn = true;
            //}
            continue;
         }
         else {
            Tile* above = boardGetTile(board, row - 1, col);
            above->type = tile->type;
            above->texture = tile->texture;
            if (row == board->h - 1) {
               tileInit(board, tile, row, col);    //create new tiles here
            }
         }
      }
   }
   cursor->SetYPosition((cursor->GetYPosition() - board->tileHeight));
   boardCheckClear(board, boardGetRow(board, board->h -1));
}

void _swapTiles(Tile* tile1, Tile* tile2) {
   if (tile1->type == tile_cleared || tile2->type == tile_cleared) { return; }
   if (tile1->type == tile_garbage || tile2->type == tile_garbage) { return; }
   TileEnum tmpEnum = tile2->type;
   SDL_Texture* tmpTexture = tile2->texture;

   tile2->type = tile1->type;
   tile2->texture = tile1->texture;

   tile1->type = tmpEnum;
   tile1->texture = tmpTexture;
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
   boardCheckClear(board, tiles);
   //boardCheckFalling(board);

   return;
}

void boardClearTiles(Board* board, std::vector <Tile*> matches, int delay) {
   //todo: adjust this so it waits a shorter time for 3 clears
   int bonus = 0;
   board->gracePeriod += delay / 1000;
   SDL_Delay(delay);
   //std::this_thread::sleep_for(std::chrono::milliseconds(delay));
   board->gracePeriod -= delay / 1000;

   for (auto&& m : matches) {
      m->type = tile_empty;
      m->texture = nullptr;
   }

   if (matches.size() > 3) {
      bonus = 2000;
      board->gracePeriod += bonus / 1000;
      SDL_Delay(bonus);
      //std::this_thread::sleep_for(std::chrono::milliseconds(bonus));
      board->gracePeriod -= bonus / 1000;
   }
}

void boardCheckClear(Board* board, std::vector <Tile*> tileList) {
   //triggers for matches
   //swaps, board moving
   //falling blocks
   std::vector <Tile*> matches;

   for (auto&& tile : tileList) {
      std::vector <Tile*> cols = boardGetCol(board, posXToCol(board, tile->xpos));
      std::vector <Tile*> rows = boardGetRow(board, posYToRow(board, tile->ypos));

      //check columns
      int current = 0;
      Tile* match = nullptr;

      while (current <= cols.size() - 1) {
         // if we're matching, look for one more and add it
         // if not matching, check if tile is empty or different from next... set matching false
         // if we're not matching... look for group of three

         if (match && match->type == cols[current]->type && cols[current]->type != tile_cleared) {   //We found a match and now we're extending it
            matches.push_back(cols[current]);
         }
         else if (cols[current]->type == tile_empty || cols[current]->type == tile_cleared) {
            match = nullptr;
         }
         else if (current + 2 <= cols.size() - 1) {
            match = nullptr;  //starting to look for a new match here
            if (cols[current]->type == cols[(current + 1)]->type && cols[current]->type == cols[(current + 2)]->type) {
               //add to match list and move counter ahead looking for more
               matches.push_back(cols[current]);
               matches.push_back(cols[current + 1]);
               matches.push_back(cols[current + 2]);

               match = cols[current];
               current = current + 2;
            }
         }
         current = current + 1;
      }

      //check rows
      current = 0;
      match = nullptr;

       while (current <= rows.size() - 1) {

         // if we're matching, look for one more and add it
         // if not matching, check if tile is empty or different from next... set matching false
         // if we're not matching... look for group of three

         if (match && match->type == rows[current]->type && cols[current]->type != tile_cleared) {
            matches.push_back(rows[current]);
         }
         else if (rows[current]->type == tile_empty || cols[current]->type == tile_cleared) {
            match = nullptr;
         }
         else if (current + 2 <= rows.size() - 1) {
            match = nullptr;
            if (rows[current]->type == rows[(current + 1)]->type && rows[current]->type == rows[(current + 2)]->type) {
               //add to match list and move pointers ahead looking for more
               matches.push_back(rows[current]);
               matches.push_back(rows[current + 1]);
               matches.push_back(rows[current + 2]);

               match = rows[current];
               current = current + 2;
            }
         }
         current = current + 1;
      }
      //if (matches.size() > 0) {
      //   //printf("Match size: %d\n", matches.size());
      //   for (auto&& m : matches) {
      //      m->type = tile_cleared;
      //      m->texture = nullptr;
      //   }
      //   matches.clear();
      //}
   }

   if (matches.size() > 0) {
      for (auto&& m : matches) {
         m->texture = TextureManager::LoadTexture(board->game, "assets/skull.png");
         m->type = tile_cleared;
      }
      std::thread t1(boardClearTiles, board, matches, 1500);
      t1.detach();
   }
}

//std::vector <Tile*> boardCheckFalling(Board* board) {
//   std::vector <Tile*> falling;
//   for (int row = board->h - 2; row >= 0; row--) {
//      for (int col = 0; col < board->w; col++) {
//         Tile* tile = boardGetTile(board, row, col);
//         if (tile->type == tile_empty) {
//            continue;
//         }
//         Tile* below = boardGetTile(board, row + 1, col);
//         if (below->type == tile_empty || below->falling == true) {
//            tile->falling = true;
//            falling.push_back(tile);
//            //_swapTiles(tile, below);
//         }
//         else {
//            tile->falling = false;
//         }
//      }
//   }
//   //boardCheckClear(board, doneFalling);
//}

void boardUpdateFalling(Board* board) {
   std::vector <Tile*> tilesToCheck;
   for (int row = board->h - 2; row >= 0; row--) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_empty || tile->type == tile_cleared) {
            continue;
         }
         Tile* below = boardGetTile(board, row + 1, col);
         if (below->type == tile_empty || below->falling == true) {
            _swapTiles(tile, below);

            tile = below;  //now that they are swapped, check if it is still falling
            if (row + 1 == board->h - 1) {  //hit the bottom
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
      boardCheckClear(board, tilesToCheck);
   }
}
