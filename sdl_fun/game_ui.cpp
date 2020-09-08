#include "game_ui.h"

#include "board.h"
#include "serialize.h"
#include "netplay.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

const char* credits = R"(
A special thanks goes out to:
Stephanie Anderson
Brandon Townsend
Sean Hunter
...
)";

void mainUI(Game* game) {

   ImGui::PushFont(game->fonts[20]);
   if (!ImGui::Begin("Menu")) {
      ImGui::PopFont();
      ImGui::End();
      return;
   }

   ImGui::NewLine();

   float width = ImGui::GetWindowContentRegionWidth();

   if (ImGui::Button("One Player", ImVec2{width, 0}) ) {
      game->players = 1;
      gameStartMatch(game);
   }
   ImGui::NewLine();

   static bool showGGPOSession = false;
   if (ImGui::Button("Two Player", ImVec2{ width, 0 }) ) {
      game->players = 2;
      showGGPOSession = true;
   }
   if (showGGPOSession && game->playing == false) {
      ggpoSessionUI(game, &showGGPOSession);
   }

   ImGui::NewLine();

   static bool showSettings = false;
   if (ImGui::Button("Settings", ImVec2{ width, 0 }) ) {
      showSettings = true;
   }
   if (showSettings) {
      gameSettingsUI(game, &showSettings);
   }

   //gameMenuUI(game);

   ImGui::PopFont();
   ImGui::End();
}

static void _drawBoardTexture(Game* game, int index) {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

      char playerName[20] = "Player";
      sprintf(playerName, "Player %d", index + 1);
      ImVec2 screenPos = ImGui::GetCursorScreenPos();
      ImGui::BeginChild(playerName, ImVec2{ (float)game->tWidth * (game->bWidth), (float)game->tHeight * (game->bHeight) }, true, 0);

      //This is the secret sauce to render a texture in ImGui
      //The ImVec2{ 0, 1 }, ImVec2{ 1, 0 } is because it uses a different coordinate system by default
      if (game->fbos[index]) {
         ImGui::Image((void*)(intptr_t)game->fbos[index]->texture, { game->fbos[index]->w, game->fbos[index]->h }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
      }

      ////Proof of concept abitrary text rending
      ////todo look at ImDrawList API for arbitrary rendering
      //ImVec2 screenPos2 = ImGui::GetCursorScreenPos();
      //ImGui::SetCursorScreenPos(ImVec2{ screenPos.x + 64, screenPos.y + 64 });
      //ImGui::Text("%s", playerName);
      //ImGui::SetCursorScreenPos(screenPos2);
      ImGui::EndChild();

      ImGui::PopStyleVar();
}

void boardUI(Game* game) {
   if (game->playing == true) {

      ImGui::PushFont(game->fonts[20]);
      ImGui::Begin("Drop and Swap");

      _drawBoardTexture(game, 0);
      ImGui::SameLine();

      ImGui::BeginChild("Game Info", ImVec2{ ImGui::GetWindowContentRegionWidth() * 0.2f, (float)game->tHeight * (game->bHeight) }, true, 0);
      for (auto&& board : game->boards) {
         ImGui::NewLine();

         static int lastChain = 0;
         static int chainTime = 0;
         if (board->chain > 1) {
            lastChain = board->chain;
            chainTime = game->timer;
         }
         ImGui::Text("%d chain", board->chain);
         ImGui::Text("Last chain: %d", lastChain);
         ImGui::Text("Pause Time: %d", board->pauseLength);
         ImGui::Text("Game Time: %d", game->timer);
         ImGui::NewLine();

         if (ImGui::Button("End Game")) {
            gameEndMatch(game);
         }
      }
      ImGui::EndChild();

      if (game->players > 1) { 
         ImGui::SameLine();
         _drawBoardTexture(game, 1); 
      }

      ImGui::PopFont();
      ImGui::End();
   }
}

void gameSettingsUI(Game* game, bool* p_open) {
   if (!ImGui::Begin("Game Settings", p_open) ) {
      ImGui::End();
      return;
   }

   ImGui::Combo("Game Controls", &game->controls, "Keyboard\0Controller\0");
   ImGui::Combo("Sound Effects", &game->sounds, "On\0Off\0");
   static int backgroundMusic = 0;
   ImGui::Combo("Background Music", &backgroundMusic, "On\0Off\0");

   ImGui::InputInt("Tile Width", &game->tWidth, 16);
   ImGui::InputInt("Tile Height", &game->tHeight, 16);

   ImGui::InputInt("Board Width", &game->bWidth);
   ImGui::InputInt("Board Height", &game->bHeight);

   ImGui::End();
}

//Show the game menu window
void gameMenuUI(Game* game) {

   if (!ImGui::Begin("Game Menus")) {
      ImGui::End();
      return;
   }

   ImGui::PushFont(game->fonts[13]);
   //Game Over message
   if (ImGui::BeginPopupModal("Game Over", NULL)) {
      ImGui::Text("You won or lost or something...");
      if (ImGui::Button("Accept Defeat")) {
         ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
   }

   ImGui::SetNextItemOpen(true, ImGuiCond_Once);
   if (ImGui::CollapsingHeader("Player Settings")) {
      static int peoplePlaying = game->players - 1;
      ImGui::Combo("Players", &peoplePlaying, "One Player\0Two Player\0");
      game->players = peoplePlaying + 1;
      ImGui::Combo("Game Controls", &game->controls, "Keyboard\0Controller\0");
      ImGui::Combo("Sound Effects", &game->sounds, "On\0Off\0");
      static int backgroundMusic = 0;
      ImGui::Combo("Background Music", &backgroundMusic, "On\0Off\0");

      static bool showGGPOSession = false;
      if (game->players > 1 || game->syncTest == true) {
         if (ImGui::Button("Connection Window")) {
            showGGPOSession = true;
         }
         if (showGGPOSession && game->playing == false) {
            ggpoSessionUI(game, &showGGPOSession);
         }
      }
   }

   ImGui::SetNextItemOpen(true, ImGuiCond_Once);
   if (ImGui::CollapsingHeader("Board Settings")) {
      ImGui::InputInt("Tile Width", &game->tWidth, 16);
      ImGui::InputInt("Tile Height", &game->tHeight, 16);

      ImGui::InputInt("Board Width", &game->bWidth);
      ImGui::InputInt("Board Height", &game->bHeight);
   }

   ImGui::SetNextItemOpen(true, ImGuiCond_Once);
   if (ImGui::CollapsingHeader("Game Actions")) {
      if (game->playing == false) {
         if (ImGui::Button("Start Game")) {
            gameStartMatch(game);
         }
      }
      if (game->playing == true) {
         if (ImGui::Button("End Game")) {
            gameEndMatch(game);
         }
      }

      if (game->playing == true) {
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
      }

      if (ImGui::Button("Load Board")) {
         gameLoadState(game, "saves/game_state.dat");
      }

      if (ImGui::Button("Save Game")) {
         gameSaveState(game, "saves/game_state.dat");
      }
   }

   if (ImGui::CollapsingHeader("Debug")) {

      static bool showDemo = false;
      if (showDemo == false) {
         if (ImGui::Button("Show ImGui Demo")) {
            showDemo = true;
         }
      }
      else {
         if (ImGui::Button("Hide ImGui Demo")) {
            showDemo = false;
         }
      }
      if (showDemo == true) { ImGui::ShowDemoWindow(&showDemo); }

      if (ImGui::Button("Clear Board")) {
         if (game->playing == true) {
            for (auto&& board : game->boards) {
               if (board) { boardClear(board); }
            }
         }
      }

      if (ImGui::Button("Make it rain")) {
         if (game->playing == true) {
            for (auto&& board : game->boards) {
               if (board) { makeItRain(board); }
            }
         }
      }

      if (game->playing == true) {
         static int gWidth = 6;
         static int gHeight = 1;
         static bool isMetal = false;
         ImGui::InputInt("Garbage Width", &gWidth);
         ImGui::InputInt("Garbage Height", &gHeight);
         ImGui::Checkbox("Metal", &isMetal);

         if (ImGui::Button("Dumpstered")) {

            for (auto&& board : game->boards) {
               if (board) { garbageCreate(board, gWidth, gHeight, isMetal); }
            }
         }
      }

      if (game->playing == true) {
         for (auto&& board : game->boards) {
            if (board) {
               float minFallSpeed = 0;
               float maxFallSpeed = 8.0;

               ImGui::SliderScalar("Fall Speed", ImGuiDataType_Float, &board->fallSpeed, &minFallSpeed, &maxFallSpeed);

               float minBoardSpeed = 0;
               float maxBoardSpeed = 10.0;
               ImGui::SliderScalar("Board Speed", ImGuiDataType_Float, &board->moveSpeed, &minBoardSpeed, &maxBoardSpeed);

               float minBoardLevel = 0;
               float maxBoardLevel = 10.0;
               ImGui::SliderScalar("Board Level", ImGuiDataType_Float, &board->level, &minBoardLevel, &maxBoardLevel);
            }
         }
      }
   }


   if (ImGui::CollapsingHeader("Credits")) {
      //todo maybe just read in a file here
      ImGui::TextUnformatted(credits);
   }
   ImGui::PopFont();

   ImGui::End();
}

//Show the connection window for GGPO... only for 2 players
void ggpoSessionUI(Game* game, bool* p_open) {

   if (!ImGui::Begin("Host Setup", p_open)) {
      ImGui::End();
      return;
   }

   ImGui::PushFont(game->fonts[13]);
   //Debug turn on sync test
   ImGui::Checkbox("DEBUG: sync test", &game->syncTest);

   static SessionInfo hostSetup[GAME_MAX_PLAYERS];

   static int seed = 0;
   ImGui::DragInt("Seed", &seed, 1, 1.0, 5000);
   game->seed = seed;
   ImGui::NewLine();

   static unsigned short participants = 2;
   int pMin = 2;
   int pMax = GAME_MAX_PLAYERS;

   ImGui::PushItemWidth(120);
   ImGui::SliderScalar("Participants", ImGuiDataType_U8, &participants, &pMin, &pMax);
   ImGui::SameLine();

   if (ImGui::Button("Load From File")) {
      FILE* in;
      int err = fopen_s(&in, "saves/ggpo_session_setup.csv", "r");

      if (err == 0) {
         int i = 0;  //participants
         char* tok;
         char buffer[2048];

         fgets(buffer, 2048, in); // header
         fgets(buffer, 2048, in); //First data line
         while (!feof(in))
         {
            hostSetup[i].me = atoi(strtok(buffer, ",\n"));          // me
            hostSetup[i].host = atoi(strtok(nullptr, ",\n"));       // host
            hostSetup[i].playerType = atoi(strtok(nullptr, ",\n")); // player type
            strcpy(hostSetup[i].ipAddress, strtok(nullptr, ",\n"));  // ip address
            hostSetup[i].localPort = atoi(strtok(nullptr, ",\n"));    //port

            i++;
            fgets(buffer, 2048, in);
         }
         participants = i;
      }
      else { printf("Failed to load file... Err: %d\n", err); }
      fclose(in);
   }

   ImGui::SameLine();
   if (ImGui::Button("Save To File")) {
      FILE* out;
      int err = fopen_s(&out, "saves/ggpo_session_setup.csv", "w");

      if (err == 0) {
         fprintf(out, "Me,Host,Player Number,Type,IP Address,Port\n");
         for (int i = 0; i < participants; i++) {
            fprintf(out, "%d,", hostSetup[i].me);
            fprintf(out, "%d,", hostSetup[i].host);
            fprintf(out, "%d,", hostSetup[i].playerType);
            fprintf(out, "%s,", hostSetup[i].ipAddress);
            fprintf(out, "%d,", hostSetup[i].localPort);
            fprintf(out, "\n");
         }
      }
      else { printf("Failed to create file... Err: %d\n", err); }
      fclose(out);
   }

   ImGui::SameLine();
   if (ImGui::Button("Clear Setup")) {
      for (int i = 0; i < participants; i++) {
         hostSetup[i].me = false;
         hostSetup[i].host = false;
         hostSetup[i].playerType = 0;
         hostSetup[i].localPort = 7001;
      }
      participants = 2;
   }


   ImGui::PopItemWidth();
   ImGui::NewLine();

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
      ImGui::Text(ggpoShowStatus(game, i));

      ImGui::PopItemWidth();
      ImGui::PopID();
   }
   ImGui::PopID();
   ImGui::NewLine();

   if (ImGui::Button("Start Session")) {
      ggpoCreateSession(game, hostSetup, participants);
      //ImGui::ProgressBar(0, ImVec2(0.0f, 0.0f));
   }
   ImGui::SameLine();
   if (ImGui::Button("End Session")) {
      ggpoEndSession(game);
   }

   static bool readySent = false;
   static bool replyRead = false;

   int ready = true;
   for (int i = 0; i < participants; i++) {
      if (game->net->connections[i].state == 2) {
         continue;
      }
      else { ready = false; }
   }

   if (ready == true) {
      if (ImGui::Button("Start Game")) {
         gameStartMatch(game);
      }
   }

   //if (game->net && game->net->connections[game->net->myConnNum].state == 2) {
   //   if (ImGui::Button("Send/Receive Seed")) {
   //      if (readySent == false && game->net->localPlayer == 1) {
   //         game->seed = time(0);
   //         ggpoSendMessage(game->seed);
   //         readySent = true;
   //      }
   //      else if (readySent == false && game->net->localPlayer != 1) {
   //         ggpoSendMessage(1);
   //         readySent = true;
   //      }
   //   }

   //   static int waitTime = 0;
   //   while (readySent == true) {
   //      ggpoSendMessage(1);
   //      sdlSleep(5000);
   //      if (game->net->localPlayer != 1) {
   //         if (game->inputs[0].timer > 0) {
   //            game->seed = game->inputs[0].msg;
   //            readySent = false;
   //            gameStartMatch(game);
   //         }
   //      }
   //      else if (game->net->localPlayer == 1) {
   //         if (game->inputs[1].timer > 0) {
   //            readySent = false;
   //            gameStartMatch(game);
   //         }
   //      }
   //      waitTime++;
   //   }
   //}

   ImGui::PopFont();
   ImGui::End();
}