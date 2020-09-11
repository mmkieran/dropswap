#pragma once

typedef struct Game Game;
enum SoundEffect;

void soundsInit();
void soundsDestroy();

int soundsPlaySound(Game* game, SoundEffect sound, background = false, bool silence = false);
void soundsStopAll();
void soundsStopSound(int handle);