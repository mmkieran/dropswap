#pragma once
#include "render.h"

typedef struct Resources Resources;

Resources* initResources();

void destroyResources(Resources* resources);

Texture* resourcesGetTexture(Resources* resources, int index);

unsigned int resourcesGetShader(Game* game);