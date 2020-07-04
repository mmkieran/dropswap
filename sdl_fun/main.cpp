
#include <iostream>
#include <stdlib.h>

#include "game.h"
#include "render.h"

Game *game = nullptr;

const int FPS = 60;
const int frameDelay = 1000 / FPS;
Uint32 frameStart;
int frameTime;


int main(int argc, char* args[])
{
   game = gameCreate("Game test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 900, false);
   if (!game) {
      printf("Failed to create game...\n");
      return -1;
   }

   while (gameRunning(game)) {
      frameStart = SDL_GetTicks();

      gameHandleEvents(game);

      if (!game->paused) {
         gameUpdate(game);
         gameRender(game);
      }
      else {
         game->pauseLength += SDL_GetTicks() - frameStart;  //Use this to correct timeDelta after pause
      }

      frameTime = SDL_GetTicks() - frameStart;
      if (frameDelay >= frameTime) {  //Wait so we get a steady frame rate
         SDL_Delay(frameDelay - frameTime);
      }

      game->timeDelta = SDL_GetTicks() - frameStart;
      game->timer += game->timeDelta;
   }

   gameDestroy(game);
   return 0;
}
