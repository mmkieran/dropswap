#pragma once
#include "tile.h"
#include "cursor.h"

typedef struct Board Board;

Board* boardCreate(int height, int width, int tileHeight, int tileWidth);
void boardDestroy(Board* board);

Tile* boardGetTile(Board* board, int row, int col);

int boardFillTiles(Board* board);

void boardUpdate(Board* board);
void boardRender(Board* board);
void boardMoveUp(Board* board);

void boardSwap(Board* board, Cursor* cursor);
void boardCheckClear(Board* board, std::vector <Tile*> tileList);