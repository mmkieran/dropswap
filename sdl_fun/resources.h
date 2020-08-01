#pragma once

#include "game.h"
#include "render.h"

struct Resources;

Resources* initResources();

Resources* destroyResources(Resources* resources);

Texture* resourcesGetTexture(Resources* resources, TextureEnum texture);

unsigned int resourcesGetShader(Game* game);