#include "game_ui.h"

#include "board.h"
#include "serialize.h"
#include "netplay.h"
#include "resources.h"
#include "sounds.h"

#include "imgui/imgui.h"
#include "win.h"

#include <thread>

void singlePlayerGame(Game* game, bool* p_open);
void singleVersusUI(Game* game, bool* p_open);
void multiHostOrGuest(Game* game, bool* p_open, bool* multiSetup, bool* isHost);
void multiplayerJoin(Game* game, bool* p_open);
void multiplayerHost(Game* game, bool* p_open);
void ggpoReadyModal(Game* game);
void loadReplayFromFile(Game* game);
void replayUI(Game* game);
void aboutUI(Game* game, bool* p_open);
void gameInfoUI(Game* game, bool* p_open);
void licensesUI(Game* game, bool* p_open);
void creditsUI(Game* game, bool* p_open);

void gameSetupError(int pCount, bool teams[2]);

void debugConnections(Game* game, bool* p_open);
void debugMultiplayerSetup(Game* game, bool* p_open);

//Globals used by TCP transfer threads
ServerStatus serverStatus = server_none;     //What stage of the game info transfer is the server in
ClientStatus clientStatus = client_none;     //What stage of the game info transfer is the client in
std::thread serverThread;                    //Is the server thread handle
std::thread clientThread;                    //Is the client thread handle
bool clientRunning = false;                  //Is the client thread running
bool serverRunning = false;                  //Is the server thread running

std::vector <std::string> credits = {
   "This game is dedicated to the Eclipse Mining Technologies team.",
   "I hope that we'll be able to Drop and Swap in person again soon.",
   "A special thanks goes out to:",
   "My wife, Stephanie Anderson, and our boys. Enough said, lol.",
   "Brandon Townsend for teaching me everything I needed to start.",
   "I finally feel like we could do a game jam, lol.",
   "Sean Hunter for always being there with the answers.",
   "I'm excited for the final version of Super Puzzled Cat.",
   "And all the people who helped me test Drop and Swap.",
   "It was much more painful than I ever imagined.",
   "   Tyler Fowler",
   "   Russ Bohnhoff",
   "   Kyle McDonald",
   "   Paul Castleberry",
};

const char* aboutText = R"(
Written by Kieran McDonald 
08/23/2020

Drop and Swap was born out of love for Panel De Pon 
and Puyo Puyo. It started out as a joke, but it turned 
into a fun and challenging project. I'd always wanted to 
make a video game since I started playing and hacking 
them on the Commodore 64. I'm glad I finally got the
chance. 

Hope you enjoy it :)
Kieran

)";

ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

int frameRange[3] = { 0, 0, 0 };
int frameRate[3] = { 0, 1, 16 };
bool replayLoaded = false;

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

//Used to display a texture in ImGui... we do ImVec2{ 0, 1 }, ImVec2{ 1, 0 } because it uses a different coordinate system
void imguiDrawTexture(Game* game, TextureEnum textureType, ImVec2 size) {
   Texture* texture = resourcesGetTexture(game->resources, textureType);
   ImGui::Image((void*)(intptr_t)texture->handle, size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
}

//Overload for the above function taking GLuint handle for a Texture
void imguiDrawTexture(Game* game, unsigned int handle, ImVec2 size) {
   ImGui::Image((void*)(intptr_t)handle, size, ImVec2 { 0, 1 }, ImVec2{ 1, 0 });
}

void errorLoadingReplay() {
   if (popupOpen(Popup_LoadFailed) == true) {
      ImGui::SetNextWindowSize({ 200, 200 }, ImGuiCond_Once);
      ImGui::OpenPopup("Failed to Load File");
      popups[Popup_LoadFailed].isOpen = true;
   }
   if (ImGui::BeginPopupModal("Failed to Load File")) {
      if (popups[Popup_LoadFailed].isOpen == false) {
         ImGui::CloseCurrentPopup();
         popupDisable(Popup_LoadFailed);
      }

      if (ImGui::Button("Close")) {
         ImGui::CloseCurrentPopup();
         popupDisable(Popup_LoadFailed);
      }

      ImGui::EndPopup();
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

//Main menu UI
void mainUI(Game* game) {
   centerWindow(game, { 400, 340 });
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

   ImGui::NewLine();
   static bool aboutWindow = false;
   if (ImGui::Button("About", ImVec2{ width, 0 })) {
      aboutWindow = true;
   }
   if (aboutWindow) {
      aboutUI(game, &aboutWindow);
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
   if (ImGui::Button("Practice Solo", ImVec2{ width, 0 })) {
      game->players = 1;
      game->settings.mode = single_player;
      gameStartMatch(game);
   }

   static bool singleVersus = false;
   ImGui::NewLine();
   if (ImGui::Button("Versus Computer", ImVec2{ width, 0 })) {
      singleVersus = true;
   }
   if (singleVersus == true) { singleVersusUI(game, &singleVersus); }

   ImGui::NewLine();
   if (ImGui::Button("Watch Replay", ImVec2{ width, 0 })) {
      loadReplayFromFile(game);
   }
   errorLoadingReplay();

   ImGui::NewLine();
   ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.6f, 0.6f));
   if (ImGui::Button("Back", ImVec2{ width, 0 })) {
      *p_open = false;
   }
   ImGui::PopStyleColor();

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
   ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.6f, 0.6f));

   if (ImGui::Button("Back", ImVec2{ width, 0 })) {
      *p_open = false;
   }   
   ImGui::PopStyleColor();

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

   static bool repSaved = false;
   if (ImGui::Button("Accept Defeat")) {
      gameEndMatch(game);
      ImGui::CloseCurrentPopup();
      popupDisable(Popup_GameOver);
      repSaved = false;
   }
   
   if (game->settings.replaying == false && repSaved == false) {  //Save the replay
      ImGui::SameLine();
      if (ImGui::Button("Save Replay")) {
         char* path = fileSaveUI();
         if (path != " ") {
            createReplay(game, path);
         }
         if (path != nullptr) { delete path; }
         repSaved = true;
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

      if (game->settings.replaying == true) { replayUI(game); }

      ImGuiStyle style = ImGui::GetStyle();
      ImGui::TextColored(ImVec4(0.1f, 0.9f, 0.1f, 1.0f), "Game Time: %d s", (int)game->timer / 1000);
      if (game->debug == true) {
         ImGui::SameLine();
         ImGui::TextColored(ImVec4(0.1f, 0.9f, 0.1f, 1.0f), "Frame: %d", game->frameCount);
      }
      for (int i = 0; i < game->boards.size(); i++) {
         Board* board = game->boards[i];
         char playerInfo[30] = "Player Info";
         sprintf(playerInfo, "Player Info %d", i + 1);
         ImGui::BeginChild(playerInfo, ImVec2{ (float)board->tileWidth * (board->w) + (style.WindowPadding.x * 2), 0 }, true, 0);

         //Board Header
         if (game->settings.mode == multi_shared) {  
            ImGui::Text("Team %d", board->team + 1);
            for (auto&& cursor : board->cursors) {
               if (cursor->index == game->user.number) {
                  imguiDrawTexture(game, Texture_star, { 16, 16 });
                  ImGui::SameLine();
               }
               ImGui::Text(game->pList[cursor->index].name);
            }
         }
         else if (game->settings.mode == multi_solo || game->settings.mode == single_vs) { 
            if (i == game->user.number - 1) {  //todo add player icon
               imguiDrawTexture(game, Texture_star, { 16, 16 });
               ImGui::SameLine();
            }
            else if (game->pList[game->user.number].team == board->team) {
               imguiDrawTexture(game, Texture_heart, { 16, 16 });
               ImGui::SameLine();
            }
            ImGui::Text("Team %d", board->team + 1);
            ImGui::Text(game->pList[i + 1].name); 
         }
         else if (game->settings.mode == single_player) { 
            if (i == game->pList[game->user.number].team - 1) {  //todo add player icon
               imguiDrawTexture(game, Texture_star, { 16, 16 });
               ImGui::SameLine();
            }
            ImGui::Text(game->user.name); 
         }
         //Board status row
         ImGui::PushFont(game->fonts[18]);
         imguiDrawTexture(game, Texture_silver, { 16, 16 });  //todo pause icon
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
         imguiDrawTexture(game, Texture_g, { 16, 16 });  //todo incoming garbage icon
         ImGui::SameLine();
         ImGui::Text("%d X %d", pieces, tiles);
         //todo This is for adding an icon for being targetted, but since it happens after the fact, it's not super handy
         //if (game->settings.mode == multi_solo) {
         //   for (int i = 0; i < game->boards.size(); i++) {
         //      if (game->boards[i]->target == board->index) {
         //         ImGui::SameLine();
         //         imguiDrawTexture(game, Texture_dtriangle, { 16, 16 });  //todo garbage target icon
         //      }
         //   }
         //}
         ImGui::PopFont();

         //Draw the board
         char playerName[30] = "Player";
         sprintf(playerName, "Player %d", i + 1);
         ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
         ImGui::BeginChild(playerName, ImVec2{ (float)board->w * board->tileWidth, (float)board->h * board->tileHeight }, true, ImGuiWindowFlags_NoScrollbar);
         ImVec2 csPos = ImGui::GetCursorScreenPos();
         board->sPos.x = csPos.x;
         board->sPos.y = csPos.y;

         if (game->fbos[i]) {
            imguiDrawTexture(game, game->fbos[i]->texture, { game->fbos[i]->w, game->fbos[i]->h });
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
         if (game->waiting && game->timer == 0) {  //Count in message
            static int lastCount = 0;
            int count = ceil(game->waitLength / 1000.0);
            char countDownText[10];
            ImGui::PushFont(game->fonts[72]);
            sprintf(countDownText, "%d", count);
            ImVec2 tSize = ImGui::CalcTextSize(countDownText);
            ImVec2 msgLoc = { csPos.x - (pad.x * 2) - tSize.x / 2 + board->w * board->tileWidth / 2, csPos.y - (pad.y * 2) - tSize.y / 2 + board->h * board->tileHeight / 2 };
            dList->AddRectFilled({ msgLoc.x - 20, msgLoc.y - 20 }, { (msgLoc.x + 20) + tSize.x, msgLoc.y + tSize.y + 20 }, IM_COL32(255, 255, 255, 255), 3.0);
            dList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), msgLoc, IM_COL32(255, 0, 0, 255), countDownText, NULL);
            ImGui::PopFont();
            if (count != lastCount ) {
               lastCount = count;
               game->soundToggles[sound_clear] = true;
            }
         }
         ImGui::EndChild();

         //More Board Stats
         char boardStats[30] = "Board Info";
         sprintf(boardStats, "Player Info %d", i + 1);
         ImGui::BeginChild(boardStats, ImVec2{ (float)board->tileWidth * (board->w) + (style.WindowPadding.x * 2), 0 }, false, 0);

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
         game->soundToggles[sound_musicbox] = true;  //Play gameover music
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
      static float globalVol[3] = { soundsGetGlobalVolume(), 0.0, 1.0 };
      ImGui::SliderScalar("Master Volume", ImGuiDataType_Float, &globalVol[0], &globalVol[1], &globalVol[2]);
      if (ImGui::IsItemEdited() == true) { soundsSetGlobalVolume(globalVol[0]); }

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

         if (ImGui::CollapsingHeader("Sound Test")) {
            if (ImGui::Button("Stop Sounds")) {
               soundsStopAll();
            }
            for (int i = 0; i < sound_COUNT; i++) {
               ImGui::PushID(i);
               if (ImGui::Button("Sound")) {
                  game->soundToggles[(SoundEffect)i] = true;
               }
               ImGui::PopID();
            }
         }
      }
   }
   ImGui::End();
}

void onePlayerOptions(Game* game) {

   if (game->settings.replaying == false) {
      if (game->debug == true || game->debug == false) {  //todo turn this off later when I make a proper 1 player

         if (ImGui::Button("Save State (F1)")) { game->settings.save = gameSave(game); }
         ImGui::SameLine();
         if (ImGui::Button("Load State (F2)")) {
            if (game->settings.save.size() > 0) {
               unsigned char* start = game->settings.save.data();
               gameCallbackLoad(game, start);
            }
         }
         ImGui::NewLine();

         if (game->ai == true) { ImGui::Text(aiGetMove(1)); }
         ImGui::NewLine();

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

//Todo this isn't used right now
//Get some stats about the GGPO connection
//void ggpoNetStatsUI(Game* game, bool* p_open) {
//
//   if (!ImGui::Begin("Network Stats", p_open)) {
//      ImGui::End();
//      return;
//   }
//
//   GGPONetworkStats stats;
//   for (int i = 0; i < game->players; i++) {
//      if (game->user.number == i + 1) { continue; }
//      ggpo_get_network_stats(game->net->ggpo, i + 1, &stats);
//
//      ImGui::Text("Player %d Connection Info", i + 1);
//      ImGui::Text("%.2f kilobytes/sec sent", stats.network.kbps_sent);
//      ImGui::Text("Send queue length: %d", stats.network.send_queue_len);
//      ImGui::Text("Receive queue length: %d", stats.network.recv_queue_len);
//      ImGui::Text("Ping: %d ", stats.network.ping);
//      ImGui::Text("Local Frames behind: %d", stats.timesync.local_frames_behind);
//      ImGui::Text("Remote frames behind: %d", stats.timesync.remote_frames_behind);
//   }
//
//   ImGui::End();
//}

void multiplayerJoin(Game* game, bool* p_open) {
   centerWindow(game, { 1000, 800 });
   if (!ImGui::Begin("Join a Game", p_open, winFlags)) {
      ImGui::End();
      return;
   }

   helpfulCenterText("Enter the IP address of the host and hit Connect To Host.");
   ImGui::NewLine();

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
      ImGui::EndChild();
      ImGui::NewLine();
   }

   if (clientStatus >= client_connecting && clientStatus < client_received) {  
      popupEnable(Popup_MultiStarted);
   }
   else { popupDisable(Popup_MultiStarted); }

   if (popupOpen(Popup_MultiStarted) == true) {
      ImGui::OpenPopup("Connection Started");
      popups[Popup_MultiStarted].isOpen = true;
   }
   if (ImGui::BeginPopupModal("Connection Started", (bool*)0, ImGuiWindowFlags_AlwaysAutoResize)) {
      if (popups[Popup_MultiStarted].isOpen == false) { ImGui::CloseCurrentPopup(); }
      if (game->net->messages.size() > 0) { ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), game->net->messages.back().c_str()); }
      ImGui::NewLine();

      if (ImGui::Button("Cancel")) {
         ImGui::CloseCurrentPopup();
         popupDisable(Popup_MultiStarted);
         tcpReset();
         clientStatus = client_none;
         //todo add a send message to host, so we know we lost a player
      }
      ImGui::EndPopup();
   }

   if (clientStatus >= client_received && clientStatus != client_done) {  //If we created a socket then pop up a modal with info/status
      popupEnable(Popup_GameSetup);
   }
   else { popupDisable(Popup_GameSetup); }

   if (popupOpen(Popup_GameSetup) == true) {
      ImGui::OpenPopup("Connection Status");
      popups[Popup_GameSetup].isOpen = true;
   }
   if (ImGui::BeginPopupModal("Connection Status", (bool*)0, ImGuiWindowFlags_AlwaysAutoResize)) {
      if (popups[Popup_GameSetup].isOpen == false) { ImGui::CloseCurrentPopup(); }
      if (game->net->messages.size() > 0) { ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), game->net->messages.back().c_str()); }
      ImGui::NewLine();

      ImGui::BeginChild("Game Setup", { width, 300 }, true);
      ImGui::Text("Game Setup");
      ImGui::Separator();
      if (game->settings.mode == multi_solo) { ImGui::Text("Board Type: Individual boards"); }
      else if (game->settings.mode == multi_shared) { ImGui::Text("Board Type: Shared board"); }
      ImGui::Text("Board Size: %dw x %dh", game->settings.bWidth, game->settings.bHeight);
      ImGui::NewLine();
      ImGui::Text("Player Info");
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

   helpfulCenterText("Select the number of players and hit Find Players.");
   helpfulCenterText("Each player will need your IP address to join the game.");
   ImGui::NewLine();

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

   if (serverStatus > server_started && serverStatus < server_waiting) {  //Once we start listening on our socket pop up a modal with status/info
      popupEnable(Popup_MultiStarted);
   }
   else { popupDisable(Popup_MultiStarted); }

   if (popupOpen(Popup_MultiStarted) == true) {
      ImGui::OpenPopup("Connection Started");
      popups[Popup_MultiStarted].isOpen = true;
   }
   if (ImGui::BeginPopupModal("Connection Started", (bool*)0, ImGuiWindowFlags_AlwaysAutoResize)) {
      if (popups[Popup_MultiStarted].isOpen == false) { ImGui::CloseCurrentPopup(); }
      if (game->net->messages.size() > 0) { ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), game->net->messages.back().c_str()); }
      ImGui::NewLine();

      if (ImGui::Button("Cancel")) {
         ImGui::CloseCurrentPopup();
         popupDisable(Popup_MultiStarted);
         tcpReset();
         serverStatus = server_none;
      }
      ImGui::EndPopup();
   }

   if (serverStatus >= server_waiting && serverStatus != server_done) {
      popupEnable(Popup_GameSetup);
   }
   else { popupDisable(Popup_GameSetup); }

   if (popupOpen(Popup_GameSetup) == true) {
      ImGui::OpenPopup("Connection Status");
      popups[Popup_GameSetup].isOpen = true;
   }
   if (ImGui::BeginPopupModal("Connection Status", (bool*)0, ImGuiWindowFlags_AlwaysAutoResize)) {
      if (popups[Popup_GameSetup].isOpen == false) { ImGui::CloseCurrentPopup(); }
      if (game->net->messages.size() > 0) { ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), game->net->messages.back().c_str()); }
      ImGui::NewLine();

      static bool teams[2] = { false, false };
      static int pCount = -1;
      //This is the player information table
      if (serverStatus == server_waiting) {
         ImGui::Text("Game Setup");
         ImGui::Separator();
         static int mode = 0;
         static int bSize = 0;
         if (people[0] >= 2) {
            ImGui::Combo("Board Type", &mode, "Individual\0Shared\0");
            if (ImGui::IsItemEdited) {
               game->settings.mode = (GameMode)mode;
               if (mode == 0) {
                  game->settings.bWidth = 6;
                  game->settings.bHeight = 12;
               }
            }
            if (mode == 1) {
               ImGui::Combo("Board Size", &bSize, "Regular\0Wide\0");
               if (ImGui::IsItemEdited) {
                  game->settings.bHeight = 12;
                  if (mode == 1 && bSize == 0) { game->settings.bWidth = 6; }
                  else if (mode == 1 && bSize == 1) { game->settings.bWidth = 12; }
               }
            }
         }
         ImGui::SliderScalar("Frame Delay", ImGuiDataType_U32, &game->net->frameDelay[0], &game->net->frameDelay[1], &game->net->frameDelay[2]);
         ImGui::SliderScalar("Disconnect Wait", ImGuiDataType_U32, &game->net->disconnectTime[0], &game->net->disconnectTime[1], &game->net->disconnectTime[2]);
         ImGui::NewLine();

         ImGui::PushID("Player Info Set");
         ImGui::PushItemWidth(100);
         ImGui::Text("Player Info");
         ImGui::Separator();
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

         ImGui::NewLine();
         ImGui::Separator();

         if (ImGui::Button("Start Game")) {
            validateMultiSetup(people[0], pCount, teams, serverStatus);
         }
         ImGui::SameLine();
      }

      gameSetupError(pCount, teams);

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

void singleVersusUI(Game* game, bool* p_open) {
   centerWindow(game, { 600, 600 });
   if (!ImGui::Begin("Single Player Versus", p_open, winFlags)) {
      ImGui::End();
      return;
   }

   static int people[3] = { 2, 2, 4 };
   static int levelRanges[3] = { 5, 1, 10 };
   static int teams[4] = { 0, 1, 1, 1 };  //Team setup for each player
   static int levels[4] = { 5, 5, 5, 5 };  //Player handicaps
   ImGui::SliderScalar("Total Players", ImGuiDataType_U32, &people[0], &people[1], &people[2]);

   ImGui::NewLine();
   ImGui::PushItemWidth(150);
   for (int i = 0; i < people[0]; i++) {
      ImGui::PushID(i);
      if (i == 0) { ImGui::Text("You"); }
      else { ImGui::Text("Computer %d", i); }
      ImGui::SameLine();
      ImGui::SetCursorPosX(140);  //Sameline wipes this out if you do it before
      ImGui::Combo("Team", &teams[i], "One\0Two\0");
      ImGui::SameLine();
      ImGui::SliderScalar("Level", ImGuiDataType_U32, &levels[i], &levelRanges[1], &levelRanges[2]);
      ImGui::PopID();
   }
   ImGui::PopItemWidth();

   ImGui::NewLine();
   static bool teamCheck[2] = { false, false };
   if (ImGui::Button("Start")) {
      game->players = people[0];
      game->settings.mode = single_vs;

      teamCheck[0] = teamCheck[1] = false;
      for (int i = 0; i < game->players; i++) {
         teamCheck[teams[i]] = true;
      }

      if (teamCheck[0] == true && teamCheck[1] == true) {
         for (int i = 0; i < game->players; i++) {
            game->pList[i + 1].team = teams[i];
            game->pList[i + 1].level = levels[i];
            game->pList[i + 1].number = i + 1;
            sprintf(game->pList[i + 1].name, "Player %d", i + 1);
         }
         gameStartMatch(game);
      }
      else { popupEnable(Popup_Error); }
   }

   gameSetupError(people[0], teamCheck);

   ImGui::End();
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

void loadReplayFromFile(Game* game) {
   char* path = fileOpenUI();

   if (strcmp(path, " ") != 0) {
      game->settings.replayStream = streamLoadFromFile(path);
      replayLoaded = loadReplay(game, game->settings.replayStream);
      if (replayLoaded == true) {
         if (game->playing == true) { gameEndMatch(game); }
         game->settings.replaying = true;
         gameStartMatch(game);
         frameRange[2] = game->settings.repInputs.size() - 1;
      }
      else {
         popupEnable(Popup_LoadFailed);
         replayLoaded = false;
      }
   }
   if (path != nullptr) { delete path; }
}

void replayUI(Game* game) {
   if (!ImGui::Begin("Replay Options")) {
      ImGui::End();
      return;
   }

   if (replayLoaded == true) {
      ImGui::SliderScalar("Speed X", ImGuiDataType_U32, &game->settings.replaySpeed, &frameRate[1], &frameRate[2]);

      ImGui::SliderScalar("Frame", ImGuiDataType_U32, &frameRange[0], &frameRange[1], &frameRange[2]);
      if (ImGui::IsItemDeactivatedAfterEdit() == true) {
         if (frameRange[0] < game->frameCount) {
            if (game->playing == true) { gameEndMatch(game); }
            loadReplay(game, game->settings.replayStream);
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

      if (game->settings.replayPaused == false) {
         if (ImGui::Button("Pause Replay")) {
            game->settings.replayPaused = true;
         }
      }
      else if (game->settings.replayPaused == true) {
         if (ImGui::Button("Unpause Replay")) {
            game->settings.replayPaused = false;
         }
      }
   }
   errorLoadingReplay();

   ImGui::NewLine();
   if (ImGui::Button("Load Replay")) {
      loadReplayFromFile(game);
   }

   ImGui::SameLine();
   if (ImGui::Button("Exit Replays")) {
      if (game->playing == true) { gameEndMatch(game); }
      game->settings.replaying = false;
   }

   ImGui::End();
}

void licensesUI(Game* game, bool* p_open) {
   centerWindow(game, { 440, 400 });
   float width = ImGui::GetWindowContentRegionWidth();

   if (!ImGui::Begin("Licenses", p_open, winFlags) ) {
      ImGui::End();
      return;
   }

   ImGui::NewLine();
   helpfulText("Dear ImGui"); ImGui::SameLine(); ImGui::SetCursorPosX(200);
   if (ImGui::Button("View License##ImGui", { 200, 0 })) {
      fileOpenDefaultProgram(L".\\license\\imgui.txt");
   }

   ImGui::NewLine();
   helpfulText("GGPO"); ImGui::SameLine(); ImGui::SetCursorPosX(200);
   if (ImGui::Button("View License##GGPO", { 200, 0 })) {
      fileOpenDefaultProgram(L".\\license\\ggpo.txt");
   }

   ImGui::NewLine();
   helpfulText("SDL2"); ImGui::SameLine(); ImGui::SetCursorPosX(200);
   if (ImGui::Button("View License##SDL2", { 200, 0 })) {
      fileOpenDefaultProgram(L".\\license\\sdl2.txt");
   }

   ImGui::NewLine();
   helpfulText("SoLoud"); ImGui::SameLine(); ImGui::SetCursorPosX(200);
   if (ImGui::Button("View License##SoLoud", { 200, 0 })) {
      fileOpenDefaultProgram(L".\\license\\soloud.txt");
   }

   ImGui::NewLine();
   helpfulText("MiniUPnP"); ImGui::SameLine(); ImGui::SetCursorPosX(200);
   if (ImGui::Button("View License##MiniUPnP", { 200, 0 })) {
      fileOpenDefaultProgram(L".\\license\\miniupnp.txt");
   }

   ImGui::NewLine();
   helpfulText("STB"); ImGui::SameLine(); ImGui::SetCursorPosX(200);
   if (ImGui::Button("View License##STB", { 200, 0 })) {
      fileOpenDefaultProgram(L".\\license\\stb_image.txt");
   }

   ImGui::End();
}

void creditsUI(Game* game, bool* p_open) {
   centerWindow(game, { 800, 600 });
   static int timer = 0;

   if (!ImGui::Begin("Credits", (bool*)0, winFlags)) {
      timer = 0;
      ImGui::End();
      return;
   }
   timer++;

   ImVec2 currPos = ImGui::GetCursorPos();
   bool done = true;
   for (int i = 0; i < credits.size(); i++) {
      float posY = 500 - (400 / 300) * (timer/2) + (i * 50);
      if (posY > 40 && posY <= 500) {
         ImGui::SetCursorPosY(posY);
         helpfulCenterText(credits[i].c_str());
         done = false;
      }
   }
   if (done == true) { 
      ImGui::NewLine();
      ImGui::PushFont(game->fonts[72]);
      helpfulCenterText("Thanks Everybody!"); 
      ImGui::PopFont();
   }

   ImGui::SetCursorPosY(540);
   ImGui::Separator();
   if (ImGui::Button("Back")) {
      timer = 0;
      *p_open = false;
   }

   ImGui::SameLine();
   if (ImGui::Button("Restart")) {
      timer = 0;
   }

   ImGui::End();
}

void aboutUI(Game* game, bool* p_open) {
   centerWindow(game, { 400, 400 });

   if (!ImGui::Begin("About", p_open, winFlags)) {
      ImGui::End();
      return;
   }

   float width = ImGui::GetWindowContentRegionWidth();

   ImGui::NewLine();
   static bool showInfo = false;
   if (ImGui::Button("Info", ImVec2{ width, 0 })) {
      showInfo = true;
   }
   if (showInfo) {
      gameInfoUI(game, &showInfo);
   }

   ImGui::NewLine();
   static bool showLicenses = false;
   if (ImGui::Button("Licenses", ImVec2{ width, 0 })) {
      showLicenses = true;
   }
   if (showLicenses) {
      licensesUI(game, &showLicenses);
   }

   ImGui::NewLine();
   static bool showCredits = false;
   if (ImGui::Button("Credits", ImVec2{ width, 0 })) {
      showCredits = true;
   }
   if (showCredits) {
      creditsUI(game, &showCredits);
   }

   ImGui::NewLine();
   ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.6f, 0.6f));
   if (ImGui::Button("Back", ImVec2{ width, 0 })) {
      *p_open = false;
   }
   ImGui::PopStyleColor();

   ImGui::End();
}

void gameInfoUI(Game* game, bool* p_open) {
   centerWindow(game, { 500, 500 });

   if (!ImGui::Begin("Info", p_open, winFlags)) {
      ImGui::End();
      return;
   }

   ImGui::TextUnformatted(aboutText);

   ImGui::End();
}

void gameSetupError(int pCount, bool teams[2]) {
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