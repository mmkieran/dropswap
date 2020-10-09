
#include <iostream>
#include <stdlib.h>
#include <SDL.h>

//#include <Windows.h>  //For win socks 
#include <winsock.h>  //For win socks 

#include "game.h"

Game *game = nullptr;

int main(int argc, char* args[]) {
   game = gameCreate("Drop and Swap", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1216, 896, false);
   if (!game) { return -1; }

   WSADATA wd = { 0 };  //Initialize windows socket... Error 10093 means it wasn't started
   WSAStartup(MAKEWORD(2, 2), &wd);  //This was a lot of trouble for 2 lines of code >.<

   game->kt.gameStart = SDL_GetPerformanceCounter(); 
   game->kt.timeFreq = SDL_GetPerformanceFrequency();  //used to convert the performance counter ticks to seconds

   while (gameRunning(game)) {
      game->kt.frameStart = game->kt.getTime();

      gameHandleEvents(game);  //Inputs have to come before imgui start frame
      imguiStartFrame(game);

      bool bust = gameCheckBust(game);
      if (bust == false) {
         if (game->players > 1) { gameRunFrame(); }
         else if (game->players == 1) { gameSinglePlayer(game); }
      }
      imguiRender(game);  //draw the board, cursor, and other things

      game->kt.frameTime = game->kt.getTime() - game->kt.frameStart;
      if (game->kt.frameDelay >= game->kt.frameTime) {  
         int leftover = (game->kt.frameDelay - game->kt.frameTime) / 1000;
         if (game->players > 1) {
            gameGiveIdleToGGPO(game, leftover - 2);  //Give some time to GGPO, but leave some for vsync
         }
      }

      sdlSwapWindow(game);  //Try putting this at the end so vsync doesn't take all the time

      //gameFrameDelay(game);  //Wait so we get a steady frame rate

   }

   gameDestroy(game);
   WSACleanup();  //Cleanup the socket stuff

   return 0;
}
