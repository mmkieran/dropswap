#pragma once

typedef struct Board Board;
typedef struct Tile Tile;
typedef struct Mesh Mesh;

struct Garbage {
   int ID;

   int width;
   int layers;

   Tile* start;  //top left of garbage
   Mesh* mesh;
};

Garbage* garbageCreate(Board* board, int width, int layers);
void garbageDestroy(Garbage* garbage);

void garbageClear();

void garbageFall(Board* board, float velocity);

void garbageDraw(Board* board);