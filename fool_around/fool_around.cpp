// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <vector>


//I really recommend having your own struct that looks like this!
struct Vec2
{
   float x, y;
};

struct Garbage {
   int ID;

   int width;
   int layers;

   Vec2* vec2;
};

struct Holder {
   std::vector <Garbage*> garbage;
};

Garbage* createGarbage() {
   Garbage* garbage = new Garbage;
   return garbage;
}

int main()
{
   Holder* holder = new Holder;

   for (int i = 0; i < 5; i++) {
      holder->garbage.push_back(createGarbage());
   }

   for (int i = 0; i < 5; i++) {
      printf("%d\n", holder->garbage[i]->ID);
   }

   for (int i = 0; i < 5; i++) {
      delete holder->garbage[i];
   }

   delete holder;

   printf("Worked great...\n");
}