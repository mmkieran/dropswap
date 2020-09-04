// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <random>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include "sdl_fun/myvector.h"

typedef unsigned char Byte;

struct ButtonState {
   bool p = false;  //pressed
   bool h = false;  //held
   Byte fc = 0;
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

   //int msg = 0;
   int timer = 0;
};

int main(int argc, char* args[])
{

   printf("Size of inputs: %d", sizeof(UserInput));

   return 0;
}