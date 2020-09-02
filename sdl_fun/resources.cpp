
#include <stdio.h>
#include <vector>

#include "render.h"
#include "resources.h"
#include "game.h"

#include "soloud/soloud.h"
#include "soloud/soloud_wav.h"

//Soloud class
SoLoud::Soloud gSoloud;

struct Resources {
   std::vector <Texture*> textures; 
   std::vector <SoLoud::Wav*> sounds;
   GLuint shaderProgram;

   //others?
};

static const char* _texturePaths[] =
{
   "",
   "assets/circle.png",
   "assets/diamond.png",
   "assets/utriangle.png",
   "assets/dtriangle.png",
   "assets/star.png",
   "assets/heart.png",
   "assets/silver.png",
   "assets/garbage.png",
   "assets/cleared.png",
   "assets/cursor.png",
   "assets/frame.png",
   "assets/g.png",
   "assets/metal.png"
};

static const char* _soundPaths[] =
{
   "assets/swap.wav",
   "assets/clear.wav",
   "assets/land.wav",
   "assets/crashland.wav",
   "assets/chain.wav",
   "assets/waltz.mp3"
   //"assets/anxiety.wav"
};

Resources* initResources() {
   Resources* resources = new Resources;
   gSoloud.init();

   for (int i = 0; i < Texture_COUNT; i++) {
      if (i == 0){
         resources->textures.push_back(nullptr);
         continue;
      }
      resources->textures.push_back(textureLoadFromFile(_texturePaths[i]) );
   }

   for (int i = 0; i < sound_COUNT; i++) {
      SoLoud::Wav* gWave = new SoLoud::Wav;
      gWave->load(_soundPaths[i]);
      resources->sounds.push_back(gWave);
   }

   //Might have more than 1 shader eventually?
   resources->shaderProgram = shaderProgramCreate();

   return resources;
}

Resources* destroyResources(Resources* resources) {
   if (resources) {

      gSoloud.deinit();  //SoLoud shutdown

      for (auto&& wave : resources->sounds) {
         if (wave) { delete wave; }
      }

      if (resources->textures.size() > 0) {
         for (auto&& tex : resources->textures) {
            textureDestroy(tex);
         }
      }
      if (resources->shaderProgram) {
         shaderDestroyProgram(resources->shaderProgram);
      }
      delete resources;
   }
   return nullptr;
}

Texture* resourcesGetTexture(Resources* resources, TextureEnum texture) {
   return resources->textures[texture];
}

int resourcesPlaySound(Resources* resources, SoundEffect sound) {
   int handle;
   if (sound == sound_waltz) {
      handle = gSoloud.playBackground(*resources->sounds[sound]);
      resources->sounds[sound]->setLooping(true);
      gSoloud.setProtectVoice(handle, true);
   }
   else {
      handle = gSoloud.play(*resources->sounds[sound]);
   }
   return handle;
}

unsigned int resourcesGetShader(Game* game) {
   return game->resources->shaderProgram;
}