#include "game_ui.h"

#include "board.h"
#include "serialize.h"
#include "netplay.h"
#include "resources.h"

#include "imgui/imgui.h"
#include "win.h"

#include <thread>

void ggpoSessionUI(Game* game, bool* p_open);
void singlePlayerGame(Game* game, bool* p_open);
void multiHostOrGuest(Game* game, bool* p_open, bool* multiSetup, bool* isHost);
void multiplayerJoin(Game* game, bool* p_open);
void multiplayerHost(Game* game, bool* p_open);
void ggpoReadyModal(Game* game);
void replayUI(Game* game, bool* p_open);

void debugConnections(Game* game, bool* p_open);
void debugMultiplayerSetup(Game* game, bool* p_open);

//Globals used by TCP transfer threads
ServerStatus serverStatus = server_none;     //What stage of the game info transfer is the server in
ClientStatus clientStatus = client_none;     //What stage of the game info transfer is the client in
std::thread serverThread;                    //Is the server thread handle
std::thread clientThread;                    //Is the client thread handle
bool clientRunning = false;                  //Is the client thread running
bool serverRunning = false;                  //Is the server thread running


const char* credits = R"(
A special thanks goes out to:
Stephanie Anderson
Brandon Townsend
Sean Hunter
...
)";

ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

//Struct to contain information about the popup
struct popupInfo {
   bool isOpen = false;                      //Is it currently open
   bool triggered = false;                   //Signal that the popup should be opened
   int other = 0;                            //Random other info
};

//Map to hold popups by type
std::map <PopupType, popupInfo> popups; 

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

//Show helper text that is horizontally centered
static void helpfulCenterText(const char* input) {
   float width = ImGui::GetWindowContentRegionWidth();
   ImVec2 size = ImGui::CalcTextSize(input);
   ImGui::SetCursorPosX((width - size.x) / 2);
   helpfulText(input);
}

//Sets the next window to be centered
static void centerWindow(Game* game, ImVec2 size) {
   ImGui::SetNextWindowSize(size, ImGuiCond_Once);
   ImVec2 pos = { (game->windowWidth - size.x) / 2, (game->windowHeight - size.y) / 2 };
   ImGui::SetNextWindowPos(pos, ImGuiCond_Once);
}

//todo this isn't used and doesn't quite work... remove?
static void botRightButton(const char* input) {
   ImVec2 pad = ImGui::GetStyle().WindowPadding;
   ImVec2 win = ImGui::GetWindowSize();
   ImVec2 tSize = ImGui::CalcTextSize(input);
   ImGui::SetCursorPos({ win.x - (tSize.x + pad.x) * 2, win.y - (tSize.y + pad.y) * 2 });
   ImGui::Separator();
}

//Main menu UI
void mainUI(Game* game) {
   centerWindow(game, { 400, 300 });
   ImGui::PushFont(game->fonts[20]);
   if (!ImGui::Begin("Menu", (bool*)0, winFlags)) {
      ImGui::PopFont();
      ImGui::End();
      return;
   }

   ImGui::NewLine();
   float width = ImGui::GetWindowContentRegionWidth();

   if (game->playing == true) {
      helpfulCenterText("Hit Pause to Resume the Game");
      ImGui::NewLine();
      if (ImGui::Button("End Game", ImVec2{ width, 0 })) {
         //ImGui::OpenPopup("Game Over");
         gameEndMatch(game);
      }
      ImGui::NewLine();
   }

   if (game->playing == false) {
      static bool singlePlayer = false;
      if (ImGui::Button("One Player", ImVec2{ width, 0 })) {
         singlePlayer = true;
      }
      if (singlePlayer) { singlePlayerGame(game, &singlePlayer); }
      ImGui::NewLine();

      static bool showConnectionType = false;
      static bool showMultiPlayer = false;
      static bool isHost = false;
      if (ImGui::Button("Multiplayer", ImVec2{ width, 0 })) {
         showConnectionType = true;
      }
      ImGui::NewLine();

      if (showConnectionType && game->playing == false) {
         multiHostOrGuest(game, &showConnectionType, &showMultiPlayer, &isHost);
      }
      if (showMultiPlayer && game->playing == false && isHost == true) {
         multiplayerHost(game, &showMultiPlayer);
      }
      else if (showMultiPlayer && game->playing == false && isHost == false) {
         multiplayerJoin(game, &showMultiPlayer);
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

void singlePlayerGame(Game* game, bool* p_open) {
   centerWindow(game, { 600, 500 });
   if (!ImGui::Begin("Single Player", p_open, winFlags)) {
      ImGui::End();
      return;
   }

   ImGui::NewLine();
   float width = ImGui::GetWindowContentRegionWidth();
   if (ImGui::Button("Practice", ImVec2{ width, 0 })) {
      game->players = 1;
      game->settings.mode = single_player;
      gameStartMatch(game);
   }

   static bool replayWindow = false;
   ImGui::NewLine();
   if (ImGui::Button("Replay", ImVec2{ width, 0 })) {
      replayWindow = true;
   }
   if (replayWindow == true) { replayUI(game, &replayWindow); }

   ImGui::End();
}

void multiHostOrGuest(Game* game, bool* p_open, bool* multiSetup, bool* isHost) {
   centerWindow(game, { 600, 500 });
   if (!ImGui::Begin("Connection Type", p_open, winFlags)) {
      ImGui::End();
      return;
   }

   ImGui::NewLine();
   float width = ImGui::GetWindowContentRegionWidth();

   if (ImGui::Button("Host a Game", ImVec2{ width, 0 })) {
      *multiSetup = true;
      *p_open = false;
      *isHost = true;
   }
   ImGui::NewLine();
   if (ImGui::Button("Join a Game", ImVec2{ width, 0 })) {
      *multiSetup = true;
      *p_open = false;
      *isHost = false;
   }
   ImGui::NewLine();
   if (ImGui::Button("Back", ImVec2{ width, 0 })) {
      *p_open = false;
   }
   ImGui::End();
}

//The popup window that shows a summary of a game after bust
static void _gameResults(Game* game) {
   ImGui::PushFont(game->fonts[20]);
   ImGui::Text("Team %d lost or something...", game->busted + 1);
   ImGui::NewLine();

   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
   float width = ImGui::GetContentRegionAvailWidth();
   ImVec2 wSize = ImGui::GetWindowSize();
   float cursorY = ImGui::GetCursorPosY();
   for (int i = 0; i < game->boards.size(); i++) {
      Board* board = game->boards[i];
      char playerName[20] = "Player";
      if (game->settings.mode == multi_shared) {
         sprintf(playerName, "Team %d", board->team + 1);
      }
      else { sprintf(playerName, "Player %d", game->pList[i + 1].number); }
      ImGui::BeginChild(playerName, { width / game->boards.size(), (wSize.y - cursorY) * 0.9f });
      ImGui::Text("Team: %d", board->team + 1);
      ImGui::Text(game->pList[i + 1].name);
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

   ImGui::PopFont();
   if (ImGui::Button("Accept Defeat")) {
      gameEndMatch(game);
      ImGui::CloseCurrentPopup();
      popupDisable(Popup_GameOver);
   }
   
   if (game->settings.replaying == false) {  //Save the replay
      ImGui::SameLine();
      if (ImGui::Button("Save Replay")) {
         char* path = fileSaveUI();
         if (path != " ") {
            createReplay(game, path);
         }
         if (path != nullptr) { delete path; }
      }
   }
}

//Draw the window and child regions for the board texture to be rendered in
void boardUI(Game* game) {
   if (game->playing == true) {
      ImGui::SetNextWindowSize({ game->windowWidth, game->windowHeight }, ImGuiCond_Once);
      ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Once);
      ImGui::PushFont(game->fonts[20]);
      if (!ImGui::Begin("Drop and Swap", (bool*)0, winFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav) ) {
         ImGui::PopFont();
         ImGui::End();
         return;
      }

      if (game->settings.replaying == true) { replayUI(game, (bool*)0); }

      ImGuiStyle style = ImGui::GetStyle();
      ImGui::TextColored(ImVec4(0.1f, 0.9f, 0.1f, 1.0f), "Game Time: %d s", game->timer / 1000);
      for (int i = 0; i < game->boards.size(); i++) {
         Board* board = game->boards[i];
         char playerInfo[30] = "Player Info";
         sprintf(playerInfo, "Player Info %d", i + 1);
         //ImGui::PushStyleColor(ImGuiCol_ChildBg, (ImVec4)ImColor::ImColor({ 200, 255, 0, 255 }));
         ImGui::BeginChild(playerInfo, ImVec2{ (float)board->tileWidth * (board->w) + (style.WindowPadding.x * 2), 0 }, true, 0);

         //Board Header
         Texture* star = resourcesGetTexture(game->resources, Texture_star);
         Texture* heart = resourcesGetTexture(game->resources, Texture_heart);
         Texture* silver = resourcesGetTexture(game->resources, Texture_silver);
         Texture* dtriangle = resourcesGetTexture(game->resources, Texture_dtriangle);
         Texture* garbage = resourcesGetTexture(game->resources, Texture_g);
         if (game->settings.mode == multi_shared) {  
            ImGui::Text("Team %d", board->team + 1);
            for (auto&& cursor : board->cursors) {
               if (cursor->index == game->user.number) {
                  ImGui::Image((void*)(intptr_t)star->handle, { 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
                  ImGui::SameLine();
               }
               ImGui::Text(game->pList[cursor->index].name);
            }
         }
         if (game->settings.mode == multi_solo) { 
            if (i == game->user.number - 1) {  //todo add player icon
               ImGui::Image((void*)(intptr_t)star->handle, { 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
               ImGui::SameLine();
            }
            else if (game->pList[game->user.number].team == board->team) {
               ImGui::Image((void*)(intptr_t)heart->handle, { 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
               ImGui::SameLine();
            }
            ImGui::Text("Team %d", board->team + 1);
            ImGui::Text(game->pList[i + 1].name); 
         }
         if (game->settings.mode == single_player) { 
            if (i == game->pList[game->user.number].team - 1) {  //todo add player icon
               ImGui::Image((void*)(intptr_t)star->handle, { 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
               ImGui::SameLine();
            }
            ImGui::Text(game->user.name); 
         }
         //Board status row
         ImGui::PushFont(game->fonts[18]);
         ImGui::Image((void*)(intptr_t)silver->handle, { 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });  //todo pause icon
         ImGui::SameLine();
         ImGui::Text("%0.1f s", board->pauseLength / 1000.0);
         ImGui::SameLine();
         int pieces = 0;
         int tiles = 0;
         for (auto&& garbage : board->pile->garbage) {
            if (garbage.second->deployed == false) {
               pieces++;
               tiles += garbage.second->width * garbage.second->layers;
            }
         }
         ImGui::Image((void*)(intptr_t)garbage->handle, { 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });  //todo incoming garbage icon
         ImGui::SameLine();
         ImGui::Text("%d X %d", pieces, tiles);
         //todo This is for adding an icon for being targetted, but since it happens after the fact, it's not super handy
         //if (game->settings.mode == multi_solo) {
         //   for (int i = 0; i < game->boards.size(); i++) {
         //      if (game->boards[i]->target == board->index) {
         //         ImGui::SameLine();
         //         ImGui::Image((void*)(intptr_t)dtriangle->handle, { 16, 16 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });  //todo garbage target icon
         //      }
         //   }
         //}
         ImGui::PopFont();
         //ImGui::PopStyleColor();

         //Draw the board
         char playerName[30] = "Player";
         sprintf(playerName, "Player %d", i + 1);
         ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
         ImGui::BeginChild(playerName, ImVec2{ (float)board->w * board->tileWidth, (float)board->h * board->tileHeight }, true, ImGuiWindowFlags_NoScrollbar);
         ImVec2 csPos = ImGui::GetCursorScreenPos();
         board->sPos.x = csPos.x;
         board->sPos.y = csPos.y;

         //Used to display a texture in ImGui... we do ImVec2{ 0, 1 }, ImVec2{ 1, 0 } because it uses a different coordinate
         if (game->fbos[i]) {
            ImGui::Image((void*)(intptr_t)game->fbos[i]->texture, { game->fbos[i]->w, game->fbos[i]->h }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
         }
         ImDrawList* dList = ImGui::GetWindowDrawList();
         ImVec2 pad = ImGui::GetStyle().WindowPadding;
         if (board->visualEvents[visual_clear].active == true) {  //Draw the visual events over the board texture
            VisualEvent e = board->visualEvents[visual_clear];
            if (board->chain > 1 || board->boardStats.lastCombo > 3) {
               char clearText[10];
               if (board->chain > 1) { sprintf(clearText, "%d Chain", board->chain); }
               else if (board->boardStats.lastCombo > 3) { sprintf(clearText, "%d Combo", board->boardStats.lastCombo); }
               ImVec2 textSize = ImGui::CalcTextSize(clearText);
               dList->AddRectFilled({ e.pos.x + csPos.x - (pad.x * 2), e.pos.y - (pad.y * 2) }, { e.pos.x + csPos.x + textSize.x, e.pos.y + textSize.y }, IM_COL32(0, 0, 0, 200));
               dList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), { e.pos.x + csPos.x, e.pos.y }, IM_COL32_WHITE, clearText, NULL);
            }
         }
         if (board->bust == true) {  //Draw game over message over the board
            char gameOverText[10] = "Game Over";
            ImVec2 tSize = ImGui::CalcTextSize(gameOverText);
            ImVec2 msgLoc = { csPos.x - (pad.x * 2) - tSize.x / 2 + board->w * board->tileWidth / 2, csPos.y - (pad.y * 2) - tSize.y / 2 + board->h * board->tileHeight / 2 };
            dList->AddRectFilled({ msgLoc.x - 20, msgLoc.y - 20 }, { (msgLoc.x + 20) + tSize.x, msgLoc.y + tSize.y + 20 }, IM_COL32(0, 0, 0, 255));
            dList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), msgLoc, IM_COL32_WHITE, gameOverText, NULL);
         }
         if (game->waiting && game->timer < game->timings.countIn[0]) {  //Count in message
            static bool countSound[3] = { false, false, false };
            char countDownText[10];
            ImGui::PushFont(game->fonts[72]);
            sprintf(countDownText, "%d", game->waitLength / 1000 + 1);
            ImVec2 tSize = ImGui::CalcTextSize(countDownText);
            ImVec2 msgLoc = { csPos.x - (pad.x * 2) - tSize.x / 2 + board->w * board->tileWidth / 2, csPos.y - (pad.y * 2) - tSize.y / 2 + board->h * board->tileHeight / 2 };
            dList->AddRectFilled({ msgLoc.x - 20, msgLoc.y - 20 }, { (msgLoc.x + 20) + tSize.x, msgLoc.y + tSize.y + 20 }, IM_COL32(255, 255, 255, 255), 3.0);
            dList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), msgLoc, IM_COL32(255, 0, 0, 255), countDownText, NULL);
            ImGui::PopFont();
            for (int i = 0; i < 3; i++) {  //Play a sound as we count down
               if (game->waitLength < (i + 1) * 1000 && countSound[i] == false) {
                  countSound[i] = true;
                  game->soundToggles[sound_clear] = true;
               }
            }
         }
         ImGui::EndChild();

         //More Board Stats
         char boardStats[30] = "Board Info";
         sprintf(boardStats, "Player Info %d", i + 1);
         ImGui::BeginChild(boardStats, ImVec2{ (float)board->tileWidth * (board->w) + (style.WindowPadding.x * 2), 0 }, false, 0);
         //ImGui::Text("Frame: %d", game->frameCount);

         ImGui::EndChild();
         ImGui::PopStyleVar();
         ImGui::EndChild();
         if (i + 1 != game->boards.size()) { ImGui::SameLine(); }
      }

      if (game->players == 1) {  //One Player Options
         ImGui::SameLine();
         ImGui::BeginChild("One Player Options");
         onePlayerOptions(game);
         ImGui::EndChild();
      }

      //Game over popup
      if (game->busted != -1 && popupStatus(Popup_GameOver) == false) {
         popupEnable(Popup_GameOver);
      }
      if (popupOpen(Popup_GameOver) == true) { 
         ImGui::SetNextWindowSize({ 600, 500 }, ImGuiCond_Once);
         ImGui::OpenPopup("Game Over"); 
         popups[Popup_GameOver].isOpen = true;
      }
      if (ImGui::BeginPopupModal("Game Over") ) {
         if (popups[Popup_GameOver].isOpen == false) { ImGui::CloseCurrentPopup(); }
         _gameResults(game);
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
            for (int i = 0; i < game->net->participants; i++) {
               if (game->net->hostSetup[i].state == Disconnecting) {
                  int leftover = (game->net->disconnectTime[0] - (currentTime - game->net->hostSetup[i].dcStart) / 1000) / 1000;
                  ImGui::Text(game->net->hostSetup[i].name); ImGui::SameLine();
                  ImGui::Text("Time to Reconnect: %d", leftover);
               }
               if (game->net->hostSetup[i].state == Disconnected) {
                  ImGui::Text(game->net->hostSetup[i].name); ImGui::SameLine();
                  ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected");
               }
            }
            ImGui::NewLine();
            if (ImGui::Button("Bail Out")) {
               gameEndMatch(game);
               ImGui::CloseCurrentPopup();
               popupDisable(Popup_Disconnect);
            }
         }
         ImGui::EndPopup();
      }

      if (popupOpen(Popup_Quit) == true) {
         //ImGui::SetNextWindowSize({ 200, 200 }, ImGuiCond_Once);
         ImGui::OpenPopup("Quit Game");
         popups[Popup_Quit].isOpen = true;
      }
      if (ImGui::BeginPopupModal("Quit Game")) {
         if (popups[Popup_Quit].isOpen == false) { ImGui::CloseCurrentPopup(); }
         if (ImGui::Button("Quit")) {
            gameEndMatch(game);
            ImGui::CloseCurrentPopup();
            popupDisable(Popup_Quit);
         }
         ImGui::SameLine();
         if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            popupDisable(Popup_Quit);
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

      ImGui::PopFont();
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
      ImGui::Button("Cursor Mode", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("F");
   }

   else if (controls == 1) {
      ImGui::Button("Movement", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("D Pad");
      ImGui::Button("Swap", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("A");
      ImGui::Button("Pause", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("Start");
      ImGui::Button("Nudge", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("RB Trigger");
      ImGui::Button("Cursor Mode", ImVec2{ width / ratio, 0 }); ImGui::SameLine(); ImGui::Text("X");
   }
   ImGui::PopStyleColor();
}

void gameSettingsUI(Game* game, bool* p_open) {
   centerWindow(game, { 800, 800 });
   if (!ImGui::Begin("Game Settings", p_open, winFlags) ) {
      ImGui::End();
      return;
   }

   if (ImGui::CollapsingHeader("Display and Sound")) {
      static int backgroundMusic = 0;
      //ImGui::Combo("Background Music", &backgroundMusic, "On\0Off\0");

      ImGui::Combo("Sound Effects", &game->sounds, "On\0Off\0");

      static int tileSize = 1;
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
         ImGui::Checkbox("Network log file", &game->net->netlog);
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

   //todo fix statesave
   //if (ImGui::Button("Load Game State")) { gameLoadState(game, "saves/game_state.dat"); }
   //if (ImGui::Button("Save Game State")) { gameSaveState(game, "saves/game_state.dat"); }

   if (game->settings.replaying == false) {
      if (game->debug == true || game->debug == false) {  //todo turn this off later when I make a proper 1 player
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
                  float maxBoardSpeed = 1.0;
                  ImGui::SliderScalar("Board Speed", ImGuiDataType_Float, &board->moveSpeed, &minBoardSpeed, &maxBoardSpeed);

                  float minBoardLevel = 1.0;
                  float maxBoardLevel = 10.0;
                  ImGui::SliderScalar("Board Level", ImGuiDataType_Float, &board->level, &minBoardLevel, &maxBoardLevel);
               }
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
   static int mode = 0;
   ImGui::Combo("Game Mode", &mode, "Individual Boards\0Shared Board\0");
   game->settings.mode = (GameMode)mode;

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
         game->players = pNum - 1;
         game->net->participants = participants;
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
      if (game->net && getMyConnState() != Running) {
         ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Connecting...");
         ImGui::NewLine();
      }
      else if (game->net && getMyConnState() == Running) {
         ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Ready");
         ImGui::NewLine();
      }
   }

   int ready = true;
   for (int i = 0; i < participants; i++) {
      if (game->net->hostSetup[i].state == Running) {
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

   if (game->net && getMyConnState() == Running) {
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
      if (game->user.number == i + 1) { continue; }
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

void multiplayerJoin(Game* game, bool* p_open) {
   centerWindow(game, { 1000, 800 });
   if (!ImGui::Begin("Join a Game", p_open, winFlags)) {
      ImGui::End();
      return;
   }

   if (game->debug == true) {
      static bool showDebugConn = false;
      if (ImGui::Button("Show Conn State")) {
         showDebugConn = true;
      }
      if (showDebugConn == true) { debugConnections(game, &showDebugConn); }

      static bool showMultiSetup = false;
      if (ImGui::Button("Show Game Setup")) {
         showMultiSetup = true;
      }
      if (showMultiSetup == true) { debugMultiplayerSetup(game, &showMultiSetup); }
   }

   static char ipAddress[20] = "127.0.0.1";

   if (clientStatus == client_none) {
      ImGui::InputText("Host IP", ipAddress, IM_ARRAYSIZE(ipAddress));
      ImGui::Checkbox("Use UPNP", &game->net->upnp);
      ImGui::SameLine(); HelpMarker("Universal Plug and Play must be used if you aren't on the same internal network.");
      ImGui::NewLine();
      ImGui::InputText("Your Name", game->user.name, IM_ARRAYSIZE(game->user.name));
      int minBoardLevel = 1;
      int maxBoardLevel = 10;
      ImGui::SliderScalar("Board Speed", ImGuiDataType_U32, &game->user.level, &minBoardLevel, &maxBoardLevel);
      ImGui::Checkbox("I AM A ROBOT", &game->ai);
      ImGui::NewLine();
   }

   float width = ImGui::GetWindowContentRegionWidth();
   if (clientStatus != client_none) {
      ImGui::Text(game->user.name);
      ImGui::NewLine();
      ImGui::BeginChild("Connection Status", { width, 200 }, true);
      ImGui::Text("Messages");
      ImGui::Separator();
      if (game->net && getMyConnState() == Running) {
         game->net->messages.clear();
         *p_open = false;
      }
      else {
         for (auto&& message : game->net->messages) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), message.c_str());
         }
      }

      if (clientStatus >= client_connecting) {  //If we created a socket then pop up a modal with info/status
         popupEnable(Popup_GameSetup);
      }
      else { popupDisable(Popup_GameSetup); }
      ImGui::EndChild();
      ImGui::NewLine();
   }

   if (popupOpen(Popup_GameSetup) == true) {
      //ImGui::SetNextWindowSize({ 700, 700 }, ImGuiCond_Once);
      ImGui::OpenPopup("Connection Status");
      popups[Popup_GameSetup].isOpen = true;
   }
   if (ImGui::BeginPopupModal("Connection Status", (bool*)0, ImGuiWindowFlags_AlwaysAutoResize)) {
      if (popups[Popup_GameSetup].isOpen == false) { ImGui::CloseCurrentPopup(); }
      if (game->net->messages.size() > 0) { ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), game->net->messages.back().c_str()); }
      ImGui::NewLine();

      //This displays the game information once it is received
      if (clientStatus >= client_received) {
         ImGui::BeginChild("Game Setup", { width, 200 }, true);
         ImGui::Text("Game Setup");
         ImGui::Separator();
         for (int i = 0; i < game->net->participants; i++) {
            ImGui::BeginChild((char*)game->net->hostSetup[i].id, { width / game->net->participants, 100 });
            ImGui::Text(game->net->hostSetup[i].name);
            ImGui::Text("Player: %d", game->net->hostSetup[i].pNum);
            ImGui::Text("Team: %d", game->net->hostSetup[i].team + 1);
            ImGui::Text("Board Speed: %d", game->net->hostSetup[i].level);
            ImGui::EndChild();
            if (i + 1 != game->net->participants) { ImGui::SameLine(); }
         }
         ImGui::EndChild();
         ImGui::NewLine();

         if (ImGui::Button("Start Game")) {
            for (int i = 0; i < game->net->participants; i++) {
               if (i == 0) {
                  strcpy(game->net->hostSetup[i].ipAddress, ipAddress);  //Use the IP we used to connect to the host
               }
            }
            clientStatus = client_loaded;
            ggpoCreateSession(game, game->net->hostSetup, game->net->participants);
         }
         ImGui::SameLine();
      }

      if (ImGui::Button("Cancel")) {
         ImGui::CloseCurrentPopup();
         popupDisable(Popup_GameSetup);
         tcpReset();
         clientStatus = client_none; 
         //todo add a send message to host, so we know we lost a player
      }
      ImGui::EndPopup();
   }

   if (game->winsockRunning == true) {
      if (clientStatus == client_done) {
         tcpCleanup();
         clientStatus = client_none;
      }
   }

   if (clientRunning == false) {
      if (ImGui::Button("Connect to Host")) {
         clientStatus = client_started;
         //This is the client loop thread
         clientThread = std::thread(tcpClientLoop, 7000, ipAddress, std::ref(clientStatus), game->user.name, std::ref(clientRunning));
         clientThread.detach();
      }
      ImGui::SameLine();
   }

   if (ImGui::Button("Reset Connection")) {  //This just nukes everything
      tcpReset();
      clientStatus = client_none;
   }

   //If the GGPO session is started launch the connecting modal window
   if (game->playing == false && game->net->ggpo) {
      popupEnable(Popup_Connecting);
      popupDisable(Popup_GameSetup);
   }
   ggpoReadyModal(game);

   ImGui::End();
}

void multiplayerHost(Game* game, bool* p_open) {
   centerWindow(game, { 1000, 800 });
   if (!ImGui::Begin("Host a Game", p_open, winFlags)) {
      ImGui::End();
      return;
   }

   if (game->debug == true) {
      static bool showDebugConn = false;
      if (ImGui::Button("Show Conn State")) {
         showDebugConn = true;
      }
      if (showDebugConn == true) { debugConnections(game, &showDebugConn); }
      static bool showMultiSetup = false;
      if (ImGui::Button("Show Game Setup")) {
         showMultiSetup = true;
      }
      if (showMultiSetup == true) { debugMultiplayerSetup(game, &showMultiSetup); }
   }

   static char ipAddress[20] = "127.0.0.1";
   static int people[3] = { 2, 2, GAME_MAX_PLAYERS };

   if (serverStatus == server_none) {
      ImGui::SliderScalar("Total Players", ImGuiDataType_U32, &people[0], &people[1], &people[2]);
      ImGui::Checkbox("Use UPNP", &game->net->upnp);
      ImGui::SameLine(); HelpMarker("Universal Plug and Play must be used if you aren't on the same internal network.");
      ImGui::NewLine();
      ImGui::InputText("Your Name", game->user.name, IM_ARRAYSIZE(game->user.name));
      int minBoardLevel = 1;
      int maxBoardLevel = 10;
      ImGui::SliderScalar("Board Speed", ImGuiDataType_U32, &game->user.level, &minBoardLevel, &maxBoardLevel);
      ImGui::Checkbox("I AM A ROBOT", &game->ai);
      ImGui::NewLine();
   }

   if (serverStatus != server_none) {
      float width = ImGui::GetWindowContentRegionWidth();
      ImGui::Text(game->user.name);
      ImGui::NewLine();
      ImGui::BeginChild("Connection Status", { width, 200 }, true);
      ImGui::Text("Messages");
      ImGui::Separator();
      if (game->net && getMyConnState() == Running) {
         game->net->messages.clear();
         *p_open = false;
      }
      else {
         for (auto&& message : game->net->messages) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), message.c_str());
         }
      }
      ImGui::EndChild();
      ImGui::NewLine();
   }

   if (serverRunning == false) {
      if (ImGui::Button("Find Players")) {
         serverStatus = server_started;
         //This is the thread for the server loop
         serverThread = std::thread(tcpServerLoop, 7000, people[0] - 1, std::ref(serverStatus), std::ref(serverRunning));
         serverThread.detach();
      }
      ImGui::SameLine();
   }

   if (ImGui::Button("Reset Connection")) {
      tcpReset();
      serverStatus = server_none;
   }

   if (serverStatus > server_started) {  //Once we start listening on our socket pop up a modal with status/info
      popupEnable(Popup_GameSetup);
   }
   else { popupDisable(Popup_GameSetup); }

   if (popupOpen(Popup_GameSetup) == true) {
      //ImGui::SetNextWindowSize({ 700, 700 }, ImGuiCond_Once);
      ImGui::OpenPopup("Connection Status");
      popups[Popup_GameSetup].isOpen = true;
   }
   if (ImGui::BeginPopupModal("Connection Status", (bool*)0, ImGuiWindowFlags_AlwaysAutoResize)) {
      if (popups[Popup_GameSetup].isOpen == false) { ImGui::CloseCurrentPopup(); }
      if (game->net->messages.size() > 0) { ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), game->net->messages.back().c_str()); }
      ImGui::NewLine();

      if (serverStatus == server_waiting) {
         //This is the player information table
         ImGui::Text("Game Setup");
         ImGui::Separator();
         ImGui::PushID("Player Info Set");
         ImGui::PushItemWidth(100);
         for (int i = 0; i < people[0]; i++) {
            ImGui::PushID(i);  //So widgets don't name collide
            SocketInfo sock = getSocket(i - 1);
            if (i == 0) { ImGui::Text(game->user.name); }
            else { ImGui::Text(sock.name); }
            ImGui::SameLine();
            ImGui::SetCursorPosX(224);
            ImGui::Combo("Team", &game->net->hostSetup[i].team, "One\0Two\0");
            ImGui::SameLine();
            ImGui::Combo("Player Type", &game->net->hostSetup[i].playerType, "Player\0Spectator\0");
            //ImGui::SameLine();
            //ImGui::Text(inet_ntoa(sock.address.sin_addr));
            ImGui::PopID();
         }
         ImGui::PopID();
         ImGui::PopItemWidth();

         static int mode = 0;
         ImGui::NewLine();
         if (ImGui::CollapsingHeader("Board Setup"), ImGuiTreeNodeFlags_DefaultOpen) {
            ImGui::Combo("Board Type", &mode, "Individual\0Shared\0");
            ImGui::InputInt("Board Width", &game->settings.bWidth);
            ImGui::InputInt("Board Height", &game->settings.bHeight);
            ImGui::NewLine();
         }

         if (ImGui::CollapsingHeader("GGPO Options")) {
            ImGui::SliderScalar("Frame Delay", ImGuiDataType_U32, &game->net->frameDelay[0], &game->net->frameDelay[1], &game->net->frameDelay[2]);
            ImGui::SliderScalar("Disconnect Wait", ImGuiDataType_U32, &game->net->disconnectTime[0], &game->net->disconnectTime[1], &game->net->disconnectTime[2]);
            ImGui::NewLine();
         }
         if (game->settings.mode != (GameMode)mode) { game->settings.mode = (GameMode)mode; }  //todo this seems a bit awkward
         ImGui::NewLine();

         static bool teams[2] = { false, false };
         static int pCount = -1;
         if (ImGui::Button("Start Game")) {
            validateMultiSetup(people[0], pCount, teams, serverStatus);
         }
         ImGui::SameLine();

         if (popupOpen(Popup_Error) == true) {
            ImGui::OpenPopup("Setup Error");
            popups[Popup_Error].isOpen = true;
         }
         if (ImGui::BeginPopupModal("Setup Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (popups[Popup_Error].isOpen == false) { ImGui::CloseCurrentPopup(); }
            else {
               if (pCount < 2) { ImGui::Text("You must have at least 2 players."); }
               else if (pCount > 4) { ImGui::Text("You can't have more than 4 players."); }
               else if (teams[0] == false) { ImGui::Text("You need at least one player on team 1"); }
               else if (teams[1] == false) { ImGui::Text("You need at least one player on team 2"); }
               ImGui::NewLine();
               if (ImGui::Button("OK")) {
                  ImGui::CloseCurrentPopup();
                  popupDisable(Popup_Error);
               }
               ImGui::EndPopup();
            }
         }
      }

      if (ImGui::Button("Cancel")) {
         ImGui::CloseCurrentPopup();
         popupDisable(Popup_GameSetup);
         tcpReset();
         serverStatus = server_none; 
      }
      ImGui::EndPopup();
   }

   if (game->winsockRunning == true) {
      if (serverStatus == server_done) {
         tcpCleanup();
         serverStatus = server_none;
         ggpoCreateSession(game, game->net->hostSetup, game->net->participants);
      }
   }


   //If the GGPO session is started launch the connecting modal window
   if (game->playing == false && game->net->ggpo) {
      popupEnable(Popup_Connecting);
      popupDisable(Popup_GameSetup);
   }
   ggpoReadyModal(game);
   ImGui::End();
}

void ggpoReadyModal(Game* game) {
   static float framesBeforeReady = 300.0;
   if (popupOpen(Popup_Connecting) == true) {
      ImGui::SetNextWindowSize({ 800, 800 }, ImGuiCond_Once);
      ImGui::OpenPopup("Synchronizing Players");
      popups[Popup_Connecting].isOpen = true;
   }
   if (ImGui::BeginPopupModal("Synchronizing Players")) {
      if (popups[Popup_Connecting].isOpen == false) { ImGui::CloseCurrentPopup(); }
      static int readyCount = 0;
      int ready = true;

      if (game->net->ggpo != nullptr) {
         ImGui::Text("Player Status");
         ImGui::Separator();
         float width = ImGui::GetWindowContentRegionWidth();
         for (int i = 0; i < game->net->participants; i++) {
            ImGui::BeginChild((char*)game->net->hostSetup[i].id, { width / game->net->participants, 200 });
            ImGui::Text(game->net->hostSetup[i].name);
            ImGui::Text("Player: %d", game->net->hostSetup[i].pNum);
            ImGui::Text("Team: %d", game->net->hostSetup[i].team + 1);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ggpoShowStatus(game, i));
            if (game->net->hostSetup[i].state != Running) {
               readyCount = 0;
               ready = false;
            }
            ImGui::EndChild();
            if (i + 1 != game->net->participants) { ImGui::SameLine(); }
         }
      }
      if (ready == true) { readyCount++; }
      if (readyCount > framesBeforeReady) {
         popupDisable(Popup_Connecting);
         ImGui::CloseCurrentPopup();
         gameStartMatch(game); 

      }
      if (readyCount > 0) {
         ImGui::Text("Preparing Session: "); 
         ImGui::SameLine();
         ImGui::ProgressBar(readyCount / framesBeforeReady, ImVec2(0.0f, 0.0f));
      }
      ImGui::NewLine();
      if (ImGui::Button("Disconnect")) {
         ImGui::CloseCurrentPopup();
         popupDisable(Popup_Connecting);
         ggpoEndSession(game);
      }
      ImGui::EndPopup();
   }
}

void debugConnections(Game* game, bool* p_open) {
   if (!ImGui::Begin("Conn State", p_open)) {
      ImGui::End();
      return;
   }

   ImGui::Text("Winsock running: %d", game->winsockRunning);
   ImGui::Text("UPNP running: %d", game->upnpRunning);
   ImGui::Text("Server thread running: %d", serverRunning);
   ImGui::Text("Server status: %d", serverStatus);
   ImGui::Text("Client thread running: %d", clientRunning);
   ImGui::Text("Client status: %d", clientStatus);

   _connectionInfo();

   ImGui::End();
}

void debugMultiplayerSetup(Game* game, bool* p_open) {
   if (!ImGui::Begin("Conn State", p_open)) {
      ImGui::End();
      return;
   }

   ImGui::Text("Participants: %d", game->net->participants);
   ImGui::Text("Seed: %d", game->seed);

   for (int i = 0; i < game->net->participants; i++) {
      if (ImGui::CollapsingHeader(game->net->hostSetup[i].name)) {
         ImGui::Text("Me: %d", game->net->hostSetup[i].me);
         ImGui::Text("Local Port: %d", game->net->hostSetup[i].localPort);
         ImGui::Text(game->net->hostSetup[i].ipAddress);
         ImGui::Text(game->net->hostSetup[i].name);
         ImGui::Text("Host: %d", game->net->hostSetup[i].host);
         ImGui::Text("ID: %d", game->net->hostSetup[i].id);
         ImGui::Text("Player Number: %d", game->net->hostSetup[i].pNum);
         ImGui::Text("Team: %d", game->net->hostSetup[i].team);
         ImGui::Text("Level: %d", game->net->hostSetup[i].level);
      }
   }

   //Add GGPO state

   ImGui::End();
}

void replayUI(Game* game, bool* p_open) {
   if (!ImGui::Begin("Replay Options", p_open)) {
      ImGui::End();
      return;
   }

   static int frameRange[3] = { 0, 0, 0 };
   static int frameRate[3] = { 0, 1, 16 };
   static bool replayLoaded = false;
   if (ImGui::Button("Load Replay")) {
      char* path = fileOpenUI();

      if (strcmp(path, " ") != 0) {
         if (game->playing == true) { gameEndMatch(game); }
         std::vector <Byte> stream = streamLoadFromFile(path); 
         loadReplay(game, stream);  //todo we should validate the file and bail before/during load
         game->settings.replaying = true;
         gameStartMatch(game);

         frameRange[2] = game->settings.repInputs.size() - 1;
         replayLoaded = true;
      }
      if (path != nullptr) { delete path; }
   }

   if (replayLoaded == true) {
      ImGui::SliderScalar("Speed X", ImGuiDataType_U32, &game->settings.replaySpeed, &frameRate[1], &frameRate[2]);

      ImGui::SliderScalar("Frame", ImGuiDataType_U32, &frameRange[0], &frameRange[1], &frameRange[2]);
      if (ImGui::IsItemDeactivatedAfterEdit() == true) {
         if (frameRange[0] < game->frameCount) {
            if (game->playing == true) { gameEndMatch(game); }
            std::vector <Byte> stream = streamLoadFromFile("saves/replay.rep");
            loadReplay(game, stream);
            game->settings.replaying = true;
            gameStartMatch(game);
         }
         while (game->frameCount < frameRange[0]) {
            gameReplay(game);
         }
      }
      else if (ImGui::IsItemActive() == false) {
         frameRange[0] = game->frameCount;
      }
   }

   ImGui::End();
}