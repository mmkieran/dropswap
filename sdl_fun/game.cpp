
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
#include "serialize.h"
#include "game_inputs.h"

#include <ggponet.h>

int gameLoad(Game* game, unsigned char*& start);
std::vector <Byte> gameSave(Game* game);

void gameAdvanceFrame(Game* game);

//todo temporary place for GGPO
//Dunno what I need yet

/*
Run frame

Advance Frame
1. Update game
2. Checksum and frame number
3. notify advance frame
4. Performance update?

Init Spectator

Init Player

Idle

Disconnect Player
*/

enum PlayerConnectState {
    Connecting = 0,
    Synchronizing,
    Running,
    Disconnected,
    Disconnecting,
};

struct PlayerConnectionInfo {
    GGPOPlayerType       type;
    GGPOPlayerHandle     handle;
    PlayerConnectState   state;
    int                  connect_progress;
    int                  disconnect_timeout;
    int                  disconnect_start;
};

struct ggpoHandle {
   GGPOSession* ggpo = nullptr;
   GGPOErrorCode result;
   Game* game = nullptr;
   GGPOPlayer players[MAX_PLAYERS];
   PlayerConnectionInfo connections[MAX_PLAYERS];
   GGPOPlayerHandle localPlayer;
};

ggpoHandle ggHandle;

struct GameWindow {
   SDL_Window *window;
   SDL_GLContext gl_context;

   unsigned int VAO;  //This doesn't really belong here
   std::vector <unsigned char> save;  //todo debug find a better place for this later

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
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
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

//Make way for GGPO
//Don't call it a callback!
bool __cdecl ds_begin_game_callback(const char*) {
   //we don't need to do anything here apparently
   return true;
}

bool __cdecl ds_advance_frame_callback(int) {

   int disconnect_flags;

   //Figure out the inputs and check for disconnects
   ggpo_synchronize_input(ggHandle.ggpo, (void*)ggHandle.game->inputs, sizeof(UserInput) * MAX_PLAYERS, &disconnect_flags);

   //Call function to advance frame
   gameAdvanceFrame(ggHandle.game);

   return true;
}


bool __cdecl ds_load_game_callback(unsigned char* buffer, int len) {

   //std::vector <Byte> stream;
   //stream.resize(len);

   if (len > 0) {
      unsigned char* start = buffer;
      gameLoad(ggHandle.game, start);

      return true;
   }
   else { return false; }
}

bool __cdecl ds_save_game_callback(unsigned char** buffer, int* len, int* checksum, int) {

   std::vector <Byte> stream = gameSave(ggHandle.game);
   if (stream.size() == 0) { return false; }

   *buffer = (unsigned char*)malloc(*len);
   if (buffer) {
      *len = stream.size();
      memcpy(*buffer, stream.data(), *len);
      return true;
   }
   else { return false; }

}

void __cdecl ds_free_buffer_callback(void* buffer) {
    if (buffer) {free(buffer); }
}

bool __cdecl ds_on_event_callback(GGPOEvent* info) {
    int progress;

    //todo come back in fill in events

    //switch (info->code) {
    //case GGPO_EVENTCODE_CONNECTED_TO_PEER:
    //    ngs.SetConnectState(info->u.connected.player, Synchronizing);
    //    break;
    //case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
    //    progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
    //    ngs.UpdateConnectProgress(info->u.synchronizing.player, progress);
    //    break;
    //case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
    //    ngs.UpdateConnectProgress(info->u.synchronized.player, 100);
    //    break;
    //case GGPO_EVENTCODE_RUNNING:
    //    ngs.SetConnectState(Running);
    //    renderer->SetStatusText("");
    //    break;
    //case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
    //    ngs.SetDisconnectTimeout(info->u.connection_interrupted.player,
    //        timeGetTime(),
    //        info->u.connection_interrupted.disconnect_timeout);
    //    break;
    //case GGPO_EVENTCODE_CONNECTION_RESUMED:
    //    ngs.SetConnectState(info->u.connection_resumed.player, Running);
    //    break;
    //case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
    //    ngs.SetConnectState(info->u.disconnected.player, Disconnected);
    //    break;
    //case GGPO_EVENTCODE_TIMESYNC:
    //    Sleep(1000 * info->u.timesync.frames_ahead / 60);
    //    break;
    //}

    return true;
}

bool __cdecl ds_log_game_state_callback(char* filename, unsigned char* buffer, int len) {

    Game* game = new Game;
    if (game) {
        game->boards = vectorCreate<Board*>(4, 10);
        if (game->boards) {
            gameLoad(game, buffer);

            FILE* log = nullptr;
            fopen_s(&log, filename, "w");
            if (log) {
                fprintf(log, "Current Game State\n");
                fprintf(log, "Players: %d\n", game->players);
                fprintf(log, "Game timer: %d\n", game->timer);
                fprintf(log, "Seed: %d\n", game->seed);
                fprintf(log, "Boards: %d\n", vectorSize(game->boards));
                for (int i = 1; i <= vectorSize(game->boards); i++) {
                    Board* board = vectorGet(game->boards, i);
                    fprintf(log, "Player: %d\n", board->player);
                    fprintf(log, "RandomCalls: %d\n", board->randomCalls);
                }
            }

            fclose(log);

            for (int i = 1; i <= vectorSize(game->boards); i++) {
                Board* board = vectorGet(game->boards, i);
                boardDestroy(board);
            }
            vectorDestroy(game->boards);
        }
    }
    delete game;
    return true;
}

void ggpoInitPlayer(Game* game, int playerCount, unsigned short localport) {

    GGPOErrorCode result;
    //init game state

   //Called at startup to setup GGPO session
   GGPOSessionCallbacks cb;
   cb.begin_game = ds_begin_game_callback;
   cb.advance_frame = ds_advance_frame_callback;  //todo not done
   cb.load_game_state = ds_load_game_callback;
   cb.save_game_state = ds_save_game_callback;
   cb.free_buffer = ds_free_buffer_callback;
   cb.on_event = ds_on_event_callback;
   cb.log_game_state = ds_log_game_state_callback;

   //Can add sync test here
   localport = 7001;  //todo hard code this for now
   result = ggpo_start_session(&ggHandle.ggpo, &cb, "Drop and Swap", playerCount, sizeof(UserInput), localport);

   // automatically disconnect clients after 3000 ms and start our count-down timer
   // for disconnects after 1000 ms.   To completely disable disconnects, simply use
   // a value of 0 for ggpo_set_disconnect_timeout.
   ggpo_set_disconnect_timeout(ggHandle.ggpo, 3000);
   ggpo_set_disconnect_notify_start(ggHandle.ggpo, 1000);

   //todo don't do this the dumb way
   ggHandle.players[0].type = GGPO_PLAYERTYPE_LOCAL;
   ggHandle.players[0].size = sizeof(GGPOPlayer);
   ggHandle.players[0].player_num = 1;
   ggHandle.players[0].u.local;

   ggHandle.players[1].type = GGPO_PLAYERTYPE_REMOTE;
   ggHandle.players[1].size = sizeof(GGPOPlayer);
   ggHandle.players[1].player_num = 2;
   const char* ip = "127.0.0.1";
   strcpy(ggHandle.players[1].u.remote.ip_address, ip);
   ggHandle.players[1].u.remote.port = 7002;

   //loop to add Players
   for (int i = 0; i < playerCount; i++) {
       GGPOPlayerHandle handle;
       result = ggpo_add_player(ggHandle.ggpo, &ggHandle.players[i], &handle);
       ggHandle.connections[i].handle = handle;
       ggHandle.connections[i].type = ggHandle.players->type;
       if (ggHandle.players[i].type == GGPO_PLAYERTYPE_LOCAL) {
          ggHandle.localPlayer = handle;
          ggpo_set_frame_delay(ggHandle.ggpo, handle, FRAME_DELAY);
       }

   }

}

void ggpoInitSpectator() {

    //result = ggpo_start_spectating...
}

void gameAdvanceFrame(Game* game) {
   gameUpdate(game);  //todo come back and make this work

   //Tell GGPO we moved ahead a frame
   ggpo_advance_frame(game->ggHandle->ggpo);

   //todo <- come back and fill this in... not sure about player handles

   //// Update the performance monitor display.
   //GGPOPlayerHandle handles[MAX_PLAYERS];
   //int count = 0;
   //for (int i = 0; i < ngs.num_players; i++) {
   //   if (ngs.players[i].type == GGPO_PLAYERTYPE_REMOTE) {
   //      handles[count++] = ngs.players[i].handle;
   //   }
   //}
   //ggpoutil_perfmon_update(ggpo, handles, count);
}

void gameRunFrame() {

   GGPOErrorCode result = GGPO_OK;
   int disconnect_flags;

   //read local inputs

   if (ggHandle.localPlayer != GGPO_INVALID_HANDLE) {
      inputProcessKeyboard(ggHandle.game);
   }

   //Can do sync test here

   result = ggpo_add_local_input(ggHandle.ggpo, ggHandle.localPlayer, &ggHandle.game->p1Input, sizeof(UserInput));
   //If we got the local inputs successfully, merge in remote ones
   if (GGPO_SUCCEEDED(result)) {
      result = ggpo_synchronize_input(ggHandle.ggpo, (void*)ggHandle.game->inputs, sizeof(UserInput) * MAX_PLAYERS, &disconnect_flags);
      if (GGPO_SUCCEEDED(result)) {
         gameAdvanceFrame(ggHandle.game);  //Update the game 
      }
   }

   gameRender(ggHandle.game);  //Draw everything
}

//End GGPO stuff

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
   ggHandle.game = game;

   //game->ggHandle = new ggpoHandle;

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

   //todo add in pausing again once all the dust clears

   //inputProcessKeyboard(game);  //Fill out the UserInput struct from keyboard

   //if (game->p1Input.pause.p == true) {  //pause game... update doesn't happen if we're paused, so don't put it there
   //   if (game->paused == true) { game->paused = false; }
   //   else if (game->paused == false) { game->paused = true; }
   //}

}

void gameUpdate(Game* game) {

   //todo rework to use new inputs
   
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

   ////Draw game objects
   //if (game->playing == true) {
   //   for (int i = 1; i <= vectorSize(game->boards); i++) {
   //      Board* board = vectorGet(game->boards, i);
   //      if (board) {
   //         boardRender(game, board);
   //      }
   //   }
   //   //debugCursor(game);  //imgui debug tools
   //   //debugGarbage(game);
   //}

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
   TTF_Quit();  //close ttf
   SDL_Quit();
   delete game->sdl;
   //delete game->ggHandle;
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

void ggpoUI(bool* p_open) {
   if (!ImGui::Begin("GGPO Setup", p_open)) {
      ImGui::End();
      return;
   }

   int localPort, remotePort;
   char remote_ip[32];

   ImGui::InputText("Remote IP", remote_ip, IM_ARRAYSIZE(remote_ip));

   ImGui::InputInt("Local port", &localPort);
   ImGui::InputInt("Remote port", &remotePort);


   ImGui::End();
}

void showGameMenu(Game* game) {

   if (!ImGui::Begin("Game Menus")) {
      ImGui::End();
      return;
   }

   //ImGuiIO& io = ImGui::GetIO();
   //if (true) {
   //   io.WantCaptureKeyboard = true;

   //}

   ImGui::InputInt("Players", &game->players);

   static bool showGGPO = false;
   if (ImGui::Button("GGPO Setup")) {
      showGGPO = true;
   }
   if (showGGPO) { ggpoUI(&showGGPO); }

   ImGui::InputInt("Tile Width", &game->tWidth, 16);
   ImGui::InputInt("Tile Height", &game->tHeight, 16);

   ImGui::InputInt("Board Width", &game->bWidth);
   ImGui::InputInt("Board Height", &game->bHeight);


   if (game->playing == false) {
      if (ImGui::Button("Start Game")) {

         //setting up board
         for (int i = 1; i <= game->players; i++) {
            Board* board = boardCreate(game);
            board->player = i;
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


