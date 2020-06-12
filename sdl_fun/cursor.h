#pragma once
#include <SDL.h>
#include "game.h"

class Cursor {
public:
   Cursor(Game* game, const char* texturesheet, int x, int y);
   ~Cursor();

   void SetXPosition(int x);
   void SetYPosition(int y);

   int GetXPosition();
   int GetYPosition();

   void Update();
   void Render(Game* game);

   int xpos;
   int ypos;

   SDL_Texture* objTexture;

   SDL_Rect srcRect;
   SDL_Rect destRect;

   int height;
   int width;
};