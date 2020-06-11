#pragma once
#include <SDL.h>

class Cursor {
public:
   Cursor(const char* texturesheet, int x, int y, int cursor_width, int cursor_height);
   ~Cursor();

   void SetXPosition(int x);
   void SetYPosition(int y);

   int GetXPosition();
   int GetYPosition();

   void Update();
   void Render();

   int xpos;
   int ypos;

   SDL_Texture* objTexture;

   SDL_Rect srcRect;
   SDL_Rect destRect;

   int height;
   int width;
};