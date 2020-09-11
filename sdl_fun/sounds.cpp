
#include "sounds.h"
#include "resources.h"

//Soloud class
SoLoud::Soloud gSoloud;
std::map <SoundEffect, SoundState> soundBoard;

struct SoundState {
   bool play = false;
   int coolDown = 0;
   bool loop = false;
   bool interrupt = false;  //Other sounds don't stop it
   int position = 0;
   float finish = 0;  //Ending time in secs based on game clock
};

void soundsInit() {
   gSoloud.init();
}

void soundsDestroy() {
   gSoloud.deinit();  //SoLoud shutdown
}

static void _setCoolDown(SoundEffect sound) {
   switch (sound) {
   case sound_crashland:
      soundBoard[sound].coolDown = 50;
   default:
      soundBoard[sound].coolDown = 1;
   }
}

int soundsPlaySound(Game* game, SoundEffect sound, background = false, bool silence = false) {
   if (soundBoard[sound].coolDown <= game->timer) { return; }
   if (soundBoard[sound].finish >= game->timer) { return; }
   if (soundBoard[sound].play == true) { return; }

   int handle;
   SoLoud::Wav* wav = resourcesGetSound(game->resources, sound);

   if (background == true) { 
      wav->setLooping(true);
      soundBoard[sound].loop = true;
      handle = gSoloud.playBackground(*wav); 
      gSoloud.setProtectVoice(handle, true);
      soundBoard[sound].interrupt = false;
   }
   else { handle = gSoloud.play(*wav); }
   soundBoard[sound].play = true;
   soundBoard[sound].finish = game->timer + wav->getLength();
   _setCoolDown(sound);

   return handle;
}

void soundsStopSound(int handle) {
   gSoloud.setProtectVoice(handle, false);
   gSoloud.stop(handle);
}

void soundsStopAll() {
   gSoloud.stopAll();
}