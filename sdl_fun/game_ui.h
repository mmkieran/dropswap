#pragma once

#include "game.h"

enum PopupType {
   Popup_GameOver = 0,
   Popup_Disconnect,
   Popup_Connecting,
   Popup_Error,
   Popup_LoadFailed,
   Popup_Waiting,
   Popup_MultiStarted,
   Popup_GameSetup,
   Popup_Quit,
};

void imguiSetup(Game* game);
void imguiStartFrame(Game* game);

void mainUI(Game* game);
void boardUI(Game* game);

void onePlayerOptions(Game* game);

void gameSettingsUI(Game* game, bool* p_open);

void popupEnable(PopupType popup, int other = 0);
bool popupStatus(PopupType popup);
void popupDisable(PopupType popup);