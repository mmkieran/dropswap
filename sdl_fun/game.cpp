
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
   //use the shader program

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

   ////setting up board
   //game->board = boardCreate(game);
   //game->board->cursor = new Cursor(game, "assets/cursor.png", (game->bWidth / 2 - 1) * game->tWidth, (game->bHeight / 2 + 1) * game->tHeight);
   //game->board->game = game;


   //game->board->paused = false;
   //game->board->pauseLength = 0;

   ////todo: Use premade boards or fix algorithm so there are no matches at the start
   //boardFillTiles(game->board);

   for (int i = 0; i < 6; i++) {
      game->meshes.push_back(createMesh(game));
   }

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
   ImGui_ImplSDL2_NewFrame(game->window);
   ImGui::NewFrame();


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

   ////Draw game objects
   //boardRender(game, game->board);

   clearRenderer(0.0, 0.0, 0.0, 0.0);

   int width, height;
   SDL_GetWindowSize(game->window, &width, &height);

   setRenderTarget(0, 0, width, height);  //Gotta remember if the window resizes to resize everything

   ////Do this if we want the meshes to stay the same size when then window changes...
   //worldToDevice(game, 0.0f, 0.0f, width, height);

   for (int i = 0; i < game->meshes.size(); i++) {
      game->meshes[i]->texture = resourcesGetTexture(game->resources, i);
      drawMesh(game, game->meshes[i], (64.0 * i), 0.0, 64.0, 64.0);
   }

   for (int i = 0; i < game->meshes.size(); i++) {
      //game->meshes[i]->texture = resourcesGetTexture(game->resources, i);
      drawMesh(game, game->meshes[i], (64.0 * i), 64.0, 64.0, 64.0);
   }

   //ImGui debug
   bool show_demo_window = true;
   bool show_another_window = true;
   ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

   static float f = 0.0f;
   static int counter = 0;

   ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
   ImGui::Image((void*)(intptr_t)game->meshes[1]->texture->handle, { 64, 64 } );

   ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
   ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
   ImGui::Checkbox("Another Window", &show_another_window);

   ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
   ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

   if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
      counter++;
   ImGui::SameLine();
   ImGui::Text("counter = %d", counter);

   ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
   ImGui::End();

   ImGui::Render();

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
   SDL_GL_SwapWindow(game->window);

}

void gameDestroy(Game* game) {

   TTF_CloseFont(game->font);  //free the font
   //boardDestroy(game->board);

   for (int i = 0; i < game->meshes.size(); i++) {
      destroyMesh(game->meshes[i]);
   }

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