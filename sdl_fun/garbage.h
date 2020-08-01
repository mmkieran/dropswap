#pragma once

typedef struct Board Board;
typedef struct Tile Tile;
typedef struct Mesh Mesh;

struct GarbagePile;
struct Garbage;

GarbagePile* createGarbagePile();
GarbagePile* destroyGarbagePile(GarbagePile* pile);

Garbage* garbageCreate(Board* board, int width, int layers);
void garbageDestroy(Garbage* garbage);

void garbageCheckClear(Board* board, Tile* tile);
//void garbageClear(Board* board, std::map <int, Garbage*> cleared);

void garbageFall(Board* board, float velocity);

void garbageDraw(Board* board);

Garbage* garbageGet(Board* board, int id);
void garbageSetStart(Board* board, Tile* tile);