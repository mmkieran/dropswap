
#include <iostream>
#include <stdlib.h>
#include <SDL.h>

#include "game.h"

Game *game = nullptr;

//Frame variables
uint64_t end = 0;

int main(int argc, char* args[]) {
   game = gameCreate("Drop and Swap", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1216, 896, false);
   if (!game) { return -1; }

   game->kt.gameStart = SDL_GetPerformanceCounter(); 
   game->kt.timeFreq = SDL_GetPerformanceFrequency();  //used to convert the performance counter ticks to seconds

   while (gameRunning(game)) {
      uint64_t start = game->kt.getTime();

      gameHandleEvents(game);  //Inputs have to come before imgui start frame
      imguiStartFrame(game);

      gameCheckBust(game);
      if (game->busted == -1) {
         if (game->settings.replaying == true) { 
            int rate = game->settings.replaySpeed;
            if (game->frameCount + 1 + rate > game->settings.repInputs.size()) {
               rate = game->settings.repInputs.size() - (game->frameCount + 1 + rate);
            }
            if (game->settings.replayPaused == false) {
               for (int i = 0; i < rate; i++) {
                  gameReplay(game);
               }
            }
         }
         else if (game->settings.mode == multi_solo || game->settings.mode == multi_shared) { gameRunFrame(); }
         else if (game->settings.mode == single_player || game->settings.mode == single_vs) { gameSinglePlayer(game); }
      }
      imguiRender(game);  //imgui windows, the board, cursor, and other things 
      
      end = game->kt.getTime();
      gameDelayFrame(game, end, start);
   }
   gameDestroy(game);

   return 0;
}
