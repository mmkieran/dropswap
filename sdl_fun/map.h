#pragma once
#include "game.h"

class Map {
public:
   Map();
   ~Map();

   void LoadMap(int arr[12][6]);
   void DrawMap();

private:
   SDL_Rect src, dest;
   SDL_Texture* dirt;
   SDL_Texture* grass;
   SDL_Texture* water;

   int map[12][6];

};