/*Kieran McDonald 08/23/2020
* This game is for educational purposes only
* It's completely for fun!
*/

#include <stdio.h>
#include <time.h>
#include <SDL.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

#include "game.h"
#include "board.h"
#include "resources.h"
#include "mymath.h"
#include "serialize.h"
#include "game_inputs.h"
#include "netplay.h"
#include "sounds.h"

#define GAME_COUNTIN 2000

struct GameWindow {
   SDL_Window* window;
   SDL_GLContext gl_context;

   unsigned int VAO;
};

//Creates ImGui context
void imguiSetup(Game* game) {
   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   game->fonts[13] = io.Fonts->AddFontFromFileTTF("assets/arial.ttf", 13);
   game->fonts[20] = io.Fonts->AddFontFromFileTTF("assets/arial.ttf", 20);
   game->fonts[30] = io.Fonts->AddFontFromFileTTF("assets/arial.ttf", 30);
   io.Fonts->Build();

   //Use these for Keyboard and controller navigation
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();
   //ImGui::StyleColorsClassic();

   // Setup Platform/Renderer bindings
   const char* glsl_version = "#version 130";
   ImGui_ImplSDL2_InitForOpenGL(game->sdl->window, game->sdl->gl_context);
   ImGui_ImplOpenGL3_Init(glsl_version);

}

//Tell ImGui we started another Frame
void imguiStartFrame(Game* game) {
   // Start the Dear ImGui frame
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplSDL2_NewFrame(game->sdl->window);
   ImGui::NewFrame();
}

//SDL function wrapper for getting ticks
uint64_t sdlGetCounter() {
   uint64_t current = SDL_GetPerformanceCounter();
   return current;
}

//SDL function wrapper for delay
void sdlSleep(int delay) {
   SDL_Delay(delay);  //milliseconds
}

//Give extra frame time to GGPO so it can do it's thing
void gameGiveIdleToGGPO(Game* game, int time) {
   if (game->net && game->net->ggpo && time > 0) {
      ggpo_idle(game->net->ggpo, time);
   }
}

//Calculate how long we have to wait to get a steady frame rate
void gameDelayFrame(Game* game, uint64_t end, uint64_t start) {

   uint64_t frameTime = end - start;
   uint64_t leftover = 0;

   if (game->kt.delay > frameTime) {
      leftover = (game->kt.delay - frameTime) / 1000 - 2;
      if (leftover > 0 && leftover < 15) {
         if (game->players > 1) {
            gameGiveIdleToGGPO(game, leftover);  //Give some time to GGPO, but leave to wait out frame
         }
         else { sdlSleep(leftover); }
      }
   }
   if (game->vsync != 0) {  //We need to control frame rate if vsync fails
      uint64_t newTime = game->kt.getTime() - start;
      while (game->kt.delay > newTime) { 
         newTime = game->kt.getTime() - start;
      }
   }
   else { gameSwapWindow(game); }
   game->kt.fps = (game->kt.getTime() - start) / 1000.0; 
}

void gameSwapWindow(Game* game) {
   SDL_GL_SwapWindow(game->sdl->window);
}

//Create the SDL Window for the game
bool createGameWindow(Game* game, const char* title, int xpos, int ypos, int width, int height) {
   SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

   game->sdl->window = SDL_CreateWindow(title, xpos, ypos, width, height, window_flags);
   if (!game->sdl->window) {
      printf("Failed to create SDL window...\n");
      return false;
   }

   game->sdl->gl_context = SDL_GL_CreateContext(game->sdl->window);
   SDL_GL_MakeCurrent(game->sdl->window, game->sdl->gl_context);
   game->vsync = SDL_GL_SetSwapInterval(1); // Gotta be careful about frame timing with vsync... thanks Sean!
   //game->vsync = -1;  //debug no vsync

   return true;
}

void sdlSetVsync(Game* game, bool toggle) {
   if (toggle) {
      game->vsync = SDL_GL_SetSwapInterval(1);
   }
   else {
      SDL_GL_SetSwapInterval(0);
      game->vsync = -1;
   }
}

//Create the game object and all that entails
Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
   Game* game = new Game;
   game->sdl = new GameWindow;
   game->net = new NetPlay;

   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
      printf("Failed to initialize SDL...\n");
      return nullptr;
   }
   else {printf("Initialized SDL...\n"); }

   if (openglContext() != 0) {printf("Failed to create OpenGL Context?...\n"); }
   else {printf("Created OpenGL Context...\n"); }

   if (!createGameWindow(game, title, xpos, ypos, width, height)) {
      printf("Failed to create SDL window...\n");
      return nullptr;
   }
   else {printf("Created SDL window...\n"); }

   if (gl3wInit() != 0) {
      printf("Failed to initialize gl3w...\n");
      return nullptr;
   }
   else {printf("Initialized gl3w...\n"); }

   //Load game resources
   game->resources = initResources();

   //Make a vertex array object... stores the links between attributes and vbos
   game->sdl->VAO = vaoCreate();

   //initialize IMGUI
   imguiSetup(game);

   //Use the Shader Program once it's created in resources
   shaderUseProgram(resourcesGetShader(game));

   //Set the projection matrices to change origin to world and then to device coordinates
   originToWorld(game, 0.0f, 0.0f, width, height);
   worldToDevice(game, 0.0f, 0.0f, width, height);

   game->windowHeight = height;
   game->windowWidth = width;

   game->seed = 0;  //generate the random seed for the board tiles

   game->isRunning = true;

   controllerGetAll();  //Find any attached controllers
   soundsInit();  //Initialize SoLoud components

   return game;
}

//Pass SDL Events to ImGui and detect if main window closes
void gameHandleEvents(Game* game) {
   SDL_Event event;

   while (SDL_PollEvent(&event)) {

      ImGui_ImplSDL2_ProcessEvent(&event);  //Hand events to ImGui
      if (event.type == SDL_QUIT) {
         game->isRunning = false;
      }
      if (event.type == SDL_KEYDOWN) {
         if (event.key.keysym.sym == SDLK_ESCAPE) { 
            if (game->players == 1 && game->ai == true) { gameEndMatch(game); }
            else { game->paused = true; }
         }
      }

      if (event.cdevice.type == SDL_CONTROLLERDEVICEADDED) {  //Check for controllers being removed/added
         int a = event.cdevice.which;
         controllerAdd(event.cdevice.which);
      }
      if (event.cdevice.type == SDL_CONTROLLERDEVICEREMOVED) {
         int a = event.cdevice.which;
         controllerRemove(event.cdevice.which);
      }
   }

}

//Check the boards to see if it's game over
bool gameCheckBust(Game* game) {
   for (int i = 0; i < game->boards.size(); i++) {
      if (game->boards[i]->bust == true) {
         return true;
      }
   }
   return false;
}

//Detect if any player paused the game
void gameCheckPause(Game* game, UserInput input) {
   if (input.pause.p == true) {
      if (game->paused == true) {
         game->paused = false;
      }
      else if (game->paused == false) {
         game->paused = true;
      }
   }
}

//Update the board state for all players
void gameUpdate(Game* game) {
   if (game->playing == false || game->paused == true) { return; }
   for (int i = 0; i < game->boards.size(); i++) {
      if (game->boards[i] == nullptr) { continue; }
      boardUpdate(game->boards[i]);
   }
   game->frameCount++;
   game->timer = game->frameCount * (1000.0f / 60.0f);
}

void gameSinglePlayer(Game* game) {
   if (game->playing == false) { return; }
   processInputs(game);
   if (game->ai == true) { gameAI(game, 0); }  //debug
   gameCheckPause(game, game->p1Input);
   gameUpdate(game);
}

//Create the boards and set playing to true
void gameStartMatch(Game* game) {
   if (game->players == 1) { game->seed = time(0); }

   for (int i = 0; i < game->fbos.size(); i++) {
      if (game->fbos[i]) {
         rendererDestroyFBO(game->fbos[i]);
      }
   }
   game->fbos.clear();

   int players = game->players;
   if (players > 2) { players = 2; }
   for (int i = 0; i < players; i++) {
      Board* board = boardCreate(game);
      if (board) {
         board->team = i + 1;  
         board->pauseLength = GAME_COUNTIN;
         board->paused = true;
         boardFillTiles(board);

         game->boards.push_back(board);

         FBO* fbo = rendererCreateFBO(game);  //Create Framebuffer Object
         if (fbo) { game->fbos.push_back(fbo); }
      }
   }
   game->playing = true;
   game->frameCount = 0;
   game->timer = 0;
   //game->soundToggles[sound_waltz] = true;
}

//Destroy the boards and set playing to false
void gameEndMatch(Game* game) {
   for (int i = 0; i < game->boards.size(); i++) {
      if (game->boards[i]) {
         boardDestroy(game->boards[i]);
      }
   }
   game->boards.clear();
   game->playing = false;
   game->paused = false;
   game->frameCount = 0;
   game->timer = 0;
   soundsStopAll();
   if (game->players > 1) {
      ggpoEndSession(game);
   }
}

//Call the draw function for all the boards
void gameRender(Game* game) {
   //Remember that this has to happen after ImGui, or the clear will remove everything...

   //Play sounds here because of GGPO
   static bool silence = false;
   static bool dangerPlaying = false;
   static int anxietyHandle;
   if (game->sounds == 0) {
      for (auto&& pair : game->soundToggles) {
         SoundEffect sound = pair.first;
         if (pair.first == sound_anxiety && pair.second == true) {
            //soundsStopAll();
            if (dangerPlaying == false) {
               anxietyHandle = soundsPlaySound(game, sound);
               dangerPlaying = true;
            }
            silence = true;
         }
         else if (pair.first == sound_anxiety && pair.second == false) {
            silence = false;
            dangerPlaying = false;
            soundsStopSound(anxietyHandle);
         }
         if (pair.second == true && silence == false) { soundsPlaySound(game, sound); }
         game->soundToggles[sound] = false;
      }
   }

   //Draw game objects
   if (game->playing == true) {
      int i = 0;
      for (auto&& board : game->boards) {
         if (game->fbos[i]) {
            rendererEnableFBO(game->fbos[i]);
            if (board) {
               boardRender(game, board);
            }
         }
         i++;
      }
      rendererDisableFBO();
      //debugCursor(game);
   }
}

//Draw the ImGui windows and the game objects
void imguiRender(Game* game) {

   //int width, height;
   //SDL_GetWindowSize(game->sdl->window, &width, &height);

   int boardWidth = game->tWidth * game->bWidth;
   int boardHeight = game->tHeight * game->bHeight;
   rendererSetTarget(0, 0, boardWidth, boardHeight);  

   //Do this if we want the meshes to stay the same size when then window changes...
   worldToDevice(game, 0.0f, 0.0f, boardWidth, boardHeight);

   if (game->playing == false || game->paused == true) { mainUI(game); }
   gameRender(game);  //Draw all game objects

   rendererSetTarget(0, 0, NULL, NULL);
   rendererClear(0.0, 0.0, 0.0, 0.0);

   boardUI(game);  
   ImGui::Render();

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
   if (game->vsync != 0) { SDL_GL_SwapWindow(game->sdl->window); }
}

//Jokulhaups
void gameDestroy(Game* game) {

   for (auto&& board : game->boards) {
      if (board) { boardDestroy(board); }
   }

   if (game->net && game->net->ggpo) {
      ggpoClose(game->net->ggpo);
      game->net->ggpo = nullptr;
   }

   destroyResources(game->resources);

   for (int i = 0; i < game->fbos.size(); i++) {
      if (game->fbos[i]) {
         rendererDestroyFBO(game->fbos[i]);
      }
   }
   vaoDestroy(game->sdl->VAO);
   soundsDestroy();

   //imgui stuff to shutdown
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   ImGui::DestroyContext();
   SDL_GL_DeleteContext(game->sdl->gl_context);

   //SDL cleanup
   SDL_DestroyWindow(game->sdl->window);
   SDL_Quit();
   delete game->net;
   delete game;

   printf("Cleanup successful.\n");
}

//Returns true if the game is running... used for game loop
bool gameRunning(Game* game) {
   return game->isRunning;
}

void gameAI(Game* game, int player) {
   if (game->players <= 2) {
      boardAI(game->boards[player], player);
   }
   else if (game->players > 2) {
      boardAI(game->boards[player / 2], player);
   }
}