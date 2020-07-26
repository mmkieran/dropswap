#include "cursor.h"
#include "resources.h"
#include "board.h"
#include "render.h"

struct Cursor {

   float x;
   float y;

   Mesh* mesh;

   int h;
   int w;
};

Cursor* cursorCreate(Board* board, float xpos, float ypos) {

   Cursor* cursor = new Cursor;

   cursor->x = xpos;
   cursor->y = ypos;

   cursor->mesh = meshCreate(board->game);
   cursor->mesh->texture = resourcesGetTexture(board->game->resources, Texture_cursor);

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
   //todo put something here...
}

void cursorDraw(Board* board) {
   Cursor* cursor = board->cursor;
   Game* game = board->game;

   Vec2 adj = board->origin;

   meshDraw(game, cursor->mesh, cursor->x + adj.x, cursor->y + adj.y, cursor->w, cursor->h);
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

