
#include <stdio.h>
#include <chrono>

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


bool createGameWindow(Game* game, const char* title, int xpos, int ypos, int width, int height) {
   SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

   game->window = SDL_CreateWindow(title, xpos, ypos, width, height, window_flags);
   if (!game->window) {
      printf("Failed to create SDL window...\n");
      return false;
   }

   game->gl_context = SDL_GL_CreateContext(game->window);
   SDL_GL_MakeCurrent(game->window, game->gl_context);
   SDL_GL_SetSwapInterval(1); // Enable vsync

   return true;
}

void setupImGui(Game* game) {
   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   game->io = &ImGui::GetIO(); (void)game->io;
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();
   //ImGui::StyleColorsClassic();

   // Setup Platform/Renderer bindings
   const char* glsl_version = "#version 130";
   ImGui_ImplSDL2_InitForOpenGL(game->window, game->gl_context);
   ImGui_ImplOpenGL3_Init(glsl_version);

}

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
   Game* game = new Game;

   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
      printf("Failed to initialize SDL...\n");
      return nullptr;
   }
   else {
      printf("Initialized SDL...\n");
   }
   
   if (TTF_Init() != 0) {
      printf("Failed to initialize True Text Fonts...\n");
   }
   else {
      printf("Initialized True Text Fonts...\n");
   }

   if (openglContext() != 0) {
      printf("Failed to create OpenGL Context?...\n");
   }
   else {
      printf("Created OpenGL Context...\n");
   }

   if (!createGameWindow(game, title, xpos, ypos, width, height)) {
      printf("Failed to create SDL window...\n");
      return nullptr;
   }
   else {
      printf("Created SDL window...\n");
   }

   if (gl3wInit() != 0) {
      printf("Failed to initialize gl3w...\n");
      return nullptr;
   }
   else {
      printf("Initialized gl3w...\n");
   }

   //Load game resources
   game->resources = initResources();

   //Make a vertex array object... stores the links between attributes and vbos
   game->VAO = createVAO();

   //initialize IMGUI
   setupImGui(game);

   //Use the Shader Program once it's created in resources
   useShaderProgram(resourcesGetShader(game));

   //Set the projection matrices to change origin to world and then to device coordinates
   originToWorld(game, 0.0f, 0.0f, width, height);
   worldToDevice(game, 0.0f, 0.0f, width, height);

   game->font = TTF_OpenFont("assets/arial.ttf", 14);
   if (!game->font) {
      printf("Couldn't load font?.\n");
   }

   //Setting up the game settings
   game->bHeight = 12;
   game->bWidth = 6;

   game->tWidth = 64;
   game->tHeight = 64;

   game->windowHeight = height;
   game->windowWidth = width;

   game->timer = 0;

   //setting up board
   game->board = boardCreate(game);
   game->board->game = game;
   game->board->cursor = cursorCreate(game->board, (game->bWidth / 2 - 1) * game->tWidth, (game->bHeight / 2 + 1) * game->tHeight);

   game->board->paused = false;
   game->board->pauseLength = 0;

   //todo: Use premade boards or fix algorithm so there are no matches at the start
   boardFillTiles(game->board);

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
            Garbage* garbage = garbageCreate(game->board, 6, 2);
            game->board->garbage.push_back(garbage);  //debug
            break;
         }
   }
}

void gameUpdate(Game* game) {

   // Start the Dear ImGui frame
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplSDL2_NewFrame(game->window);
   ImGui::NewFrame();

   boardRemoveClears(game->board);
   if (game->board->pauseLength > 0) {
      game->board->pauseLength -= game->timeDelta;

      if (game->board->pauseLength < 0) {
         game->board->paused = false;
         game->board->pauseLength = 0;
      }
   }
   else {
      game->board->paused = false;
   }

   //Update board
   boardUpdateArray(game->board, false);

   if (game->timer > 2000) {
      if (game->board->paused == false) {
         boardMoveUp(game->board, game->board->speed/8.0f);
      }
   }

   if (game->board->bust) {
      game->isRunning = false;
   }

   boardUpdateFalling(game->board, 8.0f);
   garbageFall(game->board, 8.0f);

   cursorUpdate(game->board);  //todo make this do something more

}

void gameRender(Game* game) {

   clearRenderer(0.0, 0.0, 0.0, 0.0);

   int width, height;
   SDL_GetWindowSize(game->window, &width, &height);

   setRenderTarget(0, 0, width, height);  //Gotta remember if the window resizes to resize everything

   ////Do this if we want the meshes to stay the same size when then window changes...
   //worldToDevice(game, 0.0f, 0.0f, width, height);

   //Draw game objects
   boardRender(game, game->board);
   garbageDraw(game->board);

   //ImGui debug

   ImGui::Begin("Drop and Swap");

   for (int row = 0; row < game->board->endH; row++) {
      for (int col = 0; col < game->board->w; col++){
         Tile* tile = boardGetTile(game->board, row, col);
         if (tile->type == tile_garbage && tile->garbage) {
            Tile* above = boardGetTile(game->board, row-1, col);
            ImGui::Text("%0.1f x, %0.1f y, ptr %d, fall %d", above->xpos, above->ypos, above->garbage, above->falling);
            ImGui::Text("%0.1f x, %0.1f y, ptr %d, fall %d, garbage %d", tile->xpos, tile->ypos, tile->garbage, tile->falling, tile->garbage->falling);
         }
      }
   }

   int row = yPosToRow(game->board, cursorGetY(game->board->cursor));
   int col = xPosToCol(game->board, cursorGetX(game->board->cursor));

   Tile* tile = boardGetTile(game->board, row, col);
   if (tile->mesh->texture) {
      ImGui::Image((void*)(intptr_t)tile->mesh->texture->handle, { 64, 64 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
   }

   ImGui::Text("Cursor x %0.1f, y %0.1f", game->board->cursor->x, game->board->cursor->y);
   ImGui::Text("%d col, %d row", col, row);
   ImGui::NewLine();

   //Tile* bufferTile = boardGetTile(game->board, 24, 2);
   //ImGui::Text("%.1f col, %.1f row", bufferTile->ypos, tile->ypos);

   ImGui::Text("%d combo", game->board->combo);
   ImGui::Text("%.1f offset", game->board->offset);

   ImGui::End();

   ImGui::Render();

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
   SDL_GL_SwapWindow(game->window);

}

void gameDestroy(Game* game) {

   TTF_CloseFont(game->font);  //free the font

   boardDestroy(game->board);

   destroyResources(game->resources);

   destroyVAO(game->VAO);

   //imgui stuff to shutdown
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   ImGui::DestroyContext();
   SDL_GL_DeleteContext(game->gl_context);

   //SDL cleanup
   SDL_DestroyWindow(game->window);
   TTF_Quit();  //close ttf
   SDL_Quit();
   delete game;

   printf("Cleanup successful.\n");
}

bool gameRunning(Game* game) {
   return game->isRunning;
}