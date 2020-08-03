// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <random>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include "sdl_fun/myvector.h"
#include "sdl_fun/serialize.h"
#include "sdl_fun/tile.h"


int main(int argc, char* args[])
{
   printf("%d\n", sizeof(uint64_t));
   printf("%d\n", sizeof(int));
   printf("%d\n", sizeof(long long unsigned int));

   uint64_t seed = 0;
   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution;

   FILE* file;
   std::ofstream fout("seed.dat");

   std::cout << generator;

   return 0;
}