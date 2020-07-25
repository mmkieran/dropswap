// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <math.h>
#include <vector>
#include "sdl_fun/myvector.h"


//I really recommend having your own struct that looks like this!
struct Vec2
{
   float x, y;
};

enum TileEnum {
   tile_empty = 0,
   tile_circle,
   tile_diamond,
   tile_utriangle,
   tile_dtriangle,
   tile_star,
   tile_heart,
   tile_silver,
   tile_garbage,
   tile_cleared
};

struct Tile {

   TileEnum type;

   float xpos;
   float ypos;

   bool falling;
   int clearTime;
   bool chain;

};

Tile* _boardCreateArray(int width, int height) {
   Tile* tiles = (Tile*)malloc(sizeof(Tile) * (height * 2 + 1) * width);
   //memset(tiles, 0, sizeof(Tile) * (height * 2 + 1) * width);
   return tiles;
}

int main()
{
   Vector<Tile>* vec = vectorCreate<Tile>(20, 2);

   for (int i = 0; i < 8; i++) {
      Tile tile;
      tile.type = (TileEnum)(tile_empty + i);
      tile.ypos = 64 * i;
      vectorPushBack(vec, tile);
   }

   vectorErase(vec, 2);
   vectorSwap(vec, 1, 3);

   for (int i = 1; i <= vectorSize(vec); i++) {
      printf("Type int: %d, ypos: %0.1f \n", vectorGet(vec, i)->type, vectorGet(vec, i)->ypos);
   }

   //Tile tile2;
   //tile2.type = (TileEnum)(tile_diamond);
   //tile2.ypos = 64;
   //printf("Found: %d\n", vectorFind(vec, tile2.type));
}