// fool_around.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <unordered_map>

enum TileEnum {
   tile_empty = 0,
   tile_circle,
   tile_diamond,
   tile_utriangle,
   tile_dtriangle,
   tile_star,
   tile_heart,
   tile_silver,
   tile_garbage
};

struct Tile {

   TileEnum type;

   int xpos;
   int ypos;

   bool falling;

   std::unordered_map <TileEnum, const char*> textures;
};

int main()
{
    std::cout << "Hello World!\n"; 

    int height = 12;
    int width = 6;

    Tile* tiles = (Tile*)malloc(sizeof(Tile) * height * width);
    Tile* tile1 = (tiles + 1);
    Tile* tile2 = (tiles + 11);

    while (tile1 + 5 < tile2) {
       //printf("%d\n", tile1);
       tile1 += 1;
    }

    std::unordered_map <TileEnum, const char*> textures;
    textures[tile_empty] = "empty";
    printf("%s\n", textures[tile_empty] );

    //tiles[0].textures = std::unordered_map <TileEnum, const char*>();
    //tiles[0].textures[tile_heart] = "heart";
    //printf("%s\n", textures[tile_heart]);

    //printf("%d\n", tile1);
    //printf("%d\n", tile2);
    //printf("%d\n", tile2 - tile1);
    //printf("%d\n", tile2 > tile1 + 11);
    //std::cout << "Pointer" << ptr << std::endl;

    free(tiles);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
