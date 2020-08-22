
#include <stdio.h>
#include <time.h>
#include <SDL.h>

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
#include "serialize.h"
#include "game_inputs.h"
#include "netplay.h"

struct GameWindow {
   SDL_Window *window;
   SDL_GLContext gl_context;

   unsigned int VAO;  //This doesn't really belong here
   std::vector <unsigned char> save;  //todo debug find a better place for this later

};

//SDL function wrappers
uint64_t sdlGetCounter() {
   uint64_t current = SDL_GetPerformanceCounter();
   return current;
}

void sdlSleep(int delay) {
   SDL_Delay(delay);  //milliseconds
}

//Give extra frame time to GGPO so it can do it's thing
void gameGiveIdleToGGPO(Game* game, int time) {
   if (game->net && game->net->ggpo && time > 0) {
      ggpo_idle(game->net->ggpo, time);
      //printf("Time: %d\n", time);
   }
}

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
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

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
   game->net = new NetPlay;
   game->net->game = game;

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
   game->boards = vectorCreate<Board*>(4, 10);
   game->seed = 0;  //generate the random seed for the board tiles

   game->isRunning = true;

   game->p1Input.code = 0;
   game->p1Input.timer = 0;
   game->p1Input.handle = 0;
   game->p1Input.msg = 0;

   return game;
}

void gameHandleEvents(Game* game) {
   SDL_Event event;

   while (SDL_PollEvent(&event)) {

      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) {
         game->isRunning = false;
      }
   }

   //if (game->input.pause.p == true) {  //pause game... update doesn't happen if we're paused, so don't put it there
   //   if (game->paused == true) { game->paused = false; }
   //   else if (game->paused == false) { game->paused = true; }
   //}

}

void gameUpdate(Game* game) {
   
   for (int i = 1; i <= vectorSize(game->boards); i++) {
      boardUpdate(vectorGet(game->boards, i), game->inputs[i - 1]);
   }
}

void gameRender(Game* game) {
   //Remember that this has to happen after imguiRender, or the clear will remove everything...

   //Draw game objects
   if (game->playing == true) {
      for (int i = 1; i <= vectorSize(game->boards); i++) {
         Board* board = vectorGet(game->boards, i);
         if (board) {
            boardRender(game, board);
         }
      }
      //debugCursor(game);  //imgui debug tools
      //debugGarbage(game);
   }
}

void gameStartMatch(Game* game) {
   //setting up board
   for (int i = 1; i <= game->players; i++) {
      Board* board = boardCreate(game);
      board->player = i;  //todo this might not work in terms of player number??
      vectorPushBack(game->boards, board);
      boardFillTiles(board);

      //todo add a smarter algorithm to tile boards in screen space if more than 2
      float xOrigin = game->tWidth * game->bWidth * (i - 1) + game->tWidth * i;
      float yOrigin = game->tHeight;

      //todo if we want more than 2 we'll have to use a tiling algorithm
      //if (i > 2) {
      //   xOrigin = game->tWidth * game->bWidth * (i - 3) + game->tWidth * (i - 2);
      //   yOrigin += game->tHeight * game->bHeight + game->tHeight * 2;
      //}

      board->origin = { xOrigin, yOrigin };
   }
   game->playing = true;
}

void imguiRender(Game* game) {

   rendererClear(0.0, 0.0, 0.0, 0.0);

   int width, height;
   SDL_GetWindowSize(game->sdl->window, &width, &height);

   rendererSetTarget(0, 0, width, height);  //Gotta remember if the window resizes to resize everything

   //Do this if we want the meshes to stay the same size when then window changes...
   worldToDevice(game, 0.0f, 0.0f, width, height);

   gameRender(game);  //Draw all game objects

   ImGui::Render();

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
   SDL_GL_SwapWindow(game->sdl->window);

}

void gameDestroy(Game* game) {

   for (int i = 1; i <= vectorSize(game->boards); i++) {
      Board* board = vectorGet(game->boards, i);
      if (board) {
         boardDestroy(board);
      }
   }

   if (game->net && game->net->ggpo) {
      ggpoClose(game->net->ggpo);
      game->net->ggpo = nullptr;
   }

   vectorDestroy(game->boards);

   destroyResources(game->resources);

   vaoDestroy(game->sdl->VAO);

   //imgui stuff to shutdown
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   ImGui::DestroyContext();
   SDL_GL_DeleteContext(game->sdl->gl_context);

   //SDL cleanup
   SDL_DestroyWindow(game->sdl->window);
   SDL_Quit();
   delete game->sdl;
   delete game->net;
   delete game;

   printf("Cleanup successful.\n");
}

bool gameRunning(Game* game) {
   return game->isRunning;
}

void debugGarbage(Game* game) {
   //ImGui debug window

   ImGui::Begin("Debug Garbage");

   Board* board = vectorGet(game->boards, 1);
   for (int row = 0; row < board->endH; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_garbage && tile->garbage) {
            int start = 0;
            if (tile->garbage) {start = 1; }
            ImGui::Text("%d type, ptr %d, %d id", tile->type, start, tile->garbage->ID);
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

void showHostWindow(Game* game, bool* p_open) {
   if (!ImGui::Begin("Host Setup", p_open) ) {
      ImGui::End();
      return;
   }

   static unsigned short participants = 2;
   int pMin = 2;
   int pMax = GAME_MAX_PLAYERS;

   ImGui::PushItemWidth(120);
   ImGui::SliderScalar("Participants", ImGuiDataType_U8, &participants, &pMin, &pMax);
   ImGui::SameLine();

   if (ImGui::Button("Load From File")) {

   }

   ImGui::SameLine();
   if (ImGui::Button("Save To File")) {

   }
   ImGui::PopItemWidth();
   ImGui::NewLine();

   static SessionInfo hostSetup[GAME_MAX_PLAYERS];

   ImGui::PushID("Player Info Set");
   for (int i = 0; i < participants; i++) {

      ImGui::PushID(i);
      ImGui::PushItemWidth(80);
      ImGui::Text("Player%d", i + 1);

      if (ImGui::Checkbox("Me", &hostSetup[i].me)) {
         for (int j = 0; j < participants; j++) {
            if (hostSetup[j].me == true && i != j) { hostSetup[j].me = false; }
         }
      }
      ImGui::SameLine();

      if (ImGui::Checkbox("Host", &hostSetup[i].host)) {
         for (int j = 0; j < participants; j++) {
            if (hostSetup[j].host == true && i != j) { hostSetup[j].host = false; }
         }
      }
      ImGui::SameLine();

      ImGui::Combo("Player Type", &hostSetup[i].playerType, "Local\0Remote\0Spectator\0");

      ImGui::SameLine();
      ImGui::InputText("IP Address", hostSetup[i].ipAddress, IM_ARRAYSIZE(hostSetup[i].ipAddress));
      ImGui::SameLine();

      ImGui::InputInt("Port", &hostSetup[i].localPort);
      ImGui::SameLine();
      ImGui::Text(ggpoShowStatus(game, i) );

      ImGui::PopItemWidth();
      ImGui::PopID();
   }
   ImGui::PopID();
   ImGui::NewLine();

   if (ImGui::Button("Start Session")) {
      ggpoCreateSession(game, hostSetup, participants);
   }
   ImGui::SameLine();
   if (ImGui::Button("End Session")) {
      ggpoEndSession(game);
   }

   static bool localReady = false;
   static bool remoteReady = false;
   static bool seedSent = false;
   static bool seedReceived = false;

   ImGui::SameLine();
   if (ImGui::Button("New Seed")) {
      seedSent = seedReceived = false;
   }

   if (game->net && game->net->connections[game->net->hostConnNum].state == 2) {
      ImGui::Text("Press [ENTER] when ready");
      if (seedReceived == true) { ImGui::Text("Received seed: %d", game->seed); }
      if (seedSent == true) { ImGui::Text("Sent seed: %d", game->seed); }

      if (game->net->localPlayer == 1) {  //Player 1 sends the game seed
         if (seedSent == false){ 
            game->seed = time(0);
            game->p1Input.msg = game->seed;
            seedSent = true;
         }
      }
      else {
         if (seedReceived == false) {
            if (game->inputs[0].msg > 0) {
               game->seed = game->inputs[0].msg;
               seedReceived = true;
            }
         }
      }

      for (int i = 0; i < game->players; i++) {
         if (game->inputs[i].pause.p == true) {  //Check if either player pressed enter to start
            if (game->net->localPlayer == i + 1) {
               localReady = true;
            }
            else {
               remoteReady = true;
            }
         }
      }
      if (remoteReady == true) { ImGui::Text("Remote player ready"); }

      if (remoteReady == true && localReady == true || SYNC_TEST == true) {
         gameStartMatch(game);
         remoteReady = localReady = false;
      }
   }
   ImGui::End();
}

void showGameMenu(Game* game) {

   if (!ImGui::Begin("Game Menus")) {
      ImGui::End();
      return;
   }

   ImGui::InputInt("Players", &game->players);

   static bool hostWindow = false;
   if (ImGui::Button("Host Window")) {
      hostWindow = true;
   }
   if (hostWindow && game->playing == false ) { 
      showHostWindow(game, &hostWindow); 
   }


   ImGui::InputInt("Tile Width", &game->tWidth, 16);
   ImGui::InputInt("Tile Height", &game->tHeight, 16);

   ImGui::InputInt("Board Width", &game->bWidth);
   ImGui::InputInt("Board Height", &game->bHeight);


   if (game->playing == false) {
      if (ImGui::Button("Start Game")) {
         gameStartMatch(game);
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

   if (ImGui::Button("Load Board")) {
      gameLoadState(game, "saves/game_state.dat");
   }

   if (ImGui::Button("Save Game")) {
      gameSaveState(game, "saves/game_state.dat");
   }

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

std::vector <Byte> gameSave (Game* game) {
   std::vector <Byte> stream;

   //serialize game settings
   _gameSerialize(stream, game);

   for (int i = 1; i <= game->players; i++) {
      Board* board = vectorGet(game->boards, i);
      if (board) {
         //serialize board settings
         _boardSerialize(stream, board);

         //serialize garbage
         _garbageSerialize(stream, board);

         //serialize tiles
         for (int row = 0; row < board->wBuffer; row++) {
            for (int col = 0; col < board->w; col++) {
               Tile* tile = boardGetTile(board, row, col);
               if (tile) {
                  _tileSerialize(stream, tile);
               }
            }
         }

         //serialize cursor
         _cursorSerialize(stream, board->cursor);

      }
   }

   return stream;
}

int gameLoad(Game* game, unsigned char* &start) {

   //destroy the boards
   for (int i = 1; i <= vectorSize(game->boards); i++) {
      Board* board = vectorGet(game->boards, i);
      boardDestroy(board);
      vectorClear(game->boards);
   }

   //deserialize game
   _gameDeserialize(start, game);

   for (int i = 1; i <= game->players; i++) {
      Board* board = nullptr;
      if (game->playing) {
         board = boardCreate(game);
         //deserialize board
         if (board) {
            _boardDeserialize(start, board);
            boardLoadRandom(board);  //Return random generator to saved state using discard

            //deserialize garbage
            _garbageDeserialize(start, board);

            for (int row = 0; row < board->wBuffer; row++) {
               for (int col = 0; col < board->w; col++) {
                  Tile* tile = boardGetTile(board, row, col);
                  //deserialize tiles
                  tile->mesh = meshCreate(board->game);
                  tile->garbage = nullptr;
                  _tileDeserialize(start, board, tile);
                  tileSetTexture(board, tile);
               }
            }

            //deserialize cursor
            board->cursor = cursorCreate(board, 0, 0);
            _cursorDeserialize(start, board->cursor);

            vectorPushBack(game->boards, board);
         }
      }

   }

   return 0;
}

FILE* gameSaveState(Game* game, const char* filename) {
   FILE* out;
   int err = fopen_s(&out, filename, "w");
   std::vector <Byte> stream;
   stream = gameSave(game);

   if (err == 0 && stream.size() > 0) {
      int streamSize = stream.size();
      fwrite(&streamSize, sizeof(int), 1, out);

      //todo make this betterer?
      for (int i = 0; i < streamSize; i++) {
         fwrite(&stream[i], sizeof(Byte), 1, out);
      }

   }
   else { printf("Failed to save file... Err: %d\n", err); }
   game->sdl->save = stream;  //todo debug replace with a nicer system to store saves
   fclose(out);
   return out;
}

int gameLoadState(Game* game, const char* path) {
   FILE* in;
   int err = fopen_s(&in, path, "r");
   std::vector <Byte> stream;
   if (err == 0) {
      int streamSize = 0;
      fread(&streamSize, sizeof(int), 1, in);
      stream.resize(streamSize);

      //Byte* start = stream.data();  //Find out how to make this work, lol
      //fread(&start, sizeof(Byte) * streamSize, 1, in);

      //todo read the entire file into memory and memcpy it to the vector
      for (int i = 0; i < streamSize; i++) {
         char c;
         fread(&c, sizeof(Byte), 1, in);
         stream[i] = c;
      }

      unsigned char* start = stream.data();
      //unsigned char* start = game->sdl->save.data();  //todo debug needs better system
      gameLoad(game, start);
   }
   else { printf("Failed to load file... Err: %d\n", err); }
   fclose(in);
   return 1;
}


