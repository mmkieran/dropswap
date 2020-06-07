#pragma once
#include "tile.h"

typedef struct Board Board;

Board* boardCreate(int height, int width);
void boardDestroy(Board* board);

Tile* boardGetTile(Board* board, int row, int col);

int boardFillTiles(Board* board);

void boardUpdate(Board* board);
void boardRender(Board* board);