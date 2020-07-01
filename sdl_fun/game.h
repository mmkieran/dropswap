#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <imgui/GL/gl3w/gl3w.h>
#include <gl/GL.h>

#include <imgui/imgui.h>
#include <random>
#include <atomic>
#include <unordered_map>

struct Board;

struct Game {

   SDL_Renderer *renderer;
   SDL_Window *window;
   SDL_Window *window2;
   SDL_GLContext gl_context;
   ImGuiIO* io;
   Board* board;

   //std::unordered_map<int, SDL_Texture*> textures;
   std::vector <SDL_Texture*> textures;
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

   SDL_Rect frame;
};

//void startTimer(int time);

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
bool gameRunning(Game* game);

void gameLoadTextures(Game* game);

void gameHandleEvents(Game* game);

void gameUpdate(Game* game);
void gameRender(Game* game);

void gameDestroy(Game* game);