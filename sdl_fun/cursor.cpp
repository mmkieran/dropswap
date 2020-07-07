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

