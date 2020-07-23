#pragma once

typedef struct Board Board;
typedef struct Tile Tile;
typedef struct Mesh Mesh;

struct Garbage;

Garbage* garbageCreate(Board* board, int width, int layers);
void garbageDestroy(Garbage* garbage);

void garbageCheckClear(Board* board, Tile* tile);
void garbageClear(Board* board, Tile* tile);

void garbageFall(Board* board, float velocity);

void garbageDraw(Board* board);

void garbageSetStart(Board* board, Tile* tile);