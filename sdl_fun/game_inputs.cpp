#include "game_inputs.h"
#include "game.h"

#include <SDL.h>
#include <map>

#define HOLDTIME 15

struct Controller {
   SDL_GameController* controller = nullptr;
   SDL_Joystick* joystick = nullptr;
   bool isController = false;
};

std::map <int, Controller> controllers;  //Hash map for controllers

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

   //Logic for held keys
   for (int i = 0; i < 5; i++) {  //todo maybe make the count smarter... 
      if (state[ keyList[i] ] == true && buttonList[i]->fc == 0) {  //Button pressed and not held
         buttonList[i]->p = true;
         buttonList[i]->fc += 1; //increment hold count
      }
      else if (state[keyList[i]] == true && buttonList[i]->fc > 0) {  //Button pressed and held
         buttonList[i]->p = false;
         buttonList[i]->fc += 1;
         if (buttonList[i]->fc > HOLDTIME) {  //You held it long enough!
            buttonList[i]->p = false;
            buttonList[i]->h = true;
            buttonList[i]->fc = HOLDTIME;  //keep it maxed at 10
         }
      }
      else {
         buttonList[i]->p = false;
         buttonList[i]->h = false;
         buttonList[i]->fc = 0;
      }
   }

   //Logic for pressed keys
   for (int i = 5; i < 8; i++) {  //todo maybe make the count smarter... 
      if (state[keyList[i]] == true && buttonList[i]->fc == 0) {
         buttonList[i]->p = true;
         buttonList[i]->fc += 1; //increment frame count
      }
      else if (state[keyList[i]] == true && buttonList[i]->fc > 0) {  //holding button does nothing
         buttonList[i]->p = false;
         buttonList[i]->fc = 1;
         continue;
      }
      else {
         buttonList[i]->p = false;
         buttonList[i]->h = false;
         buttonList[i]->fc = 0;
      }
   }
}

void controllerGetFileMapping() {
   if (SDL_GameControllerAddMappingsFromFile("assets/gamecontrollerdb.txt") < 0) {
      printf("Could not load SDL Controller mappings: %s", SDL_GetError());
   }
}

void controllerAdd(int id) {
   if (SDL_IsGameController(id)) {
      SDL_GameController* controller = SDL_GameControllerOpen(id);
      if (controller && controllers[id].isController == false) {
         controllers[id].controller = controller;
         controllers[id].joystick = SDL_GameControllerGetJoystick(controller);
         controllers[id].isController = true;
      }
   }
}

void controllerRemove(int id) {
   if (controllers[id].isController == true) {
      SDL_GameControllerClose(controllers[id].controller);
   }
}

void controllerVerify(int deviceid) {
   SDL_IsGameController(deviceid);
}

void controllerGetAll() {
   int sticks = SDL_NumJoysticks();
   for (int i = 0; i < sticks; i++) {
      SDL_GameController* controller = nullptr;
      controller = SDL_GameControllerOpen(i);
      if (controller) { 
         if (SDL_IsGameController(i)) {  //Supported controller
            SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
            int id = SDL_JoystickInstanceID(joystick);
            controllers[id].controller = controller;
            controllers[id].joystick = joystick;
            controllers[id].isController = true;
         }
      }
      else {
         printf("%d is not a valid game controller: %s\n", i, SDL_GetError());
         continue; 
      }  //Not a valid game controller
   }
}

void controllerClose(SDL_GameController* controller) {
   SDL_GameControllerClose(controller);
}

int controllerCloseAll() {
   for (auto&& controller : controllers) {
      int id = controller.first;
      SDL_GameControllerClose(controllers[id].controller);
   }
   controllers.clear();
   return 1;
}

struct ControllerMap {
   SDL_GameControllerButton c_left = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
   SDL_GameControllerButton c_right = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
   SDL_GameControllerButton c_up = SDL_CONTROLLER_BUTTON_DPAD_UP;
   SDL_GameControllerButton c_down = SDL_CONTROLLER_BUTTON_DPAD_DOWN;

   SDL_GameControllerButton c_nudge = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;

   SDL_GameControllerButton c_swap = SDL_CONTROLLER_BUTTON_A;
   SDL_GameControllerButton c_pause = SDL_CONTROLLER_BUTTON_START;
   SDL_GameControllerButton c_power = SDL_CONTROLLER_BUTTON_X;
};

ControllerMap cMap;

void inputProcessController(Game* game) {
   std::vector <UserInput> cInputs;
   for (auto&& pair : controllers) {  //todo currently last controller wins :(
      
      SDL_GameController* controller = pair.second.controller;

      SDL_GameControllerButton keyList[] = { //maybe make this global?
         cMap.c_left,
         cMap.c_right,
         cMap.c_up,
         cMap.c_down,
         cMap.c_nudge,
         //presses only
         cMap.c_swap,
         cMap.c_pause,
         cMap.c_power
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

      //Logic for held keys
      for (int i = 0; i < 5; i++) {  //todo maybe make the count smarter... 
         if (SDL_GameControllerGetButton(controller, keyList[i]) == true && buttonList[i]->fc == 0) {  //Button pressed and not held
            buttonList[i]->p = true;
            buttonList[i]->fc += 1; //increment frame count
         }
         else if (SDL_GameControllerGetButton(controller, keyList[i]) == true && buttonList[i]->fc > 0) {  //Button pressed and held
            buttonList[i]->p = false;
            buttonList[i]->fc += 1;
            if (buttonList[i]->fc > HOLDTIME) {  //You held it long enough!
               buttonList[i]->p = false;
               buttonList[i]->h = true;
               buttonList[i]->fc = HOLDTIME;
            }
         }
         else {
            buttonList[i]->p = false;
            buttonList[i]->h = false;
            buttonList[i]->fc = 0;
         }
      }

      //Logic for pressed keys
      for (int i = 5; i < 8; i++) {  //todo maybe make the count smarter... 
         if (SDL_GameControllerGetButton(controller, keyList[i]) == true && buttonList[i]->fc == 0) {
            buttonList[i]->p = true;
            buttonList[i]->fc += 1; //increment frame count
         }
         else if (SDL_GameControllerGetButton(controller, keyList[i]) == true && buttonList[i]->fc > 0) {  //holding button does nothing
            buttonList[i]->p = false;
            buttonList[i]->fc = 1;
            continue;
         }
         else {
            buttonList[i]->p = false;
            buttonList[i]->h = false;
            buttonList[i]->fc = 0;
         }
      }
   }
}