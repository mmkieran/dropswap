#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <imgui/imgui.h>

#include "texture_manager.h"

typedef struct Board Board;
typedef struct Square Square;

struct Game {

   SDL_Window *window;

   SDL_GLContext gl_context;
   ImGuiIO* io;
   Board* board;

   Square* square;
   Resources* resources;

   TTF_Font* font;

   int bHeight = 12;
   int bWidth = 6;

   int tWidth = 64;
   int tHeight = 64;

   bool isRunning = false;


   bool paused = false;
   int pauseTimer = 0;
   int pauseLength = 0;

   int timer = 0;
   int timeDelta = 0;

};

//void startTimer(int time);

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
bool gameRunning(Game* game);

void gameHandleEvents(Game* game);

void gameUpdate(Game* game);
void gameRender(Game* game);

void gameDestroy(Game* game);