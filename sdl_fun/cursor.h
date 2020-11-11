#pragma once

typedef struct Board Board;
typedef struct Mesh Mesh;
typedef struct Animation Animation;
typedef struct UserInput UserInput;
typedef struct Texture Texture;

enum CursorMode {
   cursor_swapping = 0,
   cursor_dropping,
};

//@@Start Serialize
struct Cursor {
   int index = 0;                      //This is the player number (Corresponds to GGPO player number)
   double x;                           //Cursor x position on the board
   double y;                           //Cursor y position on the board
   CursorMode mode = cursor_swapping;  //Is the cursor swapping or dropping

   Texture* texture;                   //Used to draw the cursor tag texture
   Animation* animation;               //Used to draw the cursor animation

   int h;                              //Cursor height
   int w;                              //Cursor width
};
//@@End Serialize

Cursor* cursorCreate(Board* board, float xpos, float ypos, int index);
Cursor* cursorDestroy(Cursor* cursor);

void cursorSetX(Cursor* cursor, float x);
void cursorSetY(Cursor* cursor, float y);

float cursorGetX(Cursor* cursor);
float cursorGetY(Cursor* cursor);

int cursorGetRow(Board* board, Cursor* cursor);
int cursorGetCol(Board* board, Cursor* cursor);

void cursorUpdate(Board* board, Cursor* cursor, UserInput input);
void cursorDraw(Board* board, Cursor* cursor);