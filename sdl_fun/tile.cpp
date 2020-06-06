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

void tileLoadTexture(Tile* tile, const char* path) {
   //hard code this for now
   switch (tile->type) {
   case empty:
      break;
   case circle:
      tile->texture = TextureManager::LoadTexture("assets/grass.png");
      break;
   case diamond:
      tile->texture = TextureManager::LoadTexture("assets/dirt.png");
      break;
   case utriangle:
      tile->texture = TextureManager::LoadTexture("assets/water.png");
      break;
   case dtriangle:
      tile->texture = TextureManager::LoadTexture("assets/water.png");
      break;
   case star:
      tile->texture = TextureManager::LoadTexture("assets/dirt.png");
      break;
   case heart:
      tile->texture = TextureManager::LoadTexture("assets/grass.png");
      break;
   case silver:
      tile->texture = TextureManager::LoadTexture("assets/grass.png");
      break;
   }
}

void tileSetXPosition(Tile* tile, int x) { tile->xpos = x; }
void tileSetYPosition(Tile* tile, int y) { tile->ypos = y; }

int tileGetXPosition(Tile* tile) { return tile->xpos; }
int tileGetYPosition(Tile* tile) { return tile->ypos; }