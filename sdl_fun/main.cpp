
#include <iostream>
#include <stdlib.h>

#include "game.h"
#include "render.h"

Game *game = nullptr;

const int FPS = 60;
const int frameDelay = 1000000 / FPS;  //microseconds

uint64_t frameStart;
uint64_t frameTime;

uint64_t gameStart;
uint64_t timeFreq;

uint64_t getTime() {
   uint64_t current = SDL_GetPerformanceCounter();
   return ((current - gameStart) * 1000000) / timeFreq;
}


int main(int argc, char* args[])
{
   game = gameCreate("Game test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1216, 896, false);
   if (!game) {
      printf("Failed to create game...\n");
      return -1;
   }

   gameStart = SDL_GetPerformanceCounter(); 
   timeFreq = SDL_GetPerformanceFrequency();  //used to convert the performance counter ticks to seconds

   while (gameRunning(game)) {
      frameStart = getTime();

      showGameMenu(game);

      gameHandleEvents(game);

      if (!game->paused) {
         gameUpdate(game);  //update game and board state
      }
      else {
         game->pauseLength += getTime() - frameStart;  //Use this to correct timeDelta after pause
      }

      gameRender(game);  //draw the board, cursor, and other things

      frameTime = getTime() - frameStart;
      if (frameDelay >= frameTime) {  //Wait so we get a steady frame rate
         SDL_Delay((frameDelay - frameTime) / 1000);
      }

      game->timeDelta = (getTime() - frameStart) / 1000;
      game->timer += game->timeDelta;
   }

   gameDestroy(game);
   return 0;
}
