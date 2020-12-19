#pragma once

#include "game.h"

struct MusicInfo {
   unsigned int handle = 0;
   bool play = false;
   bool paused = false;
};

void soundsInit();
void soundsDestroy();

void soundsPauseSound(int handle, bool state);

MusicInfo soundsGetMusicInfo(SoundEffect sound);
void soundsSetMusicInfo(SoundEffect sound, MusicInfo info);

float soundsGetVolume(int handle);
void soundsSetVolume(int handle, float vol);
float soundsGetGlobalVolume();
void soundsSetGlobalVolume(float vol);

void soundsPlaySound(Game* game, SoundEffect sound);
void soundsPlayMusic(Game* game, SoundEffect sound, bool loop = true);

void soundsStopAll();
void soundsStopSound(int handle);