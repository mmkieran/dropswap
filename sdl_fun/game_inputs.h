#pragma once

typedef struct Game Game;
typedef unsigned long long uint64_t;

struct ButtonState {
   bool p = false;  //pressed
   bool h = false;  //held
   unsigned char fc = 0;     //Frame count for hold
};

struct UserInput {
   ButtonState left;
   ButtonState right;
   ButtonState up;
   ButtonState down;

   ButtonState nudge;
   ButtonState pause;
   ButtonState swap;

   ButtonState power;
};

struct ButtonHoldCount {
   int left = 0;
   int right = 0;
   int up = 0;
   int down = 0;

   int nudge = 0;
   int pause = 0;
   int swap = 0;

   int power = 0;
};

void processInputs(Game* game);
void inputProcessKeyboard(Game* game);
void inputProcessController(Game* game);
void resetButtonStates(Game* game);

void controllerAdd(int id);
void controllerRemove(int id);

void controllerGetAll();
int controllerCloseAll();
