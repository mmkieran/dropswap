#pragma once
#include <SDL.h>
#include "game.h"

struct Cursor {

   float x;
   float y;

   Mesh* mesh;

   int h;
   int w;
};

Cursor* cursorCreate(Board* board, float xpos, float ypos);
void destroyCursor(Cursor* cursor);

void cursorSetX(Cursor* cursor, float x);
void cursorSetX(Cursor* cursor, float y);

float cursorGetX(Cursor* cursor);
float cursorGetX(Cursor* cursor);

//void cursorUpdate(Board* board);
void cursorDraw(Board* board);