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


int main(int argc, char* args[])
{

   printf("%d", sizeof(UserInput));

   return 0;
}