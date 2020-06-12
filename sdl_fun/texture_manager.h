#pragma once

#include "game.h"
#include <vector>

class TextureManager {
public:
   static SDL_Texture* LoadTexture(Game* game, const char* fileName);
   static void Draw(Game* game, SDL_Texture* tex, SDL_Rect src, SDL_Rect dest);

};