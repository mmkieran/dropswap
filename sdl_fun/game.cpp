
#include <stdio.h>
#include <chrono>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

#include "game.h"
#include "board.h"
#include "texture_manager.h"
#include "cursor.h"
#include "tile.h"
#include "render.h"

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
   Game* game = new Game;

   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
      printf("Failed to initialize SDL...\n");
      return nullptr;
   }

   if (TTF_Init() != 0) {
      printf("Failed to initialize True Text Fonts...\n");
   }

   // Create window with graphics context
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
   SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

   game->window = SDL_CreateWindow(title, xpos, ypos, width, height, window_flags);
   game->window2 = SDL_CreateWindow(title, xpos, ypos, width, height, window_flags);
   if (!game->window) {
      printf("Failed to create SDL window...\n");
      return nullptr;
   }

   // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 Core + GLSL 150
   const char* glsl_version = "#version 150";
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
   const char* glsl_version = "#version 130";
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

   game->gl_context = SDL_GL_CreateContext(game->window2);
   SDL_GL_MakeCurrent(game->window2, game->gl_context);
   SDL_GL_SetSwapInterval(1); // Enable vsync

   if (gl3wInit() != 0) {
      printf("Failed to initialize gl3w...\n");
      return nullptr;
   }

   void openglInit();

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
   ImGui_ImplSDL2_InitForOpenGL(game->window2, game->gl_context);
   ImGui_ImplOpenGL3_Init(glsl_version);

   game->font = TTF_OpenFont("assets/arial.ttf", 14);
   if (!game->font) {
      printf("Couldn't load font?.\n");
   }

   //Setting up the game settings
   game->bHeight = 12;
   game->bWidth = 6;

   game->frame.w = game->tWidth = 64;
   game->tHeight = 64;

   game->frame.w = game->tWidth * game->bWidth;
   game->frame.h = game->tHeight * game->bHeight;
   game->frame.x = 0;
   game->frame.y = 0;

   game->timer = 0;
   //gameLoadTextures(game);

   ////setting up board
   //game->board = boardCreate(game);
   //game->board->cursor = new Cursor(game, "assets/cursor.png", (game->bWidth / 2 - 1) * game->tWidth, (game->bHeight / 2 + 1) * game->tHeight);
   //game->board->game = game;


   //game->board->paused = false;
   //game->board->pauseLength = 0;

   ////todo: Use premade boards or fix algorithm so there are no matches at the start
   //boardFillTiles(game->board);


   game->isRunning = true;
   return game;

}

void gameLoadTextures(Game* game) {

   game->textures.push_back(TextureManager::LoadTexture(game, "assets/circle.png"));
   game->textures.push_back(TextureManager::LoadTexture(game, "assets/diamond.png"));
   game->textures.push_back(TextureManager::LoadTexture(game, "assets/utriangle.png"));
   game->textures.push_back(TextureManager::LoadTexture(game, "assets/dtriangle.png"));
   game->textures.push_back(TextureManager::LoadTexture(game, "assets/star.png"));
   game->textures.push_back(TextureManager::LoadTexture(game, "assets/heart.png"));
   game->textures.push_back(TextureManager::LoadTexture(game, "assets/empty.png"));  //silver
   game->textures.push_back(TextureManager::LoadTexture(game, "assets/skull.png")); //clear

   //game->textures[(int)tile_diamond] = TextureManager::LoadTexture(game, "assets/diamond.png");
   //game->textures[(int)tile_utriangle] = TextureManager::LoadTexture(game, "assets/utriangle.png");
   //game->textures[(int)tile_dtriangle] = TextureManager::LoadTexture(game, "assets/dtriangle.png");
   //game->textures[(int)tile_star] = TextureManager::LoadTexture(game, "assets/star.png");
   //game->textures[(int)tile_circle] = TextureManager::LoadTexture(game, "assets/circle.png");
   //game->textures[(int)tile_heart] = TextureManager::LoadTexture(game, "assets/heart.png");
   //game->textures[(int)tile_silver] = TextureManager::LoadTexture(game, "assets/grass.png");
}

void gameHandleEvents(Game* game) {
   SDL_Event event;
   SDL_PollEvent(&event);
   switch (event.type) {
   case SDL_QUIT:
      game->isRunning = false;
      break;

      //case SDL_KEYDOWN:
      //   int x = 0;
      //   int y = 0;
      //   switch (event.key.keysym.sym) {

      //   case SDLK_LEFT:
      //      x = game->board->cursor->GetXPosition();
      //      if (x <= 0) { break; }
      //      else {
      //         game->board->cursor->SetXPosition(x - game->tWidth);
      //         break;
      //      }

      //   case SDLK_RIGHT:
      //      x = game->board->cursor->GetXPosition();
      //      if (x >= (game->bWidth - 2) * game->tWidth) { break; }
      //      else {
      //         game->board->cursor->SetXPosition(x + game->tWidth);
      //         break;
      //      }

      //   case SDLK_UP:
      //      y = game->board->cursor->GetYPosition();
      //      if (y - game->tHeight <= 0) { break; }
      //      else {
      //         game->board->cursor->SetYPosition(y - game->tHeight);
      //         break;
      //      }

      //   case SDLK_DOWN:
      //      y = game->board->cursor->GetYPosition();
      //      if (y + game->tHeight >= game->tHeight * (game->board->startH - 1)) { break; }
      //      else {
      //         game->board->cursor->SetYPosition(y + game->tHeight);
      //         break;
      //      }

      //   case SDLK_SPACE:
      //      boardSwap(game->board, game->board->cursor);
      //      break;

      //   case SDLK_r:
      //      if (!game->board->paused) {
      //         boardMoveUp(game->board, 8);
      //         break;
      //      }
      //      break;
      //   }
   }
}

void gameUpdate(Game* game) {

   // Start the Dear ImGui frame
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplSDL2_NewFrame(game->window2);
   ImGui::NewFrame();

   bool show_demo_window = true;
   ImGui::ShowDemoWindow(&show_demo_window);

   //boardRemoveClears(game->board);
   //if (game->board->pauseLength > 0) {
   //   game->board->pauseLength -= game->timeDelta;

   //   if (game->board->pauseLength < 0) {
   //      game->board->paused = false;
   //      game->board->pauseLength = 0;
   //   }
   //}
   //else {
   //   game->board->paused = false;
   //}

   ////Update board
   //if (game->timer > 2000) {
   //   if (game->board->paused == false) {
   //      boardMoveUp(game->board, 1 * game->board->speed);
   //      boardUpdateArray(game->board, false);
   //   }
   //}

   //if (game->board->bust) {
   //   game->isRunning = false;
   //}

   //boardUpdateFalling(game->board, 4);
   //boardUpdateArray(game->board, false);

   //game->board->cursor->Update(game);

}

void gameRender(Game* game) {

   //SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
   //SDL_RenderClear(game->renderer);
   ////Draw game objects
   //boardRender(game, game->board);

   ////todo Rough frame for the game... use textures later
   //SDL_RenderDrawRect(game->renderer, &game->frame);

   ////debug testing rendering text
   //SDL_Color color = { 255, 255, 255 };
   //SDL_Rect textBox;
   //textBox.x = 64 * 7;
   //textBox.y = 0;
   //textBox.w = 128;
   //textBox.h = 32;
   //SDL_Surface * surface = TTF_RenderText_Solid(game->font, "Swap And Drop", color);
   //SDL_Texture * texture = SDL_CreateTextureFromSurface(game->renderer, surface);
   //SDL_QueryTexture(texture, NULL, NULL, &textBox.w, &textBox.h);
   //SDL_RenderCopy(game->renderer, texture, NULL, &textBox);


   //glViewport(0, 0, (int)game->io->DisplaySize.x, (int)game->io->DisplaySize.y);
   clearRenderer(0.0, 0.0, 0.0, 0.0);

   drawSquare(game->square);

   ImGui::Render();

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
   SDL_GL_SwapWindow(game->window2);

   ////Finish drawing and present
   //SDL_RenderPresent(game->renderer);

   ////debug temp for rendering text
   //SDL_DestroyTexture(texture);
   //SDL_FreeSurface(surface);

}

void gameDestroy(Game* game) {
   //delete game->board->cursor;  //todo make this not a class later
   //for (auto&& t : game->textures) {  //destory all textures, maybe do in function later
   //   SDL_DestroyTexture(t);
   //}
   TTF_CloseFont(game->font);  //free the font
   //boardDestroy(game->board);

   destroyTexture(game->square->texture);
   destroySquare(game->square);

   //imgui stuff to shutdown
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   ImGui::DestroyContext();
   SDL_GL_DeleteContext(game->gl_context);

   //SDL cleanup
   SDL_DestroyWindow(game->window);
   //SDL_DestroyRenderer(game->renderer);
   TTF_Quit();  //close ttf
   SDL_Quit();
   delete game;

   printf("Cleanup successful.\n");
}

bool gameRunning(Game* game) {
   return game->isRunning;
}