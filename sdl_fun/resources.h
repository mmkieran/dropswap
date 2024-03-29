#pragma once

#include "game.h"
#include "render.h"
#include "soloud/soloud.h"
#include "soloud/soloud_wav.h"

struct Resources;

typedef struct Texture Texture;

Resources* initResources();

Resources* destroyResources(Resources* resources);

Texture* resourcesGetTexture(Resources* resources, TextureEnum texture);

SoLoud::Wav* resourcesGetSound(Resources* resources, SoundEffect sound);

unsigned int resourcesGetShader(Game* game);

void resourcesGetName(Game* game);