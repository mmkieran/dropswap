#pragma once

typedef struct Game Game;

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
};


void inputProcessKeyboard(Game* game);