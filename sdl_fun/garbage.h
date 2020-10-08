#pragma once

#include <stdint.h>
#include <stdio.h>
#include <map>

typedef struct Board Board;
typedef struct Tile Tile;
typedef struct Mesh Mesh;

//@@Start Serialize
struct Garbage {
   int ID;

   int width;
   int layers;

   Tile* start = nullptr;  //bot left of garbage

   bool deployed = false;
   uint64_t deployTime = 0;

   bool falling = true;
   double totalFall = 0;
   bool metal = false;
};
//@@End Serialize

struct GarbagePile {
   std::map <int, Garbage*> garbage;
   int nextID = 0;
   Mesh* mesh = nullptr;
};

GarbagePile* garbagePileCreate();
GarbagePile* garbagePileDestroy(GarbagePile* pile);

Garbage* garbageCreateEmpty(Board* board);
Garbage* garbageCreate(Board* board, int width, int layers, bool metal = false);
void garbageDeploy(Board* board);
Garbage* garbageDestroy(Garbage* garbage);

void garbageCheckClear(Board* board, Tile* tile);
//void garbageClear(Board* board, std::map <int, Garbage*> cleared);

void garbageFall(Board* board, double velocity);

void garbageDraw(Board* board);

Garbage* garbageGet(GarbagePile* pile, int id);
void garbageSetStart(GarbagePile* pile, Tile* tile);