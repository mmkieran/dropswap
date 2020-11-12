#pragma once

typedef struct Board Board;
typedef struct Mesh Mesh;
typedef struct Animation Animation;
typedef struct UserInput UserInput;
typedef struct Texture Texture;

//@@Start Serialize
struct Cursor {
   int index = 0;                         //This is the player number (Corresponds to GGPO player number)
   double x;                              //Cursor x position on the board
   double y;                              //Cursor y position on the board
   int h;                                 //Cursor height
   int w;                                 //Cursor width
   int mode = 0;                          //1 is dropping and 0 is swapping
   int dropList[2];                       //The list of tiles currently being dropped

   Texture* texture = nullptr;            //Used to draw the cursor tag texture
   Animation* animation = nullptr;        //Used to draw the cursor animation
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