
#include <stdio.h>
#include <time.h>
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
   game->boards = vectorCreate<Board*>(4, 10);
   game->seed = time(0);  //generate the random seed for the board tiles

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

         for (int i = 1; i <= vectorSize(game->boards); i++) {
            Board* board = vectorGet(game->boards, i);
            if (i == 1) {
               switch (event.key.keysym.sym) {
                  case SDLK_LEFT:
                     cursorMove(board, move_left);
                     break;

                  case SDLK_RIGHT:
                     cursorMove(board, move_right);
                     break;

                  case SDLK_UP:
                     cursorMove(board, move_up);
                     break;

                  case SDLK_DOWN:
                     cursorMove(board, move_down);
                     break;

                  case SDLK_SPACE:
                     boardSwap(board);
                     break;

                  case SDLK_r:
                     if (!board->paused) {
                        boardMoveUp(board, 8.0f);
                        break;
                     }
                     break;

                  case SDLK_g:
                     garbageCreate(board, game->timer % 3 + 3, game->timer % 2 + 1);
                     break;

                  case SDLK_t:
                     makeItRain(board);
                     break;
                  }
               }

               if (i > 1) {
                  switch (event.key.keysym.sym) {
                  case SDLK_a:
                     cursorMove(board, move_left);
                     break;

                  case SDLK_d:
                     cursorMove(board, move_right);
                     break;

                  case SDLK_w:
                     cursorMove(board, move_up);
                     break;

                  case SDLK_s:
                     cursorMove(board, move_down);
                     break;

                  case SDLK_RETURN:
                     boardSwap(board);
                     break;

                  case SDLK_r:
                     if (!board->paused) {
                        boardMoveUp(board, 8.0f);
                        break;
                     }
                     break;

                  case SDLK_g:
                     garbageCreate(board, game->timer % 3 + 3, game->timer % 2 + 1);
                     break;

                  case SDLK_t:
                     makeItRain(board);
                     break;
               }
            }
         }
      }
   }
}

void gameUpdate(Game* game) {
   for (int i = 1; i <= vectorSize(game->boards); i++) {
      boardUpdate(vectorGet(game->boards, i));
   }
}

void gameRender(Game* game) {

   rendererClear(0.0, 0.0, 0.0, 0.0);

   int width, height;
   SDL_GetWindowSize(game->sdl->window, &width, &height);

   rendererSetTarget(0, 0, width, height);  //Gotta remember if the window resizes to resize everything

   //Do this if we want the meshes to stay the same size when then window changes...
   worldToDevice(game, 0.0f, 0.0f, width, height);

   //Draw game objects
   if (game->playing == true) {
      for (int i = 1; i <= vectorSize(game->boards); i++) {
         Board* board = vectorGet(game->boards, i);
         if (board) {
            boardRender(game, board);
         }
      }
      debugCursor(game);  //imgui debug tools
   }

   ImGui::Render();

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
   SDL_GL_SwapWindow(game->sdl->window);

}

void gameDestroy(Game* game) {

   TTF_CloseFont(game->sdl->font);  //free the font

   for (int i = 1; i <= vectorSize(game->boards); i++) {
      Board* board = vectorGet(game->boards, i);
      if (board) {
         boardDestroy(board);
      }
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

   Board* board = vectorGet(game->boards, 0);
   for (int row = 0; row < board->endH; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_garbage && tile->garbage) {
            Tile* above = boardGetTile(board, row - 1, col);
            ImGui::Text("%0.1f x, %0.1f y, ptr %d, fall %d", above->xpos, above->ypos, above->garbage, above->falling);
            ImGui::Text("%0.1f x, %0.1f y, ptr %d, fall %d", tile->xpos, tile->ypos, tile->garbage, tile->falling);
         }
      }
   }

   ImGui::End();
}

void debugCursor(Game* game) {
   if (game->playing == true) {
      ImGui::Begin("Cursor Debug");

      Board* board = vectorGet(game->boards, 1);
      if (board) {
         int row = cursorGetRow(board);
         int col = cursorGetCol(board);

         Tile* tile = boardGetTile(board, row, col);
         if (meshGetTexture(tile->mesh) != Texture_empty) {
            //ImGui::Image((void*)(intptr_t)tile->mesh->texture->handle, { 64, 64 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
         }

         ImGui::Text("%d row, %d col", row, col);
         ImGui::NewLine();

         ImGui::Text("%d combo", board->combo);
         ImGui::Text("%.1f offset", board->offset);
         ImGui::Text("%.1f level", board->level);
         ImGui::Text("%d time", game->timer);
      }

      ImGui::End();
   }
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

   ImGui::InputInt("Players", &game->players);

   ImGui::InputInt("Tile Width", &game->tWidth, 16);
   ImGui::InputInt("Tile Height", &game->tHeight, 16);

   ImGui::InputInt("Board Width", &game->bWidth);
   ImGui::InputInt("Board Height", &game->bHeight);


   if (game->playing == false) {
      if (ImGui::Button("Start Game")) {

         //setting up board
         for (int i = 1; i <= game->players; i++) {
            Board* board = boardCreate(game);
            vectorPushBack(game->boards, board);
            boardFillTiles(board);

            //todo add a smarter algorithm to tile boards in screen space if more than 2
            float xOrigin = game->tWidth * game->bWidth * (i - 1) + game->tWidth * i;
            float yOrigin = game->tHeight;

            if (i > 2) {
               xOrigin = game->tWidth * game->bWidth * (i - 3) + game->tWidth * (i - 2);
               yOrigin += game->tHeight * game->bHeight + game->tHeight * 2;
            }

            board->origin = {xOrigin, yOrigin};
         }

         game->playing = true;
      }
   }
   if (game->playing == true) {
      if (ImGui::Button("End Game")) {
         for (int i = 1; i <= vectorSize(game->boards); i++) {
            Board* board = vectorGet(game->boards, i);
            boardDestroy(board);
            vectorClear(game->boards);
            game->playing = false;
         }
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
         for (int i = 1; i <= vectorSize(game->boards); i++) {
            Board* board = vectorGet(game->boards, i);
            boardClear(board);
         }
      }
   }

   if (ImGui::Button("Make it rain") ) {
      if (game->playing == true) {
         for (int i = 1; i <= vectorSize(game->boards); i++) {
            Board* board = vectorGet(game->boards, i);
            makeItRain(board);
         }
      }
   }

   if (game->playing == true) {
      for (int i = 1; i <= vectorSize(game->boards); i++) {
         Board* board = vectorGet(game->boards, i);
         float minFallSpeed = 0;
         float maxFallSpeed = 8.0;

         ImGui::SliderScalar("Fall Speed", ImGuiDataType_Float, &board->fallSpeed, &minFallSpeed, &maxFallSpeed);

         float minBoardSpeed = 0;
         float maxBoardSpeed = 10.0;
         ImGui::SliderScalar("Board Speed", ImGuiDataType_Float, &board->moveSpeed, &minBoardSpeed, &maxBoardSpeed);
      }
   }

   ImGui::End();
}