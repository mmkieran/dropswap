
#include "sounds.h"
#include "resources.h"

//Soloud class
SoLoud::Soloud gSoloud;

void soundsInit() {
   gSoloud.init();
}

void soundsDestroy() {
   gSoloud.deinit();  //SoLoud shutdown
}

int soundsPlaySound(Game* game, SoundEffect sound) {
   int handle;
   SoLoud::Wav* wav = resourcesGetSound(game->resources, sound);
   if (sound == sound_waltz || sound == sound_anxiety) {
      wav->setLooping(true);
      handle = gSoloud.playBackground(*wav);
      gSoloud.setProtectVoice(handle, true);
   }
   else {
      handle = gSoloud.play(*wav);
   }
   return handle;
}

void soundsStopSound(int handle) {
   gSoloud.setProtectVoice(handle, false);
   gSoloud.stop(handle);
}

void soundsStopAll() {
   gSoloud.stopAll();
}