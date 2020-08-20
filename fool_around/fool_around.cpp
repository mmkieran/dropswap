// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <random>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include "sdl_fun/myvector.h"

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


int main(int argc, char* args[])
{

   printf("%d", sizeof(UserInput));
   printf("%d", sizeof(int));

   return 0;
}