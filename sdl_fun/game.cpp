/*Kieran McDonald 08/23/2020
* This game is for educational purposes only
* It's completely for fun!
*/

#include <stdio.h>
#include <time.h>
#include <SDL.h>
#include <thread>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
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

std::thread upnpThread;

//Handle for SDL window and OpenGL Context
struct GameWindow {
   SDL_Window* window;
   SDL_GLContext gl_context;

   unsigned int VAO;              //Vertex Array Object...this actually belongs to rendering
};

//Creates ImGui context
void imguiSetup(Game* game) {
   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   game->fonts[13] = io.Fonts->AddFontFromFileTTF("assets/arial.ttf", 13);
   game->fonts[18] = io.Fonts->AddFontFromFileTTF("assets/arial.ttf", 18);
   game->fonts[20] = io.Fonts->AddFontFromFileTTF("assets/arial.ttf", 20);
   game->fonts[30] = io.Fonts->AddFontFromFileTTF("assets/arial.ttf", 30);
   game->fonts[36] = io.Fonts->AddFontFromFileTTF("assets/arial.ttf", 36);
   io.Fonts->Build();

   //Use these for Keyboard and controller navigation
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();
   ImVec4* colors = ImGui::GetStyle().Colors;
   ImGuiStyle& style = ImGui::GetStyle();
   style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.98f, 0.44f, 0.31f);
   colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
   //colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
   colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
   //colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
   style.WindowPadding.x = 20;
   style.ItemSpacing.x = 14;

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

//SDL function wrapper for delay in millseconds
void sdlSleep(int delay) {
   SDL_Delay(delay);  
}

//Give extra frame time to GGPO so it can do it's thing
void gameGiveIdleToGGPO(Game* game, int time) {
   if (game->net->ggpo && time > 0) {
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
         if (game->players > 1 && game->net->ggpo != nullptr) {
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

//This updates the SDL window with OpenGL rendering
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

   return true;
}

//Set the vertical sync for the monitor refresh rate on or off
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

   game->winsockRunning = winsockStart();
   upnpThread = std::thread(upnpStartup, game);

   //Load game resources
   game->resources = initResources();
   game->mesh = meshCreate();  //Everything is drawn with this

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

   resourcesGetName(game);

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
            if (game->playing == true && popupStatus(Popup_Quit) == false) { popupEnable(Popup_Quit); }
            else { popupDisable(Popup_Quit); }
         }
         if (game->players == 1) {  //Only save state in one player games
            if (event.key.keysym.sym == SDLK_F1) { gameSaveState(game, "saves/game_state.dat"); }
            else if (event.key.keysym.sym == SDLK_F2) { gameLoadState(game, "saves/game_state.dat"); }
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
   if (game->busted == -1) {
      int dead[2] = { 0 , 0 };
      int total[2] = { 0 , 0 };
      for (int i = 0; i < game->boards.size(); i++) {
         total[game->boards[i]->team] += 1;
         if (game->boards[i]->bust == true) {
            dead[game->boards[i]->team] += 1;
         }
      }
      for (int i = 0; i < 2; i++) {
         if (total[i] == dead[i] && total[i] != 0) { 
            game->busted = i;
            return true;
         }
      }
      return false;
   }
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
   if (game->waiting == true) {
      game->waitLength -= 1000 / 60;  

      if (game->waitLength <= 0) {
         game->waiting = false;
         game->waitLength = 0;
      }
   }
   if (game->waiting == false) {
      for (int i = 0; i < game->boards.size(); i++) {
         if (game->boards[i] == nullptr) { continue; }
         if (game->boards[i]->bust == true) { continue; }
         boardUpdate(game->boards[i]);
      }
   }
   //todo add game visual logic somewhere in here?
   gameUpdateSprites(game);
   if (game->waiting == false) {
      game->frameCount++;  //Increment frame count
      game->timer = game->frameCount * (1000.0f / 60.0f);  //Increment game timer
   }
}

//Process inputs and update game - single player
void gameSinglePlayer(Game* game) {
   if (game->playing == false) { return; }
   processInputs(game);
   if (game->ai == true) { gameAI(game); }  
   gameCheckPause(game, game->user.input);
   gameUpdate(game);
}

//Create the boards and set playing to true
void gameStartMatch(Game* game) {
   if (game->players == 1) { game->seed = time(0); }

   for (int i = 0; i < game->fbos.size(); i++) {  //Destroy the old FBOs
      if (game->fbos[i]) {
         rendererDestroyFBO(game->fbos[i]);
      }
   }
   game->fbos.clear();

   int boardCount = 0;
   int myBoard = 0;
   if (game->settings.mode == single_player) {
      strcpy(game->pList[1].name, game->user.name);
      game->user.number = 1;  //Maybe move this to a 1 player UI window later
      game->pList[1].team = 0;
      game->pList[1].level = game->user.level;
      boardCount = 1;
      myBoard = 0;
   }
   else if (game->net->syncTest == true) {
      myBoard = 0;
      boardCount = game->players;
   }
   else if (game->settings.mode == multi_shared) {  //Shared board
      boardCount = 2; 
      myBoard = game->pList[game->user.number].team;
   }  
   else if (game->settings.mode == multi_solo) {  //Solo boards
      boardCount = game->players;
      myBoard = game->user.number - 1;
   }

   for (int i = 0; i < boardCount; i++) {
      Board* board;

      int team = 0;
      if (game->net->syncTest == true) { team = i; }
      else if (game->settings.mode == multi_solo) { team = game->pList[i + 1].team; }
      else if (game->settings.mode == multi_shared) { team = i; }

      if (game->players > 2) { board = boardCreate(game, team, 40, 40); }  //todo investigate why variable tile size causes desync (below)
      else { board = boardCreate(game, team, 52, 52); }  //todo Hard code board size for now
      //if (team == myBoard) { board = boardCreate(game, team, 48, 48); }  //Determine board size based on current user
      //else { board = boardCreate(game, team, 32, 32); }

      if (board) {
         board->index = i;
         boardFillTiles(board);
         game->teams[team].push_back(board);  //todo do we need this?

         //Create the cursor and assign to board
         float cursorX = (float)(game->settings.bWidth / 2 - 1) * board->tileWidth;
         float cursorY = (float)(game->settings.bHeight / 2 + 1) * board->tileHeight;
         if (game->settings.mode == single_player) {
            Cursor* cursor = cursorCreate(board, cursorX, cursorY, game->user.number);
            board->cursors.push_back(cursor);
            game->pList[1].board = board;
            game->pList[1].cursor = cursor;
         }
         else if (game->settings.mode == multi_shared) {
            float level = 0;
            int count = 0;
            for (auto&& p : game->pList) {
               if (p.second.team == team) {  //This is the shared board for this player
                  Cursor* cursor = cursorCreate(board, cursorX, cursorY, p.second.number);
                  board->cursors.push_back(cursor);
                  p.second.board = board;
                  p.second.cursor = cursor;
                  level += game->net->hostSetup[i].level;
                  count++;
               }
            }
            board->level = level / count;
         }
         if (game->settings.mode == multi_solo) {
            Cursor* cursor = cursorCreate(board, cursorX, cursorY, game->pList[i + 1].number);
            board->cursors.push_back(cursor);
            game->pList[i + 1].board = board;
            game->pList[i + 1].cursor = cursor;
            board->level = game->net->hostSetup[i].level;
         }

         game->boards.push_back(board);

         FBO* fbo = rendererCreateFBO(game, board->tileWidth, board->tileHeight);  //Create Framebuffer Object
         if (fbo) { game->fbos.push_back(fbo); }
      }
   }
   for (int i = 0; i < game->boards.size(); i++) {
      for (int j = 0; j < game->boards.size(); j++) {
         if (i == j) { continue; }
         if (game->boards[i]->team == game->boards[j]->team) { game->boards[i]->allies.push_back(j); }
         else { game->boards[i]->enemies.push_back(j); }
      }
   }
   game->playing = true;
   game->frameCount = 0;
   game->timer = 0;
   game->waiting = true;
   game->waitLength = 3000;
   //game->soundToggles[sound_waltz] = true;
}

//Destroy the boards and set playing to false
void gameEndMatch(Game* game) {
   for (int i = 0; i < game->boards.size(); i++) {
      if (game->boards[i]) {
         boardDestroy(game->boards[i]);
      }
   }
   game->pList.clear();
   game->user.number = 1;
   game->boards.clear();
   game->teams.clear();
   game->playing = false;
   game->paused = false;
   game->frameCount = 0;
   game->timer = 0;
   game->busted = -1;
   soundsStopAll();
   if (game->players > 1) {
      ggpoEndSession(game);
   }
}

//Call the draw function for all the boards
void gameRender(Game* game) {
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
         int boardWidth = board->w * board->tileWidth;
         int boardHeight = board->h * board->tileHeight;
         //Do this if we want the meshes to stay the same size when then window changes...
         worldToDevice(game, 0.0f, 0.0f, boardWidth, boardHeight);
         rendererSetTarget(0, 0, boardWidth, boardHeight);
         if (game->fbos[i]) {
            rendererEnableFBO(game->fbos[i]);
            if (board) {
               boardRender(game, board);
            }
         }
         i++;
      }
      rendererDisableFBO();
   }
}

//Draw the ImGui windows and the game objects
void imguiRender(Game* game) {
   if (game->playing == false || game->paused == true) { mainUI(game); }
   int width, height;
   SDL_GetWindowSize(game->sdl->window, &width, &height);
   gameRender(game);  //Draw all game objects

   rendererSetTarget(0, 0, width, height);
   rendererClear(0.0, 0.0, 0.0, 0.0);

   boardUI(game);  
   ImGui::Render();

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

   //Draw Sprites
   worldToDevice(game, 0.0f, 0.0f, width, height);
   gameDrawSprites(game);

   if (game->vsync != 0) { SDL_GL_SwapWindow(game->sdl->window); }
}

//Updates the sprites using their speed and direction, removes expired ones
void gameUpdateSprites(Game* game) {
   std::vector <Sprite> activeSprites;
   for (auto&& sprite : game->drawList) {
      if (sprite.end < game->kt.getTime() / 1000) { continue; }
      else {
         Vec2 move = getXYDistance( { sprite.info.rect.x, sprite.info.rect.y }, sprite.dir, sprite.speed);
         sprite.info.rect.x += move.x;
         sprite.info.rect.y += move.y;
         activeSprites.push_back(sprite);
      }
   }
   game->drawList = activeSprites;
}

//Renders the sprites after imgui has drawn it's windows so we can draw over top
//todo investigate if we could have just done foreground in ImGui, lol
void gameDrawSprites(Game* game) {
   for (auto&& sprite : game->drawList) {
      //Camera movements or mesh displacements
      Vec2 move = { 0, 0 };
      sprite.info.cam = move;

      //Color transformations
      for (int i = 0; i < 4; i++) { sprite.info.color[i] = 1.0; }

      if (sprite.render.animation != nullptr) {
         animationDraw(game, sprite.render.animation, sprite.info);
      }
      else if (sprite.render.texture != nullptr) {
         meshDraw(game, sprite.render.texture, sprite.info);
      }
   }
}

//Jokulhaups
void gameDestroy(Game* game) {
   upnpThread.join();
   for (auto&& board : game->boards) {
      if (board) { boardDestroy(board); }
   }

   if (game->net && game->net->ggpo) {
      ggpoClose(game->net->ggpo);
      game->net->ggpo = nullptr;
   }
   tcpCleanup();
   upnpCleanup();

   game->mesh = meshDestroy(game->mesh);
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

   winsockCleanup();

   printf("Cleanup successful.\n");
}

//Returns true if the game is running... used for game loop
bool gameRunning(Game* game) {
   return game->isRunning;
}

void gameAI(Game* game) {
   boardAI(game);
}