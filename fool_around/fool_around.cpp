// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <random>
#include <stdio.h>
#include <fstream>
#include <iostream>

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
};

int main(int argc, char* args[])
{

   printf("Size of inputs: %d", sizeof(UserInput));

   return 0;
}