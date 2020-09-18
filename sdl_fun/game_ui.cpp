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

std::map <PopupType, bool> popups = {
   {Popup_GameOver, false},
   {Popup_Disconnect, false},
   {Popup_Waiting, false},
};

void popupEnable(PopupType popup) {
   popups[popup] = true;
}

bool popupStatus(PopupType popup) {
   return popups[popup];
}

void popupDisable(PopupType popup) {
   popups[popup] = false;
}

//Tooltip helper text
static void HelpMarker(const char* desc)
{
   ImGui::TextDisabled("(?)");
   if (ImGui::IsItemHovered())
   {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      ImGui::TextUnformatted(desc);
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
   }
}

//Main menu UI
void mainUI(Game* game) {

   ImGui::PushFont(game->fonts[20]);
   if (!ImGui::Begin("Menu")) {
      ImGui::PopFont();
      ImGui::End();
      return;
   }

   ImGui::NewLine();
   float width = ImGui::GetWindowContentRegionWidth();

   if (game->playing == true) {
      if (ImGui::Button("End Game", ImVec2{ width, 0 })) {
         //ImGui::OpenPopup("Game Over");
         gameEndMatch(game);
      }
   }

   if (game->playing == false) {
      if (ImGui::Button("One Player", ImVec2{ width, 0 })) {
         game->players = 1;
         gameStartMatch(game);
      }
      ImGui::NewLine();

      static bool showGGPOSession = false;
      if (ImGui::Button("Two Player", ImVec2{ width, 0 })) {
         game->players = 2;
         showGGPOSession = true;
      }
      if (showGGPOSession && game->playing == false) {
         ggpoSessionUI(game, &showGGPOSession);
      }
      ImGui::NewLine();
   }

   static bool showSettings = false;
   if (ImGui::Button("Settings", ImVec2{ width, 0 }) ) {
      showSettings = true;
   }
   if (showSettings) {
      gameSettingsUI(game, &showSettings);
   }

   ImGui::PopFont();
   ImGui::End();
}

//Helper to draw the board texture into the ImGui child window
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

      for (int i = 0; i < game->boards[index]->visualEvents.size(); i++) {
         VisualEvent e = game->boards[index]->visualEvents[i];
         if (e.effect == visual_clear && e.end > game->timer) {
            ImGui::SetCursorScreenPos(ImVec2{ screenPos.x + e.pos.x, screenPos.y + e.pos.y });
            ImGui::Text("%d X", game->boards[index]->chain);
         }
      }
      ImGui::SetCursorScreenPos(screenPos);

      ImGui::EndChild();
      ImGui::PopStyleVar();
}

//Draw the window and child regions for the board texture to be rendered in
void boardUI(Game* game) {
   if (game->playing == true) {

      ImGui::PushFont(game->fonts[20]);
      ImGui::Begin("Drop and Swap");

      _drawBoardTexture(game, 0);
      ImGui::SameLine();

      ImGui::BeginChild("Game Info", ImVec2{ ImGui::GetWindowContentRegionWidth() * 0.2f, (float)game->tHeight * (game->bHeight) }, true, 0);

      static int bustee = 0;
      for (auto&& board : game->boards) {
         ImGui::NewLine();

         //Board Stats
         ImGui::Text("Player %d", board->player);
         ImGui::Text("Last chain: %d", board->boardStats.lastChain);
         if (board->game->timer > 0) {
            int apm = (board->boardStats.apm / (board->game->timer / 1000.0f)) * 60.0f;
            ImGui::Text("APM: %d", apm);
         }
         ImGui::Text("Dangeresque: %0.1f s", board->boardStats.dangeresque / 60.0f);

         ImGui::Text("Pause Time: %d s", board->pauseLength / 1000);
         ImGui::Text("Game Time: %d s", game->timer / 1000);
         ImGui::NewLine();

         if (board->bust == true && popupStatus(Popup_GameOver) == false ) {
            bustee = board->player;
            popupEnable(Popup_GameOver);
         }
      }
      ImGui::EndChild();

      if (game->players > 1) { 
         ImGui::SameLine();
         _drawBoardTexture(game, 1); 
      }
      else {
         ImGui::SameLine();
         ImGui::BeginChild("One Player Options");
         onePlayerOptions(game);
         ImGui::EndChild();
      }
      ImGui::PopFont();

      //Game over popup
      if (popupStatus(Popup_GameOver) == true) {
         ImGui::OpenPopup("Game Over");
         if (ImGui::BeginPopupModal("Game Over")) {
            ImGui::Text("Player %d lost or something...", bustee);
            if (ImGui::Button("Accept Defeat")) {
               gameEndMatch(game);
               ImGui::CloseCurrentPopup();
               popupDisable(Popup_GameOver);
            }
            ImGui::EndPopup();
         }
      }

      //Disconnect popup
      if (popupStatus(Popup_Disconnect) == true && popupStatus(Popup_GameOver) == false) {
         ImGui::SetNextWindowSize({ 200, 200 });
         ImGui::OpenPopup("Player Disconnecting");
         if (ImGui::BeginPopupModal("Player Disconnecting")) {
            game->paused = true;
            int currentTime = game->kt.getTime();
            for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
               PlayerConnectionInfo connect = game->net->connections[i];
               if (connect.state == Disconnecting) {
                  float delta = (currentTime - connect.disconnect_start) / 1000;
                  ImGui::Text("Player %d", connect.handle); 
                  ImGui::ProgressBar(delta / connect.disconnect_timeout, ImVec2(0.0f, 0.0f));
               }
               if (connect.state == Disconnected) {
                  ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Gone baby gone");
               }
            }
            if (ImGui::Button("Bail out")) {
               gameEndMatch(game);
               ImGui::CloseCurrentPopup();
               popupDisable(Popup_Disconnect);
            }
            ImGui::EndPopup();
         }
      }

      //if (popupStatus(Popup_Waiting) == true) {
      //   ImGui::OpenPopup("Waiting for Player to Catch Up");
      //   if (ImGui::BeginPopupModal("Waiting for Player to Catch Up")) {
      //      if (ImGui::Button("Quit")) {
      //         gameEndMatch(game);
      //         ImGui::CloseCurrentPopup();
      //         popupDisable(Popup_Waiting);
      //      }
      //      ImGui::EndPopup();
      //   }
      //}

      ImGui::End();
   }
}

//Helper function to provide hotkeys or buttons
static void _explainControls(Game* game) {
   //todo make it configurable later
   float width = ImGui::GetWindowContentRegionWidth();
   ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.6f, 0.6f));
   ImGui::NewLine();
   ImGui::Button("Controls", ImVec2{ width, 0 });

   int ratio = 3;
   if (game->controls == 0) {
      ImGui::Button("Movement", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("Arrow Keys");
      ImGui::Button("Swap", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("SPACEBAR");
      ImGui::Button("Pause", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("RETURN");
      ImGui::Button("Nudge", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("R");
   }

   else if (game->controls == 1) {
      ImGui::Button("Movement", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("D Pad");
      ImGui::Button("Swap", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("A");
      ImGui::Button("Pause", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("Start");
      ImGui::Button("Nudge", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("RB Trigger");
   }
   ImGui::PopStyleColor();
}

void gameSettingsUI(Game* game, bool* p_open) {
   if (!ImGui::Begin("Game Settings", p_open) ) {
      ImGui::End();
      return;
   }

   static int backgroundMusic = 0;
   //ImGui::Combo("Background Music", &backgroundMusic, "On\0Off\0");

   if (game->playing == false) {

      ImGui::Combo("Sound Effects", &game->sounds, "On\0Off\0");
      ImGui::Combo("Show Controls", &game->controls, "Keyboard\0Controller\0");

      static int tileSize = 0;
      ImGui::Combo("Tile Size", &tileSize, "Normal\0Small\0Tiny\0");
      if (tileSize == 0) { game->tWidth = game->tHeight = 64; }
      else  if (tileSize == 1) { game->tWidth = game->tHeight = 32; }
      else { game->tWidth = game->tHeight = 16; }

      //ImGui::InputInt("Tile Width", &game->tWidth, 16);
      //ImGui::InputInt("Tile Height", &game->tHeight, 16);

      ImGui::InputInt("Board Width", &game->bWidth);
      ImGui::InputInt("Board Height", &game->bHeight);
   }
   
   _explainControls(game);
   ImGui::NewLine();

   ImGui::Checkbox("Show Debug Options", &game->debug);

   if (game->debug == true) {
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
   }

   ImGui::End();
}

void onePlayerOptions(Game* game) {

   if (ImGui::Button("Load Game State")) { gameLoadState(game, "saves/game_state.dat"); }
   if (ImGui::Button("Save Game State")) { gameSaveState(game, "saves/game_state.dat"); }

   if (game->debug == true) {
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
         if (ImGui::Button("Dumpstered")) {
            for (auto&& board : game->boards) {
               if (board) { garbageCreate(board, gWidth, gHeight, isMetal); }
            }
         }
         ImGui::InputInt("Garbage Width", &gWidth);
         ImGui::InputInt("Garbage Height", &gHeight);
         ImGui::Checkbox("Metal", &isMetal);
      }

      if (game->playing == true) {
         for (auto&& board : game->boards) {
            if (board) {
               float minFallSpeed = 0;
               float maxFallSpeed = 20.0;

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
}

//Show the connection window for GGPO... only for 2 players
void ggpoSessionUI(Game* game, bool* p_open) {

   if (!ImGui::Begin("Host Setup", p_open)) {
      ImGui::End();
      return;
   }

   ImGui::PushFont(game->fonts[13]);
   //Debug turn on sync test
   if (game->debug == true) {
      ImGui::Checkbox("DEBUG: sync test", &game->syncTest);
      ImGui::SameLine(); HelpMarker("This is for detecting desynchronization issues in ggpo's rollback system.");
   }

   static SessionInfo hostSetup[GAME_MAX_PLAYERS];

   ImGui::NewLine();

   static int seed = 0;
   ImGui::DragInt("Seed", &seed, 1, 1.0, 5000);
   game->seed = seed;
   ImGui::SameLine(); HelpMarker("Both Players must agree on the seed before the match starts.");
   ImGui::NewLine();

   static unsigned short participants = 2;
   int pMin = 2;
   int pMax = GAME_MAX_PLAYERS;

   ImGui::PushItemWidth(120);
   ImGui::SliderScalar("Participants", ImGuiDataType_U8, &participants, &pMin, &pMax);
   ImGui::SameLine(); HelpMarker("Select the number of people (players/spectators) who will take part in the match.\n The host must be a player.");
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
   ImGui::SameLine(); HelpMarker("You can save/load the connection setup below to a CSV in the game's assets folder.");

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
      ImGui::SameLine(); HelpMarker("Select a unique port number that you will use to send information to host.");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ggpoShowStatus(game, i) );

      ImGui::PopItemWidth();
      ImGui::PopID();
   }
   ImGui::PopID();
   ImGui::NewLine();

   if (game->net->ggpo == nullptr) {
      if (ImGui::Button("Open Connection")) {
         ggpoCreateSession(game, hostSetup, participants);
      }
   }

   static bool netStats = false;
   if (game->net && game->net->ggpo) {
      if (ImGui::Button("Connection Info")) {
         netStats = true;
      }
      if (netStats == true) { ggpoNetStatsUI(game, &netStats); }

      ImGui::SameLine();
      if (ImGui::Button("Close Connection")) {
         ggpoEndSession(game);
      }
      if (game->net && game->net->connections[game->net->myConnNum].state != Running) {
         ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Connecting...");
         ImGui::NewLine();
      }
      else if (game->net && game->net->connections[game->net->myConnNum].state == Running) {
         ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Ready");
         ImGui::NewLine();
      }
   }

   static bool readySent = false;

   int ready = true;
   for (int i = 0; i < participants; i++) {
      if (game->net->connections[i].state == Running) {
         continue;
      }
      else { ready = false; }
   }

   if (ready == true) {

      if (ImGui::Button("Start Game")) {
         gameStartMatch(game);
      }
      ImGui::SameLine(); HelpMarker("Try and hit start roughly at the same time, lol.");
   }

   ImGui::PopFont();
   ImGui::End();
}

void ggpoNetStatsUI(Game* game, bool* p_open) {

   if (!ImGui::Begin("Network Stats", p_open)) {
      ImGui::End();
      return;
   }

   GGPONetworkStats stats;
   for (int i = 0; i < game->players; i++) {
      if (game->net->localPlayer == i + 1) { continue; }
      ggpo_get_network_stats(game->net->ggpo, i + 1, &stats);

      ImGui::Text("Player %d Connection Info", i + 1);
      ImGui::Text("%.2f kilobytes/sec", stats.network.kbps_sent / 8.0);
      ImGui::Text("Ping: %d ", stats.network.ping);
      //ImGui::Text("Frames: %.1f ", stats.network.ping ? stats.network.ping * 60.0 / 1000 : 0);
      ImGui::Text("Local Frames behind: %d", stats.timesync.local_frames_behind);
      ImGui::Text("Remote frames behind: %d", stats.timesync.remote_frames_behind);
   }

   ImGui::End();
}