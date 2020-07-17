// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <math.h>
#include <vector>


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
   int w = 6;
   int h = 12;

   int row = 2;
   int col = 3;

   Tile* tiles = _boardCreateArray(w, h);

   Tile* tile = &tiles[(w * row + col)];
   Tile* above = &tiles[(w * (row + 1) + col)];

   int calcRow = (tile - tiles) / w;
   int calcCol = (tile - tiles) % w;

   int calcRow2 = (above - tiles) / w;
   int calcCol2 = (above - tiles) % w;

   printf("row: %d\n", calcRow);
   printf("row: %d\n", calcCol);

   printf("row: %d\n", calcRow2);
   printf("row: %d\n", calcCol2);

   free(tiles);
}