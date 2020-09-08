#pragma once

typedef struct Board Board;
typedef struct Mesh Mesh;
typedef struct Animation Animation;
typedef struct UserInput UserInput;


//@@Start Serialize
struct Cursor {

   double x;
   double y;

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