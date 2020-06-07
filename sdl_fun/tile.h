#pragma once
#include <SDL.h>
#include "texture_manager.h"

enum TileEnum {
   empty = 0,
   circle,
   diamond,
   utriangle,
   dtriangle,
   star,
   heart,
   silver
};

struct Tile {

   TileEnum type;

   int xpos;
   int ypos;

   SDL_Texture* texture;   //tile image
   SDL_Rect srcRect;          //What part of the texture are we using
   SDL_Rect destRect;         //Where are we going to render it
};

void tileLoadTexture(Tile* tile, const char* path);

void tileSetXPosition(Tile* tile, int x);
void tileSetYPosition(Tile* tile, int y);

int tileGetXPosition(Tile* tile);
int tileGetYPosition(Tile* tile);