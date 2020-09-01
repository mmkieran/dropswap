#pragma once

#include "game.h"
#include "render.h"

struct Resources;

Resources* initResources();

Resources* destroyResources(Resources* resources);

Texture* resourcesGetTexture(Resources* resources, TextureEnum texture);
int resourcesPlaySound(Resources* resources, SoundEffect sound);

unsigned int resourcesGetShader(Game* game);