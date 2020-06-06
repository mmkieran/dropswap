#pragma once

#include <SDL.h>

class Game {
public:
   Game();
   ~Game();

   void init(const char* title, int x, int y, int width, int height, bool fullscreen);

   void handleEvents();

   void update();
   void render();

   void clean();

   static SDL_Renderer *renderer;

   bool running();

private:
   bool isRunning;
   SDL_Window *window;

   int count;
};
