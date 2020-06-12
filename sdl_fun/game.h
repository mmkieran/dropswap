#pragma once

#include <SDL.h>
#include <random>
#include <atomic>
#include <unordered_map>

struct Board;
enum TileEnum;

struct Game {

   SDL_Renderer *renderer;
   Board* board;

   //std::unordered_map<const char*, SDL_Texture*> textures;
   std::vector <SDL_Texture*> textures;

   int bHeight;
   int bWidth;

   int tWidth;
   int tHeight;

   bool isRunning;
   SDL_Window *window;
};

//void startTimer(int time);

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
bool gameRunning(Game* game);

void gameLoadTextures(Game* game);

void gameHandleEvents(Game* game);

void gameUpdate(Game* game);
void gameRender(Game* game);

void gameDestroy(Game* game);