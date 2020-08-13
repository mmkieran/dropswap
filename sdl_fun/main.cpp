
#include <iostream>
#include <stdlib.h>
#include <SDL.h>

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

      gameHandleEvents(game);  //Inputs have to come before imgui start frame

      imguiStartFrame(game);

      imguiShowDemo();
      showGameMenu(game);

      gameRunFrame();

      imguiRender(game);  //draw the board, cursor, and other things

      //if (game->paused == false && game->playing == true) {
      //   gameUpdate(game);  //update game and board state
      //}
      //else {
      //   game->pauseLength += getTime() - frameStart;  //Use this to correct timeDelta after pause
      //}

      frameTime = getTime() - frameStart;
      if (frameDelay >= frameTime) {  //Wait so we get a steady frame rate
         SDL_Delay((frameDelay - frameTime) / 1000);
      }

      if (game->playing == true && game->paused == false) {  //If we're paused, update game timer
         game->timeDelta = (getTime() - frameStart) / 1000;
         game->timer += game->timeDelta;
      }
   }

   gameDestroy(game);
   return 0;
}
