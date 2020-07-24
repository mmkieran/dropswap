#pragma once
#include <SDL.h>
#include "game.h"

typedef struct Mesh Mesh;

struct Cursor;

enum MoveEnum {
   move_up = 0,
   move_down,
   move_right,
   move_left
};

Cursor* cursorCreate(Board* board, float xpos, float ypos);
void cursorDestroy(Cursor* cursor);

void cursorSetX(Cursor* cursor, float x);
void cursorSetY(Cursor* cursor, float y);

float cursorGetX(Cursor* cursor);
float cursorGetY(Cursor* cursor);

int cursorGetRow(Board* board);
int cursorGetCol(Board* board);

void cursorUpdate(Board* board);
void cursorDraw(Board* board);
void cursorMove(Board* board, MoveEnum dir);