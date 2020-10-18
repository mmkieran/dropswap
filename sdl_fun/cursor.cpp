#include "cursor.h"
#include "resources.h"
#include "board.h"
#include "render.h"

Cursor* cursorCreate(Board* board, float xpos, float ypos, int index) {

   Cursor* cursor = new Cursor;

   cursor->x = xpos;
   cursor->y = ypos;

   cursor->h = board->game->tHeight;
   cursor->w = board->game->tWidth * 2;

   cursor->animation = animationCreate(7, 200, 64 + 1, 0, 64, 32, true);  //this is specific to the texture sheet
   cursor->animation->texture = resourcesGetTexture(board->game->resources, Texture_cursor);
   textureChangeInterp(cursor->animation->texture, true);

   //For player cursor label
   TextureEnum cursorList[4] = { Texture_cursor1 , Texture_cursor2 , Texture_cursor3, Texture_cursor4 };
   cursor->texture = resourcesGetTexture(board->game->resources, cursorList[index]);

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

void cursorDraw(Board* board, Cursor* cursor) {

   animationDraw(board, cursor->animation, cursor->x, cursor->y, cursor->w, cursor->h);

   meshDraw(board, cursor->texture, cursor->x - board->tileWidth/6, cursor->y - board->tileHeight /6, board->tileWidth/4, board->tileHeight/4);
}

void cursorUpdate(Board* board, Cursor* cursor, UserInput input) {
   bool apm = false;

   if (cursor->y <= 0) {
      cursor->y = board->tileHeight + board->offset;
   }

   float y = cursorGetY(cursor);
   float x = cursorGetX(cursor);

   if (input.up.p) {
      apm = true;  //Board Stats
      if (y - board->tileHeight <= 0) { return; }
      else {
         cursorSetY(cursor, (y - board->tileHeight));
      }
   }

   else if (input.down.p) {
      apm = true;  //Board Stats
      if (y + board->tileHeight >= board->tileHeight * (board->startH - 1)) { return; }
      else {
         cursorSetY(cursor, (y + board->tileHeight));
      }
   }

   else if (input.right.p) {
      apm = true;  //Board Stats
      if (x >= (board->w - 2) * board->tileWidth) { return; }
      else {
         cursorSetX(cursor, (x + board->tileWidth));
      }
   }

   else if (input.left.p) {
      apm = true;  //Board Stats
      if (x <= 0) { return; }
      else {
         cursorSetX(cursor, (x - board->tileWidth));
      }
   }

   else if (input.right.h) {
      if (x >= (board->w - 2) * board->tileWidth) { return; }
      else {
         cursorSetX(cursor, (x + board->tileWidth));
      }
   }

   else if (input.left.h) {
      if (x <= 0) { return; }
      else {
         cursorSetX(cursor, (x - board->tileWidth));
      }
   }

   else if (input.up.h) {
      if (y - board->tileHeight <= 0) { return; }
      else {
         cursorSetY(cursor, (y - board->tileHeight));
      }
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

   else if (input.nudge.p && board->paused == false) {
      apm = true;  //Board Stats
      boardMoveUp(board, 4 * (board->tileHeight / 64.0f));
   }

   else if (input.nudge.h && board->paused == false) {
      boardMoveUp(board, 4 * (board->tileHeight / 64.0f));
   }

   if (apm == true) { board->boardStats.apm++; }
}

