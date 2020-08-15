#pragma once
#include <SDL.h>
#include "game.h"

typedef struct Mesh Mesh;
typedef struct Animation Animation;
typedef struct UserInput UserInput;


//@@Start Serialize
struct Cursor {

   float x;
   float y;

   Mesh* mesh;
   Animation* animation;

   int h;
   int w;
};
//@@End Serialize

enum MoveEnum {
   move_up = 0,
   move_down,
   move_right,
   move_left
};

Cursor* cursorCreate(Board* board, float xpos, float ypos);
Cursor* cursorDestroy(Cursor* cursor);

void cursorSetX(Cursor* cursor, float x);
void cursorSetY(Cursor* cursor, float y);

float cursorGetX(Cursor* cursor);
float cursorGetY(Cursor* cursor);

int cursorGetRow(Board* board);
int cursorGetCol(Board* board);

void cursorUpdate(Board* board, UserInput input);
void cursorDraw(Board* board);