#pragma once

#include "game.h"
#include "render.h"

struct Resources {
   std::vector <Texture*> textures;  //todo maybe use a hashmap?
   GLuint shaderProgram;

   //audio
   //others?
};

Resources* initResources();

void destroyResources(Resources* resources);

Texture* resourcesGetTexture(Resources* resources, int index);

unsigned int resourcesGetShader(Game* game);