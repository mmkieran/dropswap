#pragma once

#include "game.h"

void soundsInit();
void soundsDestroy();

int soundsPlaySound(Game* game, SoundEffect sound);
void soundsStopAll();
void soundsStopSound(int handle);