#pragma once

typedef struct Board Board;
typedef struct Tile Tile;
typedef struct Mesh Mesh;

struct GarbagePile;

//@@Start Serialize
struct Garbage {
   int ID;

   int width;
   int layers;

   Tile* start;  //bot left of garbage
   Mesh* mesh;

   bool deployed = false;
   bool falling;
};
//@@End Serialize

GarbagePile* garbagePileCreate();
GarbagePile* garbagePileDestroy(GarbagePile* pile);

Garbage* garbageCreate(Board* board, int width, int layers);
Garbage* garbageDestroy(Garbage* garbage);

void garbageCheckClear(Board* board, Tile* tile);
//void garbageClear(Board* board, std::map <int, Garbage*> cleared);

void garbageFall(Board* board, float velocity);

void garbageDraw(Board* board);

Garbage* garbageGet(GarbagePile* pile, int id);
void garbageSetStart(GarbagePile* pile, Tile* tile);

void _garbageSerialize(Garbage* garbage, FILE* file);
void _garbageDeserialize(Garbage* garbage, FILE* file);