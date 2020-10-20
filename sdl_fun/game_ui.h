#pragma once

#include "game.h"

enum PopupType {
   Popup_GameOver = 0,
   Popup_Disconnect,
   Popup_Waiting,
};

void imguiSetup(Game* game);
void imguiStartFrame(Game* game);

void mainUI(Game* game);
void boardUI(Game* game);

void onePlayerOptions(Game* game);

void ggpoSessionUI(Game* game, bool* p_open);
void ggpoNetStatsUI(Game* game, bool* p_open);
void connectStatusUI(Game* game, bool* p_open);

void gameSettingsUI(Game* game, bool* p_open);

void popupEnable(PopupType popup, int other = 0);
bool popupStatus(PopupType popup);
void popupDisable(PopupType popup);
