
#include <stdio.h>
#include <vector>

#include "render.h"
#include "resources.h"
#include "game.h"

//Need a place to store textures and other assets
//Need a way to retrieve textures and assets
//Should clean them up when it's done

Resources* initResources() {
   Resources* resources = new Resources;

   resources->textures.push_back(loadTextureFromFile("assets/circle.png"));
   resources->textures.push_back(loadTextureFromFile("assets/diamond.png"));
   resources->textures.push_back(loadTextureFromFile("assets/utriangle.png"));
   resources->textures.push_back(loadTextureFromFile("assets/dtriangle.png"));
   resources->textures.push_back(loadTextureFromFile("assets/star.png"));
   resources->textures.push_back(loadTextureFromFile("assets/heart.png"));
   resources->textures.push_back(loadTextureFromFile("assets/empty.png"));  //silver
   resources->textures.push_back(loadTextureFromFile("assets/skull.png")); //clear

   resources->textures.push_back(loadTextureFromFile("assets/cursor.png")); //cursor
   resources->textures.push_back(loadTextureFromFile("assets/frame.png")); //frame

   //Might have more than 1 shader eventually?
   resources->shaderProgram = createProgram();

   return resources;
}

void destroyResources(Resources* resources) {
   if (resources->textures.size() > 0) {
      for (auto&& tex : resources->textures) {
         destroyTexture(tex);
      }
   }
   if (resources->shaderProgram) {
      destroyProgram(resources->shaderProgram);
   }
   delete resources;
}

Texture* resourcesGetTexture(Resources* resources, int index) {
   return resources->textures[index];
}

unsigned int resourcesGetShader(Game* game) {
   return game->resources->shaderProgram;
}