
#include "resources.h"

#include <time.h>

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
   "assets/metal.png",
   "assets/cursor_tag1.png",
   "assets/cursor_tag2.png",
   "assets/cursor_tag3.png",
   "assets/cursor_tag4.png",
   "assets/sword.png",
   "assets/wall.png",
};

static const char* _soundPaths[] =
{
   "assets/swap.wav",
   "assets/clear.wav",
   "assets/thud.wav",
   "assets/crashland.wav",
   "assets/chain.wav",
   "assets/anxiety.wav",
   "assets/musicbox.wav",
   "assets/siren.mp3",
   "assets/waltz.mp3"
};

Resources* initResources() {
   Resources* resources = new Resources;

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

SoLoud::Wav* resourcesGetSound(Resources* resources, SoundEffect sound) {
   if (resources) {
      return resources->sounds[sound];
   }
}

unsigned int resourcesGetShader(Game* game) {
   return game->resources->shaderProgram;
}

void resourcesGetName(Game* game) {
   FILE* in;
   int err = fopen_s(&in, "assets/character_names.csv", "r");

   if (err == 0) {
      srand(time(0));
      int target = rand() % 486;  
      char* tok;
      char buffer[2048];
      int i = 0;

      //fgets(buffer, 2048, in); // header
      fgets(buffer, 2048, in); //First data line
      while (!feof(in))
      {
         if (i == target) { strncpy(game->user.name, strtok(buffer, ",\n"), 30); } // Get a random name
         i++;
         fgets(buffer, 2048, in);
      }
      fclose(in);
   }
   else { printf("Failed to load file... Err: %d\n", err); }
}