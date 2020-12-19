
#include "sounds.h"
#include "resources.h"

//Soloud class
SoLoud::Soloud gSoloud;

std::map <SoundEffect, MusicInfo> music;

//Initialize SoLoud
void soundsInit() {
   gSoloud.init();
}

//Destroy SoLoud
void soundsDestroy() {
   gSoloud.deinit();  //SoLoud shutdown
}

void soundsPlayMusic(Game* game, SoundEffect sound, bool loop) {
   SoLoud::Wav* wav = resourcesGetSound(game->resources, sound);
   wav->setLooping(loop);
   music[sound].handle = gSoloud.playBackground(*wav);
   gSoloud.setProtectVoice(music[sound].handle, true);
   music[sound].play = true;
}

void soundsPlaySound(Game* game, SoundEffect sound) {
   SoLoud::Wav* wav = resourcesGetSound(game->resources, sound);
   gSoloud.play(*wav);
}

MusicInfo soundsGetMusicInfo(SoundEffect sound) {
   return music[sound];
}

void soundsSetMusicInfo(SoundEffect sound, MusicInfo info) {
   music[sound] = info;
}

//Set the pause state of a sound by voice handle
void soundsPauseSound(int handle, bool state) {
   gSoloud.setPause(handle, state);
}

//Get the volume of a sound using its voice handle
float soundsGetVolume(int handle) {
   return gSoloud.getVolume(handle);
}

//Set the volume of a sound using its voice handle
void soundsSetVolume(int handle, float vol) {
   gSoloud.setVolume(handle, vol);
}

//Get the global volume of all sounds
float soundsGetGlobalVolume() {
   return gSoloud.getGlobalVolume();
}

//Set the global volume of all sounds
void soundsSetGlobalVolume(float vol) {
   return gSoloud.setGlobalVolume(vol);
}

//Stop a sound even if it's protected
void soundsStopSound(int handle) {
   gSoloud.setProtectVoice(handle, false);
   gSoloud.stop(handle);
}

//Stop all sounds (even if protected)
void soundsStopAll() {
   gSoloud.stopAll();
}