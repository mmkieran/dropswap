#include "cursor.h"
#include "resources.h"
#include "board.h"
#include "render.h"


Cursor* cursorCreate(Board* board, float xpos, float ypos) {

   Cursor* cursor = new Cursor;

   cursor->x = xpos;
   cursor->y = ypos;

   cursor->mesh = createMesh(board->game);
   cursor->mesh->texture = board->game->resources->textures[8];

   cursor->h = board->game->tHeight;
   cursor->w = board->game->tWidth * 2;

   return cursor;
}

void cursorDestroy(Cursor* cursor) {
   delete cursor;
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

void cursorUpdate(Board* board) {

   //todo put something here...
}

void cursorDraw(Board* board) {
   Cursor* cursor = board->cursor;
   Game* game = board->game;

   drawMesh(game, cursor->mesh, cursor->x, cursor->y, cursor->w, cursor->h);
}

void cursorMove(Board* board, MoveEnum dir) {
   float y = cursorGetY(board->cursor);
   float x = cursorGetX(board->cursor);

   if (dir == move_up) {
      if (y - board->tileHeight <= 0) { return; }
      else {
         cursorSetY(board->cursor, (y - board->tileHeight));
      }
   }

   else if (dir == move_down) {
      if (y + board->tileHeight >= board->tileHeight * (board->startH - 1)) { return; }
      else {
         cursorSetY(board->cursor, (y + board->tileHeight));
      }
   }

   else if (dir == move_right) {
      if (x >= (board->w - 2) * board->tileWidth) { return; }
      else {
         cursorSetX(board->cursor, (x + board->tileWidth));
      }
   }

   else if (dir == move_left) {
      if (x <= 0) { return; }
      else {
         cursorSetX(board->cursor, (x - board->tileWidth));
      }
   }
}

