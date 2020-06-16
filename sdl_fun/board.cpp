#include <vector>
#include <time.h>
#include <chrono>

#include "board.h"
#include "texture_manager.h"
#include "tile.h"
#include "cursor.h"

void _checkClear(std::vector <Tile*> tiles, std::vector <Tile*> &matches);
void boardCheckClear(Board* board, std::vector <Tile*> tileList);

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
         //board->h = game->bHeight;  
         //board->w = game->bWidth;

         board->startH = game->bHeight;  //start playing area of the board
         board->endH = game->bHeight * 2;  //end playing area of the board
         board->wBuffer = board->endH + 1;  //Extra rows for garbage and upcoming rows
         board->w = game->bWidth;

         board->tileHeight = game->tHeight;
         board->tileWidth = game->tWidth;
         board->moveTimer = 0;
         board->fallTimer = 0;
         board->paused = false;
         board->pauseLength = 0;
         board->speed = 1;
         board->bust = false;
         board->offset = 0;

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

void boardUpdate(Board* board) {
   for (int row = board->startH - 1; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);

         tile->ypos -= 1;
         tile->destRect.y = tile->ypos;
      }
   }
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

void _swapTiles(Tile* tile1, Tile* tile2) {
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

void _checkClear(std::vector <Tile*> tiles, std::vector <Tile*> &matches) {
   int current = 0;
   Tile* match = nullptr;

   while (current <= tiles.size() - 1) {
      // if we're matching, look for one more and add it
      // if not matching, check if tile is empty or different from next... set matching false
      // if we're not matching... look for group of three

      if (match && match->type == tiles[current]->type && tiles[current]->type != tile_cleared) {   //We found a match and now we're extending it
         matches.push_back(tiles[current]);
      }
      else if (tiles[current]->type == tile_empty || tiles[current]->type == tile_cleared || tiles[current]->type == tile_garbage) {
         match = nullptr;
      }
      else if (current + 2 <= tiles.size() - 1) {
         match = nullptr;  //starting to look for a new match here
         if (tiles[current]->type == tiles[(current + 1)]->type && tiles[current]->type == tiles[(current + 2)]->type) {
            //add to match list and move counter ahead looking for more
            matches.push_back(tiles[current]);
            matches.push_back(tiles[current + 1]);
            matches.push_back(tiles[current + 2]);

            match = tiles[current];
            current = current + 2;
         }
      }
      current = current + 1;
   }
}

void boardCheckClear(Board* board, std::vector <Tile*> tileList) {
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
         m->texture = TextureManager::LoadTexture(board->game, "assets/skull.png");
         m->type = tile_cleared;
         m->clearTime = clearTime;
         board->paused = true;
         board->pauseLength = 2000;
      }
   }
}

void boardUpdateFalling(Board* board) {
   std::vector <Tile*> tilesToCheck;
   for (int row = board->endH - 2; row >= 0; row--) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_empty || tile->type == tile_cleared) {
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
      boardCheckClear(board, tilesToCheck);
   }
}

void boardClearBlocks(Board* board) {
   int pauseTime = 0;
   int current = SDL_GetTicks();
   for (int row = board->startH -1; row < board->endH; row++) {
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

//void boardUpdateArray(Board* board) {
//
//   for (int row = 0; row < board->wBuffer; row++) {
//      for (int col = 0; col < board->w; col++) {
//         Tile* tile = boardGetTile(board, row, col);
//
//         if (row == board->wBuffer - 1) {
//            tile->type = (TileEnum)board->distribution(board->generator);   //create new tiles here
//            tileLoadTexture(board, tile);
//            tile->ypos += board->tileHeight;
//            tileUpdate(board, tile);
//
//         }
//         else {
//
//            Tile* below = boardGetTile(board, row + 1, col);
//            tile->type = below->type;
//            tile->texture = below->texture;
//            tile->ypos = below->ypos;
//         }
//      }
//   }
//}

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

            }
            else {

               Tile* below = boardGetTile(board, row + 1, col);
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
   //printf("%d : %d\n", board->cursor->GetYPosition(), boardGetTile(board, 19, 3)->ypos );

   boardCheckClear(board, checkTiles);
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

   boardUpdateFalling(board);
   
   return 0;
}