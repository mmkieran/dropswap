
#include <stdio.h>
#include <chrono>
#include <SDL.h>
#include <SDL_ttf.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

#include "game.h"
#include "board.h"
#include "resources.h"
#include "cursor.h"
#include "tile.h"
#include "render.h"
#include "mymath.h"
#include "garbage.h"

struct GameWindow {
   SDL_Window *window;
   SDL_GLContext gl_context;

   unsigned int VAO;  //This doesn't really belong here

   TTF_Font* font;
};


bool createGameWindow(Game* game, const char* title, int xpos, int ypos, int width, int height) {
   SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

   game->sdl->window = SDL_CreateWindow(title, xpos, ypos, width, height, window_flags);
   if (!game->sdl->window) {
      printf("Failed to create SDL window...\n");
      return false;
   }

   game->sdl->gl_context = SDL_GL_CreateContext(game->sdl->window);
   SDL_GL_MakeCurrent(game->sdl->window, game->sdl->gl_context);
   SDL_GL_SetSwapInterval(1); // Enable vsync

   return true;
}

void imguiSetup(Game* game) {
   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   //game->io = &ImGui::GetIO(); (void)game->io;
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();
   //ImGui::StyleColorsClassic();

   // Setup Platform/Renderer bindings
   const char* glsl_version = "#version 130";
   ImGui_ImplSDL2_InitForOpenGL(game->sdl->window, game->sdl->gl_context);
   ImGui_ImplOpenGL3_Init(glsl_version);

}

void imguiStartFrame(Game* game) {
   // Start the Dear ImGui frame
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplSDL2_NewFrame(game->sdl->window);
   ImGui::NewFrame();
}

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
   Game* game = new Game;
   game->sdl = new GameWindow;

   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
      printf("Failed to initialize SDL...\n");
      return nullptr;
   }
   else {printf("Initialized SDL...\n"); }
   
   if (TTF_Init() != 0) {printf("Failed to initialize True Text Fonts...\n"); }
   else {printf("Initialized True Text Fonts...\n"); }

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

   game->sdl->font = TTF_OpenFont("assets/arial.ttf", 14);
   if (!game->sdl->font) {printf("Couldn't load font?.\n"); }

   game->windowHeight = height;
   game->windowWidth = width;

   game->isRunning = true;
   return game;
}

void gameHandleEvents(Game* game) {
   SDL_Event event;
   SDL_PollEvent(&event);
   switch (event.type) {
   case SDL_QUIT:
      game->isRunning = false;
      break;

   case SDL_KEYDOWN:
      if (game->paused == false && game->playing == true) {
         switch (event.key.keysym.sym) {

            case SDLK_LEFT:
               cursorMove(game->board, move_left);
               break;

            case SDLK_RIGHT:
               cursorMove(game->board, move_right);
               break;

            case SDLK_UP:
               cursorMove(game->board, move_up);
               break;

            case SDLK_DOWN:
               cursorMove(game->board, move_down);
               break;

            case SDLK_SPACE:
               boardSwap(game->board);
               break;

            case SDLK_r:
               if (!game->board->paused) {
                  boardMoveUp(game->board, 8.0f);
                  break;
               }
               break;

            case SDLK_g:
               garbageCreate(game->board, game->timer % 3 + 3, game->timer % 2 + 1);
               break;

            case SDLK_a:
               makeItRain(game->board);
               break;
         }
      }
   }
}

void gameUpdate(Game* game) {

   boardUpdate(game->board);
}

void gameRender(Game* game) {

   rendererClear(0.0, 0.0, 0.0, 0.0);

   int width, height;
   SDL_GetWindowSize(game->sdl->window, &width, &height);

   rendererSetTarget(0, 0, width, height);  //Gotta remember if the window resizes to resize everything

   ////Do this if we want the meshes to stay the same size when then window changes...
   //worldToDevice(game, 0.0f, 0.0f, width, height);

   //Draw game objects
   if (game->playing == true) {
      boardRender(game, game->board);
      debugCursor(game);  //imgui debug tools
   }

   ImGui::Render();

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
   SDL_GL_SwapWindow(game->sdl->window);

}

void gameDestroy(Game* game) {

   TTF_CloseFont(game->sdl->font);  //free the font

   if (game->board) {
      boardDestroy(game->board);
   }

   destroyResources(game->resources);

   vaoDestroy(game->sdl->VAO);

   //imgui stuff to shutdown
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   ImGui::DestroyContext();
   SDL_GL_DeleteContext(game->sdl->gl_context);

   //SDL cleanup
   SDL_DestroyWindow(game->sdl->window);
   TTF_Quit();  //close ttf
   SDL_Quit();
   delete game->sdl;
   delete game;

   printf("Cleanup successful.\n");
}

bool gameRunning(Game* game) {
   return game->isRunning;
}

void debugGarbage(Game* game) {
   //ImGui debug window

   ImGui::Begin("Debug Garbage");

   for (int row = 0; row < game->board->endH; row++) {
      for (int col = 0; col < game->board->w; col++) {
         Tile* tile = boardGetTile(game->board, row, col);
         if (tile->type == tile_garbage && tile->garbage) {
            Tile* above = boardGetTile(game->board, row - 1, col);
            ImGui::Text("%0.1f x, %0.1f y, ptr %d, fall %d", above->xpos, above->ypos, above->garbage, above->falling);
            ImGui::Text("%0.1f x, %0.1f y, ptr %d, fall %d", tile->xpos, tile->ypos, tile->garbage, tile->falling);
         }
      }
   }

   ImGui::End();
}

void debugCursor(Game* game) {
   ImGui::Begin("Cursor Debug");

   int row = cursorGetRow(game->board);
   int col = cursorGetCol(game->board);

   Tile* tile = boardGetTile(game->board, row, col);
   if (meshGetTexture(tile->mesh) != Texture_empty) {
      //ImGui::Image((void*)(intptr_t)tile->mesh->texture->handle, { 64, 64 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
   }

   ImGui::Text("%d row, %d col", row, col);
   ImGui::NewLine();

   ImGui::Text("%d combo", game->board->combo);
   ImGui::Text("%.1f offset", game->board->offset);
   ImGui::Text("%.1f level", game->board->level);
   ImGui::Text("%d time", game->timer);

   ImGui::End();
}

void imguiShowDemo() {
   bool show = true;
   ImGui::ShowDemoWindow(&show);
}

void showGameMenu(Game* game) {

   if (!ImGui::Begin("Game Menus")) {
      ImGui::End();
      return;
   }

   if (game->playing == false) {
      if (ImGui::Button("Start Game")) {

         //setting up board
         game->board = boardCreate(game);
         boardFillTiles(game->board);

         game->playing = true;
      }
   }
   if (game->playing == true) {
      if (ImGui::Button("End Game")) {

         game->board = boardDestroy(game->board);
         game->board = nullptr;
         game->playing = false;
      }
   }

   if (game->paused == true) {
      if (ImGui::Button("Unpause Game")) {
         game->paused = false;
      }
   }
   else if (game->paused == false) {
      if (ImGui::Button("Pause Game")) {
         game->paused = true;
      }
   }

   ImGui::Button("Load Board");

   ImGui::Button("Save Board");
   if (ImGui::Button("Clear Board")) {
      if (game->playing == true) {
         boardClear(game->board);
      }
   }

   if (ImGui::Button("Make it rain") ) {
      if (game->playing == true) {
         makeItRain(game->board);
      }
   }

   if (game->playing == true) {
      float minSpeed = 0;
      float maxSpeed = 8.0;
      float speed = game->board->speed;

      ImGui::SliderScalar("Fall Speed", ImGuiDataType_Float, &speed, &minSpeed, &maxSpeed);
      game->board->speed = speed;

      float minLevel = 0;
      float maxLevel = 10.0;
      float level = game->board->level;
      ImGui::SliderScalar("Board Speed", ImGuiDataType_Float, &level, &minLevel, &maxLevel);
      game->board->level = level;
   }

   ImGui::End();
}