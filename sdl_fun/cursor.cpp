#include "cursor.h"
#include "resources.h"
#include "board.h"
#include "render.h"


Cursor* cursorCreate(Board* board, float xpos, float ypos) {

   Cursor* cursor = new Cursor;

   cursor->x = xpos;
   cursor->y = ypos;

   Mesh* mesh = createMesh(board->game);
   mesh->texture = board->game->resources->textures[8];

   cursor->h = board->game->tHeight;
   cursor->w = board->game->tWidth * 2;
}

void destroyCursor(Cursor* cursor) {
   delete cursor;
}

void cursorSetX(Cursor* cursor, float x) {
   cursor->x = x;
}

void cursorSetX(Cursor* cursor, float y) {
   cursor->y = y;
}

float cursorGetX(Cursor* cursor) {
   return cursor->x;
}

float cursorGetX(Cursor* cursor) {
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

