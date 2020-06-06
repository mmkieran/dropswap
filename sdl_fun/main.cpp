
#include <iostream>
#include <SDL.h>
#include <stdlib.h>
#include "game.h"

Game *game = nullptr;

const int FPS = 60;
const int frameDelay = 1000 / FPS;
Uint32 frameStart;
int frameTime;


int main(int argc, char* args[])
{
   game = new Game;

   game->init("Game test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 640, false);

   while (game->running()) {
      frameStart = SDL_GetTicks();

      game->handleEvents();
      game->update();
      game->render();

      frameTime = SDL_GetTicks() - frameStart;
      if (frameDelay > frameTime) {
         SDL_Delay(frameDelay - frameTime);
      }
   }

   game->clean();

   delete game;
   return 0;
}
