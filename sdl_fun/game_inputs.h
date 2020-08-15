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

   ButtonState pause;
   ButtonState swap;

   ButtonState power;

   uint64_t msg = 0;
   unsigned short code = 0;
   unsigned short handle = 0;
   int timer = 0;
};


void inputProcessKeyboard(Game* game);