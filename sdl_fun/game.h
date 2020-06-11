#pragma once

#include <SDL.h>
#include "board.h"

class Game {
public:
   Game();
   ~Game();

   void init(const char* title, int x, int y, int width, int height, bool fullscreen);

   void handleEvents();

   void update();
   void render();

   void clean();

   //void CheckTime(int timeInterval);

   static SDL_Renderer *renderer;
   Cursor* cursor;
   Board* board;

   bool running();

   int bHeight;
   int bWidth;

   int tWidth;
   int tHeight;

   bool isRunning;
   SDL_Window *window;

   bool updateBoard;
   bool updateFalling;
};

void startTimer(int time);