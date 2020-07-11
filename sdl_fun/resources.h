#pragma once

#include "game.h"
#include "render.h"

enum TextureEnum {
   Texture_circle = 0,
   Texture_diamond,
   Texture_utriangle,
   Texture_dtriangle,
   Texture_star,
   Texture_heart,
   Texture_silver,
   Texture_garbage,
   Texture_cleared,
   Texture_cursor,
   Texture_frame,
   Texture_COUNT  //this one is used to get the count of all the textures
};

struct Resources;

Resources* initResources();

void destroyResources(Resources* resources);

Texture* resourcesGetTexture(Resources* resources, TextureEnum texture);

unsigned int resourcesGetShader(Game* game);