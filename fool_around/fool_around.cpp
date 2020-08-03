// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unordered_map>
#include <math.h>
#include <vector>
#include <map>
#include "sdl_fun/myvector.h"
#include "sdl_fun/serialize.h"
#include "sdl_fun/tile.h"


FILE* _gameSaveState() {
   FILE* out;
   int err = fopen_s(&out, "../sdl_fun/assets/game_state.dat", "w");
   if (err == 0) {
      Tile myTile;
      Tile* tile = &myTile;
      tile->type = tile_heart;
      tile->status = status_normal;
      tile->xpos = 128;
      tile->ypos = 700;

      tile->clearTime = 0;
      tile->statusTime = 0;
      tile->falling = false;
      tile->chain = false;

      //_tileSerialize(tile, out);
      fprintf(out, "testing");
   }
   else { printf("Failed to save file... Err: %d\n", err); }
   fclose(out);
   return out;
}


int _gameLoadState() {
   FILE* in;
   int err = fopen_s(&in, "../sdl_fun/assets/game_state.dat", "r");
   if (err == 0) {
      while (!feof(in)) {
         Tile myTile;
         //_tileDeserialize(&myTile, in);
         int a = 0;
      }
   }
   else { printf("Failed to load file... Err: %d\n", err); }
   fclose(in);
   return 1;
}


int main(int argc, char* args[])
{
   printf("%d\n", sizeof(uint64_t));
   printf("%d\n", sizeof(int));
   printf("%d\n", sizeof(long long unsigned int));

   _gameSaveState();
   _gameLoadState();

   return 0;
}