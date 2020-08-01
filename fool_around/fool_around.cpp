// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <math.h>
#include <vector>
#include <map>
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

struct Board {
   int startH = 12;
   int endH;
   int wBuffer;  //Create some extra board to store falling garbage and upcoming rows
   int w = 6;
   int tileWidth;
   int tileHeight;
   float offset = 0;
};


int main()
{
   Vector<Board*>* vec = vectorCreate<Board*>(20, 2);

   for (int i = 1; i <= 2; i++) {
      Board* board = new Board;
      board->w = i;
      vectorPushBack(vec, board);
   }

   for (int i = 1; i <= 2; i++) {
      printf("%d, %d\n", vectorGet(vec, i)->w, vec->data[i-1]->w);
   }

   for (int i = 1; i <= 2; i++) {
      delete vectorGet(vec, i);
   }
}