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

   int bHeight = 12;
   int bWidth = 6;

   int tWidth = 64;
   int tHeight = 64;

   bool isRunning = false;
   SDL_Window *window;

   bool paused = false;
   int pauseTimer = 0;
   int pauseLength = 0;

   int timer = 0;
   int timeDelta = 0;
};

//void startTimer(int time);

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
bool gameRunning(Game* game);

void gameLoadTextures(Game* game);

void gameHandleEvents(Game* game);

void gameUpdate(Game* game);
void gameRender(Game* game);

void gameDestroy(Game* game);