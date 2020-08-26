#include "game_inputs.h"
#include "game.h"

#include <SDL.h>

struct ControllerMap {

};

struct KeyboardMap {
   Uint8 hk_left = SDL_SCANCODE_LEFT;
   Uint8 hk_right = SDL_SCANCODE_RIGHT;
   Uint8 hk_up = SDL_SCANCODE_UP;
   Uint8 hk_down = SDL_SCANCODE_DOWN;

   Uint8 hk_nudge = SDL_SCANCODE_R;

   Uint8 hk_swap = SDL_SCANCODE_SPACE;
   Uint8 hk_pause = SDL_SCANCODE_RETURN;
   Uint8 hk_power = SDL_SCANCODE_F;
};

KeyboardMap kmap;  //todo where should this live?

void inputProcessKeyboard(Game* game) {

   const Uint8 *state = SDL_GetKeyboardState(NULL);

   Uint8 keyList[] = { //maybe make this global?
      kmap.hk_left,
      kmap.hk_right,
      kmap.hk_up,
      kmap.hk_down,
      kmap.hk_nudge,
      //presses only
      kmap.hk_swap,
      kmap.hk_pause,
      kmap.hk_power
   };

   ButtonState* buttonList[] = {
      &game->p1Input.left,
      &game->p1Input.right,
      &game->p1Input.up,
      &game->p1Input.down,
      &game->p1Input.nudge,
      //presses only
      &game->p1Input.swap,
      &game->p1Input.pause,
      &game->p1Input.power
   };

   int* buttonHolds[] = {
      &game->buttonHolds.left,
      &game->buttonHolds.right,
      &game->buttonHolds.up,
      &game->buttonHolds.down,
      &game->buttonHolds.nudge,
      //presses only
      &game->buttonHolds.swap,
      &game->buttonHolds.pause,
      &game->buttonHolds.power
   };

   //Logic for held keys
   for (int i = 0; i < 5; i++) {  //todo maybe make the count smarter... 
      if (state[ keyList[i] ] == true && *buttonHolds[i] == 0) {  //Button pressed and not held
         buttonList[i]->p = true;
         *buttonHolds[i] += 1; //increment frame count
      }
      else if (state[keyList[i]] == true && *buttonHolds[i] > 0) {  //Button pressed and held
         buttonList[i]->p = false;
         *buttonHolds[i] += 1;
         if (*buttonHolds[i] > 10) {  //You held it long enough!
            buttonList[i]->p = false;
            buttonList[i]->h = true;
         }
      }
      else {
         buttonList[i]->p = false;
         buttonList[i]->h = false;
         *buttonHolds[i] = 0;
      }
   }

   //Logic for pressed keys
   for (int i = 5; i < 8; i++) {  //todo maybe make the count smarter... 
      if (state[keyList[i]] == true && *buttonHolds[i] == 0) {
         buttonList[i]->p = true;
         *buttonHolds[i] += 1; //increment frame count
      }
      else if (state[keyList[i]] == true && *buttonHolds[i] > 0) {  //holding button does nothing
         buttonList[i]->p = false;
         continue;
      }
      else {
         buttonList[i]->p = false;
         *buttonHolds[i] = 0;
      }
   }
}

void controllerGetFileMapping() {
   if (SDL_GameControllerAddMappingsFromFile("assets/gamecontrollerdb.txt") < 0) {
      printf("Could not load SDL Controller mappings: %s", SDL_GetError());
   }
}

SDL_GameController* controllerGet() {
   SDL_GameController* controller = nullptr;
   for (int i = 0; i < SDL_NumJoysticks(); i++) {
      controller = SDL_GameControllerOpen(i);
      if (controller) { break; }
      else {
         printf("%d is not a valid game controller: %s\n", i, SDL_GetError());
         continue; 
      }  //Not a valid game controller
   }
   return controller;
}

void controllerClose(SDL_GameController* controller) {
   SDL_GameControllerClose(controller);
}

void controllerVerify() {

}

void controllerInput(SDL_GameController* controller) {

   bool Up = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
   bool Down = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
   bool Left = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
   bool Right = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

   bool Start = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START);
   bool Back = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK);
   bool LeftShoulder = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
   bool RightShoulder = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

   bool AButton = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
   bool BButton = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B);
   bool XButton = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X);
   bool YButton = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y);

   int StickX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
   int StickY = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
}