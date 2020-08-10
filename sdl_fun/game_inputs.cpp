#include "game_inputs.h"
#include "game.h"

#include <SDL.h>


struct KeyboardMap {
   Uint8 hk_left = SDL_SCANCODE_LEFT;
   Uint8 hk_right = SDL_SCANCODE_RIGHT;
   Uint8 hk_up = SDL_SCANCODE_UP;
   Uint8 hk_down = SDL_SCANCODE_DOWN;

   Uint8 hk_swap = SDL_SCANCODE_SPACE;
   Uint8 hk_pause = SDL_SCANCODE_RETURN;
};

KeyboardMap kmap;  //todo where should this live?

void old_inputKeyboard(Game* game, SDL_Event e) {

   switch (e.type) {

   case SDL_KEYDOWN:

         switch (e.key.keysym.sym) {
         case SDLK_LEFT:

            break;

         case SDLK_RIGHT:

            break;

         case SDLK_UP:

            break;

         case SDLK_DOWN:

            break;

         case SDLK_SPACE:

            break;

         case SDLK_r:

            break;

         case SDLK_t:

            break;
      }
   }
}

void inputProcessKeyboard(Game* game) {

   const Uint8 *state = SDL_GetKeyboardState(NULL);

   Uint8 keyList[] = { //maybe make this global?
      kmap.hk_left,
      kmap.hk_right,
      kmap.hk_up,
      kmap.hk_down,

      kmap.hk_swap,
      kmap.hk_pause,
   };

   ButtonState* buttonList[] = {
      &game->p1Input.left,
      &game->p1Input.right,
      &game->p1Input.up,
      &game->p1Input.down,

      &game->p1Input.swap,
      &game->p1Input.pause
   };

   //Logic for held keys
   for (int i = 0; i < 4; i++) {  //todo maybe make the count smarter... 
      if (state[ keyList[i] ] == true && buttonList[i]->fc == 0) {  //Button pressed and not held
         buttonList[i]->p = true;
         buttonList[i]->fc++; //increment frame count
      }
      else if (state[keyList[i]] == true && buttonList[i]->fc > 0) {  //Button pressed and held
         buttonList[i]->p = false;
         buttonList[i]->fc++;
         if (buttonList[i]->fc > 10) {  //You held it long enough!
            buttonList[i]->p = false;
            buttonList[i]->h = true;
         }
      }
      else {
         buttonList[i]->p = false;
         buttonList[i]->h = false;
         buttonList[i]->fc = 0;
      }
   }

   //Logic for pressed keys
   for (int i = 4; i < 6; i++) {  //todo maybe make the count smarter... 
      if (state[keyList[i]] == true && buttonList[i]->fc == 0) {
         buttonList[i]->p = true;
         buttonList[i]->fc++; //increment frame count
      }
      else if (state[keyList[i]] == true && buttonList[i]->fc > 0) {  //holding button does nothing
         buttonList[i]->p = false;
         continue;
      }
      else {
         buttonList[i]->p = false;
         buttonList[i]->fc = 0;
      }
   }

}