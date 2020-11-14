#include "cursor.h"
#include "resources.h"
#include "board.h"
#include "render.h"

Cursor* cursorCreate(Board* board, float xpos, float ypos, int index) {

   Cursor* cursor = new Cursor;
   cursor->index = index;
   cursor->x = xpos;
   cursor->y = ypos;

   cursor->h = board->tileHeight;
   cursor->w = board->tileWidth * 2;

   cursor->animation = animationCreate(7, 200, 64 + 1, 0, 64, 32, true);  //this is specific to the texture sheet
   cursor->animation->texture = resourcesGetTexture(board->game->resources, Texture_cursor);
   textureChangeInterp(cursor->animation->texture, true);

   //For player cursor label
   TextureEnum cursorList[4] = { Texture_cursor1 , Texture_cursor2 , Texture_cursor3, Texture_cursor4 };
   cursor->texture = resourcesGetTexture(board->game->resources, cursorList[index - 1]);

   return cursor;
}

Cursor* cursorDestroy(Cursor* cursor) {
   if (cursor) {
      cursor->animation = animationDestroy(cursor->animation);
      delete cursor;
   }
   return nullptr;
}

void cursorSetX(Cursor* cursor, float x) {
   cursor->x = x;
}

void cursorSetY(Cursor* cursor, float y) {
   cursor->y = y;
}

float cursorGetX(Cursor* cursor) {
   return cursor->x;
}

float cursorGetY(Cursor* cursor) {
   return cursor->y;
}

int cursorGetRow(Board* board, Cursor* cursor) {
   int row = (cursor->y + board->tileHeight - 0.00001) / board->tileHeight + board->startH;
   return row;
}

int cursorGetCol(Board* board, Cursor* cursor) {
   return (int)cursor->x / board->tileWidth;
}

//Draw the cursor on the board with a tag for the player number
void cursorDraw(Board* board, Cursor* cursor) {

   animationDraw(board, cursor->animation, cursor->x, cursor->y, cursor->w, cursor->h);

   //Draw the cursor tag in the correct position
   double xOffset, yOffset;
   if (cursor->index + 1 % 2 == 0) {
      xOffset = cursor->x + cursor->w + board->tileWidth / 6;
      yOffset = cursor->y + cursor->h + board->tileHeight / 6;
   }
   else {
      xOffset = cursor->x - board->tileWidth / 6;
      yOffset = cursor->y - board->tileHeight / 6;
   }
   //todo make cursors tags draw on the right if there are two
   meshDraw(board, cursor->texture, xOffset, yOffset, board->tileWidth/4, board->tileHeight/4);
}

struct DropInfo {
   Tile* rotatePoint = nullptr;
   bool vertical = true;
   
};

static void _createDropTiles(Board* board, Cursor* cursor) {
   bool enoughSpace = false;
   Tile* tile1 = boardGetTile(board, board->startH, board->w / 2);
   Tile* tile2 = boardGetTile(board, board->startH + 1, board->w / 2);
   if (tile1->type == tile_empty && tile2->type == tile_empty) { enoughSpace = true; }
   if (enoughSpace == true) {
      Tile* tiles[2] = { tile1, tile2 };
      for (int i = 0; i < 2; i++) {
         tileInit(board, tiles[i], tileGetRow(board, tiles[i]), tileGetCol(board, tiles[i]), (TileType)boardRandomTile(board));
         tiles[i]->status = status_drop;
         tiles[i]->statusTime = board->game->timer + 10000;
         tiles[i]->falling = true;
         cursor->dropList[i] = tiles[i];
      }
   }
}

void dropFindTiles(Board* board, Cursor* cursor) {
   int count = 0;
   for (int row = 0; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         if (count == 2) { break; }
         Tile* tile = boardGetTile(board, row, col);
         if (tile->status == status_drop) {
            cursor->dropList[count] = tile;
            count++;
         }
      }
   }
   if (count < 2) { _createDropTiles(board, cursor); }
}


void dropLateral(Board* board, Cursor* cursor, int dir) {
   bool enoughSpace = true;
   Tile* target[2];
   std::vector <Tile> backup;
   for (int i = 0; i < 2; i++) {
      backup.push_back(*cursor->dropList[i]);
      int row = tileGetRow(board, cursor->dropList[i]);
      int col = tileGetCol(board, cursor->dropList[i]);
      Tile* neighbor = boardGetTile(board, row, col + dir);
      if (neighbor == nullptr || (neighbor->type != tile_empty && neighbor->status != status_drop)) { enoughSpace = false; }
      else { target[i] = neighbor; }
   }
   if (enoughSpace == true) {
      for (int i = 0; i < 2; i++) {
         int row = tileGetRow(board, &backup[i]);
         int col = tileGetCol(board, &backup[i]);
         Tile tmp = *target[i];
         *target[i] = backup[i];
         target[i]->xpos = tmp.xpos;
         tileInit(board, cursor->dropList[i], row, col, tile_empty);
      }
   }
}

/*
Determine if drop tiles are horizontal or vertical
If vertical, move top... if horizontal move left
Check for space before move
*/
void dropRotate(Board* board, Cursor* cursor, int dir) {
   bool enoughSpace = true;
   Tile* target[2];

   bool vertical = false;
   Tile* top = nullptr;
   if (cursor->dropList[0]->ypos != cursor->dropList[1]->ypos) { vertical = true; }

   for (int i = 0; i < 2; i++) {
      backup.push_back(*cursor->dropList[i]);
      int row = tileGetRow(board, cursor->dropList[i]);
      int col = tileGetCol(board, cursor->dropList[i]);

   }
   if (enoughSpace == true) {
      for (int i = 0; i < 2; i++) {
         tileInit(board, cursor->dropList[i], row, col, tile_empty);
      }
   }
}

void cursorUpdate(Board* board, Cursor* cursor, UserInput input) {
   bool apm = false;
   if (cursor->y <= 0) { cursor->y = board->tileHeight + board->offset; }  //todo we should hide the swap cursor?
   if (input.power.p == true && cursor->mode == 0) { cursor->mode = 1; }
   else if (input.power.p == true && cursor->mode == 1) { cursor->mode = 0; }

   //Swapping mode
   if (cursor->mode == 0) {
      float y = cursorGetY(cursor);
      float x = cursorGetX(cursor);

      if (input.up.p) {
         apm = true;  //Board Stats
         if (y - board->tileHeight <= 0) { return; }
         else { cursorSetY(cursor, (y - board->tileHeight)); }
      }
      else if (input.down.p) {
         apm = true;  //Board Stats
         if (y + board->tileHeight >= board->tileHeight * (board->startH - 1)) { return; }
         else { cursorSetY(cursor, (y + board->tileHeight)); }
      }
      else if (input.right.p) {
         apm = true;  //Board Stats
         if (x >= (board->w - 2) * board->tileWidth) { return; }
         else { cursorSetX(cursor, (x + board->tileWidth)); }
      }
      else if (input.left.p) {
         apm = true;  //Board Stats
         if (x <= 0) { return; }
         else { cursorSetX(cursor, (x - board->tileWidth)); }
      }
      else if (input.right.h) {
         if (x >= (board->w - 2) * board->tileWidth) { return; }
         else { cursorSetX(cursor, (x + board->tileWidth)); }
      }
      else if (input.left.h) {
         if (x <= 0) { return; }
         else { cursorSetX(cursor, (x - board->tileWidth)); }
      }

      else if (input.up.h) {
         if (y - board->tileHeight <= 0) { return; }
         else { cursorSetY(cursor, (y - board->tileHeight)); }
      }
      else if (input.down.h) {
         if (y + board->tileHeight >= board->tileHeight * (board->startH - 1)) { return; }
         else {
            cursorSetY(cursor, (y + board->tileHeight));
         }
      }
      else if (input.swap.p) {
         apm = true;  //Board Stats
         boardSwap(board, cursor);
      }
      else if ((input.nudge.p || input.nudge.h) && board->waitForClear == false) {
         if (board->danger == false) {
            apm = true;  //Board Stats
            boardMoveUp(board, 4 * (board->tileHeight / 64.0f));
            board->paused = false;
            board->pauseLength = 0;
         }
      }
   }
   //Dropping mode
   if (cursor->mode == 1) {
      float y = cursorGetY(cursor);
      float x = cursorGetX(cursor);

      dropFindTiles(board, cursor);

      if (input.up.p) { return; }
      else if (input.down.p) { 
         //Trigger tiles to drop
         //Basically run it through boardFall with extra speed
      }
      else if (input.left.p) {
         dropLateral(board, cursor, -1);
      }
      else if (input.right.p) {
         dropLateral(board, cursor, 1);
      }
      else if (input.swap.p) {
         //This rotates the tiles 90 degrees clockwise?
      }
   }

   if (apm == true) { board->boardStats.apm++; }
}

