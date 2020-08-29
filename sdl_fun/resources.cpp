
#include <stdio.h>
#include <vector>

#include "render.h"
#include "resources.h"
#include "game.h"

struct Resources {
   std::vector <Texture*> textures;  //todo maybe use a hashmap?
   GLuint shaderProgram;

   //audio
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

Resources* initResources() {
   Resources* resources = new Resources;

   for (int i = 0; i < Texture_COUNT; i++) {
      if (i == 0){
         resources->textures.push_back(nullptr);
         continue;
      }
      resources->textures.push_back(textureLoadFromFile(_texturePaths[i]) );
   }

   //Might have more than 1 shader eventually?
   resources->shaderProgram = shaderProgramCreate();

   return resources;
}

Resources* destroyResources(Resources* resources) {
   if (resources) {
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

unsigned int resourcesGetShader(Game* game) {
   return game->resources->shaderProgram;
}