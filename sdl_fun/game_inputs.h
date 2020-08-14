#pragma once

typedef struct Game Game;
typedef unsigned long long uint64_t;

struct ButtonState {
   bool p = false;  //pressed
   bool h = false;  //held
   int fc = 0;      //frame count
};

struct UserInput {
   ButtonState left;
   ButtonState right;
   ButtonState up;
   ButtonState down;

   ButtonState pause;
   ButtonState swap;

   ButtonState power;

   uint64_t msg;
};


void inputProcessKeyboard(Game* game);