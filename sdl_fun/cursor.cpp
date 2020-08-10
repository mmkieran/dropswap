#include "cursor.h"
#include "resources.h"
#include "board.h"
#include "render.h"

Cursor* cursorCreate(Board* board, float xpos, float ypos) {

   Cursor* cursor = new Cursor;

   cursor->x = xpos;
   cursor->y = ypos;

   cursor->mesh = meshCreate(board->game);
   meshSetTexture(board->game, cursor->mesh, Texture_cursor);
   textureChangeInterp(cursor->mesh, true);  //Changes Linear interpolation of the texture to Nearest

   cursor->h = board->game->tHeight;
   cursor->w = board->game->tWidth * 2;

   cursor->animation = animationCreate(7, 200, 64 + 1, 0, 64, 32, true);  //this is specific to the texture sheet

   return cursor;
}

Cursor* cursorDestroy(Cursor* cursor) {
   if (cursor) {
      cursor->mesh = meshDestroy(cursor->mesh);
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

int cursorGetRow(Board* board) {
   return (int)(board->cursor->y - board->offset) / board->tileHeight + board->startH;
}

int cursorGetCol(Board* board) {
   return (int)board->cursor->x / board->tileWidth;
}

void cursorUpdate(Board* board) {
   if (board->cursor->y <= 0) {
      board->cursor->y = board->tileHeight + board->offset;
   }
   cursorMove(board);
}

void cursorDraw(Board* board) {
   Cursor* cursor = board->cursor;
   Game* game = board->game;

   Vec2 adj = board->origin;

   animationDraw(game, cursor->animation, cursor->mesh, cursor->x + adj.x, cursor->y + adj.y, cursor->w, cursor->h);
   //meshDraw(game, cursor->mesh, cursor->x + adj.x, cursor->y + adj.y, cursor->w, cursor->h);
}

void cursorMove(Board* board) {
   float y = cursorGetY(board->cursor);
   float x = cursorGetX(board->cursor);

   if (board->game->p1Input.upPressed) {
      if (y - board->tileHeight <= 0) { return; }
      else {
         cursorSetY(board->cursor, (y - board->tileHeight));
      }
   }

   else if (board->game->p1Input.downPressed) {
      if (y + board->tileHeight >= board->tileHeight * (board->startH - 1)) { return; }
      else {
         cursorSetY(board->cursor, (y + board->tileHeight));
      }
   }

   else if (board->game->p1Input.rightPressed) {
      if (x >= (board->w - 2) * board->tileWidth) { return; }
      else {
         cursorSetX(board->cursor, (x + board->tileWidth));
      }
   }

   else if (board->game->p1Input.leftPressed) {
      if (x <= 0) { return; }
      else {
         cursorSetX(board->cursor, (x - board->tileWidth));
      }
   }
}

