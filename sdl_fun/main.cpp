
#include <iostream>
#include <stdlib.h>
#include <SDL.h>

#include <Windows.h>  //For win socks 

#include "game.h"

Game *game = nullptr;

const int FPS = 60;
const int frameDelay = 1000000 / FPS;  //microseconds

uint64_t frameStart;
uint64_t frameTime;

int main(int argc, char* args[]) {
   game = gameCreate("Drop and Swap", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1216, 896, false);
   if (!game) {
      printf("Failed to create game...\n");
      return -1;
   }

   WSADATA wd = { 0 };  //Initialize windows socket... Error 10093 means it wasn't started
   WSAStartup(MAKEWORD(2, 2), &wd);  //This was a lot of trouble for 2 lines of code >.<

   game->kt.gameStart = SDL_GetPerformanceCounter(); 
   game->kt.timeFreq = SDL_GetPerformanceFrequency();  //used to convert the performance counter ticks to seconds

   while (gameRunning(game)) {
      frameStart = game->kt.getTime();

      gameHandleEvents(game);  //Inputs have to come before imgui start frame
      imguiStartFrame(game);

      bool bust = gameCheckBust(game);
      if (bust == false) {
         if (game->players > 1) { gameRunFrame(); }
         else if (game->players == 1) { gameSinglePlayer(game); }
      }
      imguiRender(game);  //draw the board, cursor, and other things

      if (game->playing == true && game->paused == false && bust == false) {  //If we're not paused, update game timer
         game->timer += frameDelay / 1000;
      }

      frameTime = game->kt.getTime() - frameStart;
      if (frameDelay >= frameTime) {  //Wait so we get a steady frame rate
         game->ggpoTime = (frameDelay - frameTime) / 1000;
         if (game->players > 1) {
            gameGiveIdleToGGPO(game, game->ggpoTime);
         }
         else { sdlSleep(game->ggpoTime); }
      }
   }

   gameDestroy(game);
   WSACleanup();  //Cleanup the socket stuff

   return 0;
}
