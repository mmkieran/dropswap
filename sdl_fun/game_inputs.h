#pragma once

typedef struct Game Game;
typedef unsigned long long uint64_t;

struct ButtonState {
   bool p = false;  //pressed
   bool h = false;  //held
   int fc = 0;      // frame count //todo... belongs in gamestate?
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

   int msg = 0;
   int timer = 0;
};


void inputProcessKeyboard(Game* game);