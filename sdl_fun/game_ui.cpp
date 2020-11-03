#include "game_ui.h"

#include "board.h"
#include "serialize.h"
#include "netplay.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

#include <thread>

std::vector <std::string> ui_messages;

void ggpoSessionUI(Game* game, bool* p_open);

const char* credits = R"(
A special thanks goes out to:
Stephanie Anderson
Brandon Townsend
Sean Hunter
...
)";

//Struct to contain information about the popup
struct popupInfo {
   bool isOpen = false;  //Is it currently open
   bool triggered = false;  //Signal that the popup should be opened
   int other = 0;  //Random other info
};

std::map <PopupType, popupInfo> popups;  //Map to hold popups by type

//External API to trigger popup
void popupEnable(PopupType popup, int other) {
   popups[popup].triggered = true;
   popups[popup].other = other;
}

//Should we OpenPopup with ImGui
bool popupOpen(PopupType popup) {
   if (popups[popup].isOpen == false && popups[popup].triggered == true) {
      return true;
   }
   else { return false; }
}

//Is the popup open now?
bool popupStatus(PopupType popup) {
   return popups[popup].isOpen;
}

//Turn it off so it can be triggered again
void popupDisable(PopupType popup) {
   popups[popup].triggered = false;
   popups[popup].isOpen = false;
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

//For green helper text
static void helpfulText(const char* input) {
   ImGui::TextColored(ImVec4(0.1f, 0.9f, 0.1f, 1.0f), input);
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

      static bool showMultiPlayer = false;
      if (ImGui::Button("Multiplayer", ImVec2{ width, 0 })) {
         showMultiPlayer = true;
      }
      ImGui::NewLine();

      if (showMultiPlayer && game->playing == false) {
         multiplayerUI(game, &showMultiPlayer);
      }
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
      ImGui::BeginChild(playerName, ImVec2{ (float)game->settings.tWidth * (game->settings.bWidth), (float)game->settings.tHeight * (game->settings.bHeight) }, true, 0);
      ImVec2 csPos = ImGui::GetCursorScreenPos();

      //This is the secret sauce to render a texture in ImGui
      //The ImVec2{ 0, 1 }, ImVec2{ 1, 0 } is because it uses a different coordinate system by default
      if (game->fbos[index]) {
         ImGui::Image((void*)(intptr_t)game->fbos[index]->texture, { game->fbos[index]->w, game->fbos[index]->h }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
      }

      Board* board = game->boards[index];
      if (board->visualEvents[visual_clear].active == true) {
         VisualEvent e = board->visualEvents[visual_clear];
         if (board->chain > 1 || board->boardStats.lastCombo > 3) {
            char clearText[10];
            if (board->chain > 1) { sprintf(clearText, "%d Chain", board->chain); }
            else if (board->boardStats.lastCombo > 3) { sprintf(clearText, "%d Combo", board->boardStats.lastCombo); }
            ImVec2 textSize = ImGui::CalcTextSize(clearText);

            ImGui::GetWindowDrawList()->AddRectFilled({ e.pos.x + csPos.x, e.pos.y }, { e.pos.x + csPos.x + textSize.x, e.pos.y + textSize.y }, IM_COL32(0, 0, 0, 200));
            ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), { e.pos.x + csPos.x, e.pos.y }, IM_COL32_WHITE, clearText, NULL);
         }
      }

      ImGui::EndChild();
      ImGui::PopStyleVar();
}

//The popup window that shows a summary of a game after bust
static void _gameResults(Game* game) {
   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
   float width = ImGui::GetContentRegionAvailWidth();
   ImVec2 wSize = ImGui::GetWindowSize();
   float cursorY = ImGui::GetCursorPosY();
   for (auto&& board : game->boards) {
      char playerName[20] = "Player";
      sprintf(playerName, "Player %d", board->team);
      ImGui::BeginChild(playerName, { width / game->boards.size(), (wSize.y - cursorY) * 0.9f });
      ImGui::Text("Player: %d", board->team);
      ImGui::NewLine();
      int apm = (board->boardStats.apm / (board->game->timer / 1000.0f)) * 60.0f;
      int danger = board->boardStats.dangeresque / 60.0f;
      ImGui::Text("APM: %d", apm);
      ImGui::Text("Cleared Tiles: %d", board->boardStats.clears);
      ImGui::Text("Danger Time: %d", danger);
      ImGui::Text("Garbage destroyed: %d", board->boardStats.garbageCrushed);
      for (auto&& chain : board->boardStats.chainCounts) {
         ImGui::Text("%d Chains: %d", chain.first, board->boardStats.chainCounts[chain.first]);
      }
      for (auto&& combo : board->boardStats.comboCounts) {
         ImGui::Text("%d Combos: %d", combo.first, board->boardStats.comboCounts[combo.first]);
      }
      ImGui::EndChild();
      ImGui::SameLine();
   }
   ImGui::PopStyleVar();
   ImGui::NewLine();
}

//Draw the window and child regions for the board texture to be rendered in
void boardUI(Game* game) {
   if (game->playing == true) {

      ImGui::PushFont(game->fonts[20]);
      ImGui::Begin("Drop and Swap");

      _drawBoardTexture(game, 0);
      ImGui::SameLine();

      ImGui::BeginChild("Game Info", ImVec2{ ImGui::GetWindowContentRegionWidth() * 0.2f, (float)game->settings.tHeight * (game->settings.bHeight) }, true, 0);

      ImGui::Text("Frame Count: %d", game->frameCount);
      if (game->players > 1) { ImGui::Text("Time Sync: %d", game->net->timeSync); }
      if (game->timer > 0) {
         ImGui::Text("FPS: %0.1f", (1000 / game->kt.fps) );
      }

      static int bustee = 0;
      for (auto&& board : game->boards) {
         ImGui::NewLine();

         //Board Stats
         ImGui::Text("Player %d", board->team);
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
            bustee = board->team;
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
      if (popupOpen(Popup_GameOver) == true) { 
         ImGui::SetNextWindowSize({ 600, 500 });
         ImGui::OpenPopup("Game Over"); 
         popups[Popup_GameOver].isOpen = true;
      }
      if (ImGui::BeginPopupModal("Game Over") ) {
         if (popups[Popup_GameOver].isOpen == false) { ImGui::CloseCurrentPopup(); }
         ImGui::PushFont(game->fonts[20]);
         ImGui::Text("Player %d lost or something...", bustee);
         ImGui::NewLine();
         _gameResults(game);
         ImGui::PopFont();
         if (ImGui::Button("Accept Defeat")) {
            gameEndMatch(game);
            ImGui::CloseCurrentPopup();
            popupDisable(Popup_GameOver);
         }
         ImGui::EndPopup();
      }

      //Disconnect popup
      if (popupOpen(Popup_Disconnect) == true && popups[Popup_GameOver].isOpen == false) {
         ImGui::OpenPopup("Player Disconnecting"); 
         popups[Popup_Disconnect].isOpen = true;
      }
      if (ImGui::BeginPopupModal("Player Disconnecting", NULL, ImGuiWindowFlags_AlwaysAutoResize) ) {
         if (popups[Popup_Disconnect].isOpen == false) { ImGui::CloseCurrentPopup(); }
         else {
            if (game->net->timeSync == 0) { game->net->timeSync = 10; }
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
         }
         ImGui::EndPopup();
      }

      //static int fCount = 0;
      //if (popupOpen(Popup_Waiting) == true) { 
      //   ImGui::OpenPopup("Waiting for Player to Catch Up"); 
      //   popups[Popup_Waiting].isOpen = true;
      //   fCount = popups[Popup_Waiting].other;
      //   game->paused = true;
      //}
      //if (ImGui::BeginPopupModal("Waiting for Player to Catch Up")) {
      //   if (fCount == 0) {
      //      ImGui::CloseCurrentPopup();
      //      popupDisable(Popup_Waiting);
      //      game->paused = false;
      //   }
      //   if (ImGui::Button("Quit")) {
      //      gameEndMatch(game);
      //      ImGui::CloseCurrentPopup();
      //      popupDisable(Popup_Waiting);
      //      game->paused = false;
      //      fCount = 0;
      //   }
      //   fCount--;
      //   ImGui::EndPopup();
      //}

      ImGui::End();
   }
}

//Add a red colored button that is the width fo the window (section break)
static void _addSection(const char* name) {
   float width = ImGui::GetWindowContentRegionWidth();
   ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.6f, 0.6f));
   ImGui::NewLine();
   ImGui::Button(name, ImVec2{ width, 0 });
   ImGui::PopStyleColor();
}

//Helper function to provide hotkeys or buttons
static void _explainControls(Game* game, int controls) {
   //todo make it configurable later
   float width = ImGui::GetWindowContentRegionWidth();
   ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.6f, 0.6f));
   ImGui::NewLine();
   ImGui::Button("Controls", ImVec2{ width, 0 });

   int ratio = 3;
   if (controls == 0) {
      ImGui::Button("Movement", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("Arrow Keys");
      ImGui::Button("Swap", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("SPACEBAR");
      ImGui::Button("Pause", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("RETURN");
      ImGui::Button("Nudge", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("R");
   }

   else if (controls == 1) {
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

   if (ImGui::CollapsingHeader("Display and Sound")) {
      static int backgroundMusic = 0;
      //ImGui::Combo("Background Music", &backgroundMusic, "On\0Off\0");

      ImGui::Combo("Sound Effects", &game->sounds, "On\0Off\0");

      static int tileSize = 0;
      ImGui::Combo("Tile Size", &tileSize, "Normal\0Small\0Tiny\0");
      if (tileSize == 0) { game->settings.tWidth = game->settings.tHeight = 64; }
      else  if (tileSize == 1) { game->settings.tWidth = game->settings.tHeight = 32; }
      else { game->settings.tWidth = game->settings.tHeight = 16; }

      static bool vsync = false;
      ImGui::Checkbox("Vsync", &vsync);
      if (vsync == true && game->vsync == -1) { sdlSetVsync(game, true); }
      if (vsync == false && game->vsync == 0) { sdlSetVsync(game, false); }

   }
   
   if (ImGui::CollapsingHeader("Controls")) {
      static int gameControls = 0;
      ImGui::Combo("Show Controls", &gameControls, "Keyboard\0Controller\0");
      _explainControls(game, gameControls);
      ImGui::NewLine();
   }

   if (ImGui::CollapsingHeader("Game Settings") ) {
      ImGui::InputInt("Board Width", &game->settings.bWidth);
      ImGui::InputInt("Board Height", &game->settings.bHeight);
      ImGui::SliderScalar("AI Speed", ImGuiDataType_U32, &game->aiDelay[0], &game->aiDelay[1], &game->aiDelay[2]);
      ImGui::SliderScalar("Remove Clear", ImGuiDataType_U32, &game->timings.removeClear[0], &game->timings.removeClear[1], &game->timings.removeClear[2]);
      ImGui::SliderScalar("Fall Delay", ImGuiDataType_U32, &game->timings.fallDelay[0], &game->timings.fallDelay[1], &game->timings.fallDelay[2]);
      ImGui::SliderScalar("Grace Period", ImGuiDataType_U32, &game->timings.gracePeriod[0], &game->timings.gracePeriod[1], &game->timings.gracePeriod[2]);
      ImGui::SliderScalar("Deploy Time", ImGuiDataType_U32, &game->timings.deployTime[0], &game->timings.deployTime[1], &game->timings.deployTime[2]);
      ImGui::SliderScalar("Start Countdown", ImGuiDataType_U32, &game->timings.countIn[0], &game->timings.countIn[1], &game->timings.countIn[2]);
      ImGui::SliderScalar("Land Time", ImGuiDataType_U32, &game->timings.landPause[0], &game->timings.landPause[1], &game->timings.landPause[2]);
      ImGui::SliderScalar("Enter Silvers", ImGuiDataType_U32, &game->timings.enterSilver[0], &game->timings.enterSilver[1], &game->timings.enterSilver[2]);
   }

   if (ImGui::CollapsingHeader("Debug")) {
      ImGui::Checkbox("Show Debug Options", &game->debug);

      if (game->debug == true) {
         static bool showOldSession = false;
         if (showOldSession == false) {
            if (ImGui::Button("Show Old Session Window")) {
               showOldSession = true;
            }
         }
         else {
            if (ImGui::Button("Hide Old Session Window")) {
               showOldSession = false;
            }
         }
         if (showOldSession == true) { ggpoSessionUI(game, &showOldSession); }

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
         ImGui::Checkbox("AI", &game->ai);
         ImGui::Checkbox("Sync test", &game->net->syncTest);
      }
   }

   ImGui::End();
}

void onePlayerOptions(Game* game) {

   if (ImGui::Button("Load Game State")) { gameLoadState(game, "saves/game_state.dat"); }
   if (ImGui::Button("Save Game State")) { gameSaveState(game, "saves/game_state.dat"); }

   if (game->debug == true) {
      ImGui::Checkbox("Turn On AI", &game->ai);
      ImGui::SliderScalar("AI Delay", ImGuiDataType_U32, &game->aiDelay[0], &game->aiDelay[1], &game->aiDelay[2]);

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

   if (!ImGui::Begin("Connection Setup", p_open)) {
      ImGui::End();
      return;
   }

   ImGui::PushFont(game->fonts[13]);
   if (game->debug == true) {
      ImGui::Checkbox("DEBUG: Sync test", &game->net->syncTest);
      ImGui::SameLine(); HelpMarker("This is for detecting desynchronization issues in ggpo's rollback system.");
   }

   helpfulText("The seed is used to generate a random board. It must be the same for both players.");
   ImGui::NewLine();
   static int seed = 0;
   ImGui::DragInt("Seed", &seed, 1, 1.0, 5000);
   game->seed = seed;

   ImGui::NewLine();

   if (ImGui::CollapsingHeader("Board Setup")) {
      ImGui::InputInt("Board Width", &game->settings.bWidth);
      ImGui::InputInt("Board Height", &game->settings.bHeight);
      ImGui::Checkbox("I AM A ROBOT", &game->ai);
      ImGui::SliderScalar("AI Delay", ImGuiDataType_U32, &game->aiDelay[0], &game->aiDelay[1], &game->aiDelay[2]);
      ImGui::NewLine();
   }

   static bool manualPorts = false;

   if (ImGui::CollapsingHeader("Connection Options") ) {
      ImGui::Checkbox("Use UPNP connection", &game->net->upnp);
      ImGui::SameLine(); HelpMarker("Universal Plug and Play must be used if you aren't on the same internal network.");
      ImGui::Checkbox("Manual Ports", &manualPorts);

      ImGui::NewLine();
      helpfulText("These options have to be the same for all players or bad things will happen...");
      ImGui::SliderScalar("Frame Delay", ImGuiDataType_U32, &game->net->frameDelay[0], &game->net->frameDelay[1], &game->net->frameDelay[2]);
      ImGui::SliderScalar("Disconnect Wait", ImGuiDataType_U32, &game->net->disconnectTime[0], &game->net->disconnectTime[1], &game->net->disconnectTime[2]);
      ImGui::NewLine();
   }

   static unsigned short participants = 2;
   int pMin = 2;
   int pMax = GAME_MAX_PLAYERS;

   if (ImGui::CollapsingHeader("Player Info", ImGuiTreeNodeFlags_DefaultOpen)) {
      helpfulText("Select the number of people (players/spectators) who will take part in the match. The host must be a player.");
      ImGui::NewLine();

      ImGui::PushItemWidth(140);
      ImGui::Text("Players");
      ImGui::SameLine();
      ImGui::SliderScalar("##Participants", ImGuiDataType_U8, &participants, &pMin, &pMax);
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
               game->net->hostSetup[i].me = atoi(strtok(buffer, ",\n"));            // me
               game->net->hostSetup[i].host = atoi(strtok(nullptr, ",\n"));         // host
               game->net->hostSetup[i].playerType = atoi(strtok(nullptr, ",\n"));   // player type
               game->net->hostSetup[i].team = atoi(strtok(nullptr, ",\n"));         // team
               strcpy(game->net->hostSetup[i].ipAddress, strtok(nullptr, ",\n"));   // ip address
               game->net->hostSetup[i].localPort = atoi(strtok(nullptr, ",\n"));    //port

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
               fprintf(out, "%d,", game->net->hostSetup[i].me);
               fprintf(out, "%d,", game->net->hostSetup[i].host);
               fprintf(out, "%d,", game->net->hostSetup[i].playerType);
               fprintf(out, "%d,", game->net->hostSetup[i].team);
               fprintf(out, "%s,", game->net->hostSetup[i].ipAddress);
               fprintf(out, "%d,", game->net->hostSetup[i].localPort);
               fprintf(out, "\n");
            }
         }
         else { printf("Failed to create file... Err: %d\n", err); }
         fclose(out);
      }

      ImGui::SameLine();
      if (ImGui::Button("Clear Setup")) {
         for (int i = 0; i < participants; i++) {
            game->net->hostSetup[i].me = false;
            game->net->hostSetup[i].host = false;
            game->net->hostSetup[i].playerType = 0;
            game->net->hostSetup[i].localPort = 7001 + i;
         }
         participants = 2;
      }
      ImGui::NewLine();
      ImGui::PopItemWidth();

      ImGui::PushID("Player Info Set");
      for (int i = 0; i < participants; i++) {

         ImGui::PushID(i);
         ImGui::PushItemWidth(80);

         int pNum = i + 1;
         ImGui::TextColored(ImVec4(1.0f, 1.0f * pNum / 4, 0.0f, 1.0f), "Player %d", pNum);
         ImGui::SameLine();

         if (ImGui::Checkbox("Me", &game->net->hostSetup[i].me)) {
            for (int j = 0; j < participants; j++) {
               if (game->net->hostSetup[j].me == true && i != j) { game->net->hostSetup[j].me = false; }
            }
         }
         ImGui::SameLine();

         if (ImGui::Checkbox("Host", &game->net->hostSetup[i].host)) {
            for (int j = 0; j < participants; j++) {
               if (game->net->hostSetup[j].host == true && i != j) { game->net->hostSetup[j].host = false; }
            }
         }
         ImGui::SameLine();
         ImGui::Combo("Player Type", &game->net->hostSetup[i].playerType, "Player\0Spectator\0");
         ImGui::SameLine();
         ImGui::Combo("Team", &game->net->hostSetup[i].team, "One\0Two\0");
         ImGui::SameLine();
         ImGui::InputText("IP Address", game->net->hostSetup[i].ipAddress, IM_ARRAYSIZE(game->net->hostSetup[i].ipAddress));
         ImGui::SameLine();
         if (manualPorts) {
            ImGui::DragScalar("Port", ImGuiDataType_U16, &game->net->hostSetup[i].localPort, 1.0);
            ImGui::SameLine(); HelpMarker("Select a unique port number that you will use to send information to host.");
            ImGui::SameLine();
         }
         ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ggpoShowStatus(game, i));

         ImGui::PopItemWidth();
         ImGui::PopID();
      }
      ImGui::PopID();
      ImGui::NewLine();
   }

   static bool connectStats = false;
   if (game->net->ggpo == nullptr) {
      if (ImGui::Button("Open Connection")) {
         connectStats = true;
         int pNum = 1;
         for (int i = 0; i < participants; i++) {
            if (manualPorts == false) { game->net->hostSetup[i].localPort = 7001 + i; }
            if (game->net->hostSetup->playerType == 0) { 
               game->net->hostSetup[i].pNum = pNum; 
               pNum++; 
            }
         }
         game->players = pNum;
         ggpoCreateSession(game, game->net->hostSetup, participants);
      }
   }

   if (connectStats) { connectStatusUI(game, &connectStats); }

   static bool netStats = false;
   if (game->net && game->net->ggpo) {
      if (ImGui::Button("Connection Info")) {
         netStats = true;
      }
      if (netStats == true) { ggpoNetStatsUI(game, &netStats); }

      ImGui::SameLine();
      if (ImGui::Button("Close Connection")) {
         ggpoEndSession(game);
         connectStats = netStats = false;
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

//Gives feedback (messages) about the progress of the GGPO connection thread
void connectStatusUI(Game* game, bool* p_open) {
   if (!ImGui::Begin("Connection Status", p_open)) {
      ImGui::End();
      return;
   }

   if (game->net && game->net->connections[game->net->myConnNum].state == Running) {
      game->net->messages.clear();
      *p_open = false;
   }
   else {
      for (auto&& message : game->net->messages) {
         ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), message.c_str());
      }
   }

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
      ImGui::Text("%.2f kilobytes/sec sent", stats.network.kbps_sent);
      ImGui::Text("Send queue length: %d", stats.network.send_queue_len);
      ImGui::Text("Receive queue length: %d", stats.network.recv_queue_len);
      ImGui::Text("Ping: %d ", stats.network.ping);
      ImGui::Text("Local Frames behind: %d", stats.timesync.local_frames_behind);
      ImGui::Text("Remote frames behind: %d", stats.timesync.remote_frames_behind);
   }

   ImGui::End();
}

ServerStatus serverStatus = server_none;
ClientStatus clientStatus = client_none;
std::thread serverThread;
std::thread clientThread;
bool clientRunning = false;
bool serverRunning = false;

static void _serverLoopUI(Game* game, int people[], bool &connectStats) {
   //This code is for the server
   if (serverRunning == false) {
      if (ImGui::Button("Find Players")) {
         serverStatus = server_started;
         //This is the thread for the server loop
         serverThread = std::thread(tcpServerLoop, 7000, people[0] - 1, std::ref(serverStatus), std::ref(serverRunning));
         serverThread.detach();
         connectStats = true;
      }
      ImGui::SameLine();
   }

   if (ImGui::Button("Reset Connection")) {
      tcpReset();
      serverStatus = server_none;
      connectStats = false;
   }

   if (serverStatus >= server_waiting) {
      //This is the player information table
      ImGui::PushID("Player Info Set");
      for (int i = 0; i < people[0]; i++) {
         ImGui::PushID(i);  //So widgets don't name collide
         ImGui::PushItemWidth(100);
         SocketInfo sock = getSocket(i - 1);
         if (i == 0) { ImGui::Text(game->p.name); }
         else { ImGui::Text(sock.name); }
         ImGui::SameLine();
         ImGui::SetCursorPosX(224);
         int minPNum = 1;
         ImGui::SliderScalar("Player Number", ImGuiDataType_U32, &game->net->hostSetup[i].pNum, &minPNum, &people[0]);
         ImGui::SameLine();
         ImGui::Combo("Team", &game->net->hostSetup[i].team, "One\0Two\0");
         ImGui::SameLine();
         ImGui::Combo("Player Type", &game->net->hostSetup[i].playerType, "Player\0Spectator\0");
         ImGui::SameLine();
         ImGui::Text(inet_ntoa(sock.address.sin_addr));

         ImGui::PopID();
      }
      ImGui::PopID();

      if (ImGui::Button("Start Game")) {
         game->players = people[0];
         game->seed = time(0);
         for (int i = 0; i < game->players; i++) {
            SocketInfo sock = getSocket(i - 1);
            if (i == 0) {
               strcpy(game->net->hostSetup[i].name, game->p.name);
               game->net->hostSetup[i].host = true;
               game->net->hostSetup[i].me = true;
               strcpy(game->net->hostSetup[i].ipAddress, "127.0.0.1");
            }
            else {
               strcpy(game->net->hostSetup[i].name, sock.name);
               game->net->hostSetup[i].host = false;
            }
            strcpy(game->net->hostSetup[i].ipAddress, inet_ntoa(sock.address.sin_addr));
            game->net->hostSetup[i].localPort = 7001 + i;
         }
         serverStatus = server_send;
      }
   }

   if (game->winsockRunning == true) {
      if (serverStatus == server_done) {
         tcpCloseConnections();
         serverStatus = server_none;
         connectStats = false;
         ggpoCreateSession(game, game->net->hostSetup, game->players);
      }
   }
}

static void _clientLoopUI(Game* game, char ipAddress[], bool& connectStats) {
   if (clientRunning == false) {
      if (ImGui::Button("Connect to Host")) {
         clientStatus = client_started;
         //This is the client loop thread
         clientThread = std::thread(tcpClientLoop, 7000, ipAddress, std::ref(clientStatus), game->p.name, std::ref(clientRunning));
         clientThread.detach();
         connectStats = true;
      }
      ImGui::SameLine();
   }

   if (ImGui::Button("Reset Connection")) {  //This just nukes everything
      tcpReset();
      clientStatus = client_none;
      connectStats = false;
   }

   //This displays the game information once it is received
   if (clientStatus >= client_received) {
      float width = ImGui::GetContentRegionAvailWidth();
      for (int i = 0; i < game->players; i++) {
         ImGui::BeginChild(game->net->hostSetup[i].name, { width / game->players, 200 });
         ImGui::Text(game->net->hostSetup[i].name);
         ImGui::Text(game->net->hostSetup[i].ipAddress);
         ImGui::Text("Host: %d", game->net->hostSetup[i].host);
         ImGui::Text("Me: %d", game->net->hostSetup[i].me);
         ImGui::Text("Player: %d", game->net->hostSetup[i].pNum);
         ImGui::Text("Team: %d", game->net->hostSetup[i].team);
         ImGui::EndChild();
         if (i + 1 != game->players) { ImGui::SameLine(); }
      }
      if (ImGui::Button("Start Game")) {
         for (int i = 0; i < game->players; i++) {
            if (i == 0) {
               strcpy(game->net->hostSetup[i].ipAddress, ipAddress);
            }
         }
         clientStatus = client_loaded;
         ggpoCreateSession(game, game->net->hostSetup, game->players);
      }
   }
   if (game->winsockRunning == true) {
      if (clientStatus == client_done) {
         tcpCloseConnections();
         clientStatus = client_none;
         connectStats = false;
      }
   }
}

void multiplayerUI(Game* game, bool* p_open) {
   if (!ImGui::Begin("Connection Setup", p_open)) {
      ImGui::End();
      return;
   }

   static bool showDebugConn = false;
   if (ImGui::Button("Show Conn State")) {
      showDebugConn = true;
   }
   if (showDebugConn == true) { debugConnections(game, &showDebugConn); }

   static bool isServer = false;
   static bool connectStats = false;
   static char ipAddress[20] = "127.0.0.1";
   static int people[3] = { 2, 2, 4 };

   ImGui::Checkbox("Host a Game", &isServer);

   if (isServer == true) {
      ImGui::NewLine();

      if (ImGui::CollapsingHeader("Board Setup")) {
         ImGui::InputInt("Board Width", &game->settings.bWidth);
         ImGui::InputInt("Board Height", &game->settings.bHeight);
         ImGui::NewLine();
      }

      if (ImGui::CollapsingHeader("GGPO Options")) { //todo send this with game info
         ImGui::SliderScalar("Frame Delay", ImGuiDataType_U32, &game->net->frameDelay[0], &game->net->frameDelay[1], &game->net->frameDelay[2]);
         ImGui::SliderScalar("Disconnect Wait", ImGuiDataType_U32, &game->net->disconnectTime[0], &game->net->disconnectTime[1], &game->net->disconnectTime[2]);
         ImGui::NewLine();
      }
   }

   ImGui::NewLine();
   ImGui::InputText("Your Name", game->p.name, IM_ARRAYSIZE(game->p.name));
   if (isServer == false) { ImGui::InputText("Host IP", ipAddress, IM_ARRAYSIZE(ipAddress)); }
   if (isServer == true) {
      ImGui::SliderScalar("Total Players", ImGuiDataType_U32, &people[0], &people[1], &people[2]);
   }

   ImGui::NewLine();
   //If you're a server, use this loop
   if (isServer == true) { _serverLoopUI(game, &people[0], connectStats); }

   //If you're a client, use this loop
   else if (isServer == false) { _clientLoopUI(game, ipAddress, connectStats); }

   //Are we viewing the Connection Messages?
   if (connectStats) { connectStatusUI(game, &connectStats); }

   //If GGPO is running then start the game!
   if (game->net->connections[game->net->myConnNum].state == Running && game->playing == false && game->winsockRunning == true) {
      gameStartMatch(game);
   }

   ImGui::End();
}

void debugConnections(Game* game, bool* p_open) {
   if (!ImGui::Begin("Conn State", p_open)) {
      ImGui::End();
      return;
   }

   ImGui::Text("Winsock running: %d", game->winsockRunning);
   ImGui::Text("UPNP running: %d", game->net->upnp);
   ImGui::Text("Server thread running: %d", serverRunning);
   ImGui::Text("Server status: %d", serverStatus);
   ImGui::Text("Client thread running: %d", clientRunning);
   ImGui::Text("Client status: %d", clientStatus);

   _connectionInfo();

   ImGui::End();
}