
#include <iostream>
#include <stdlib.h>

#include "game.h"

Game *game = nullptr;

const int FPS = 60;
const int frameDelay = 1000 / FPS;
Uint32 frameStart;
int frameTime;


int main(int argc, char* args[])
{
   game = gameCreate("Game test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 800, false);

   while (gameRunning(game)) {
      frameStart = SDL_GetTicks();

      gameHandleEvents(game);
      gameUpdate(game);
      gameRender(game);

      frameTime = SDL_GetTicks() - frameStart;
      if (frameDelay > frameTime) {
         SDL_Delay(frameDelay - frameTime);
      }
   }

   gameDestroy(game);
   return 0;
}
