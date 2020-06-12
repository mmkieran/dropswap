#include "texture_manager.h"
#include <stdio.h>
#include <SDL_image.h>
#include "game.h"


SDL_Texture* TextureManager::LoadTexture(Game* game, const char* fileName) {
   SDL_Surface* tempSurface = IMG_Load(fileName);

   if (!tempSurface) { printf("IMG_Load: %s\n", IMG_GetError()); }

   SDL_Texture* texture = SDL_CreateTextureFromSurface(game->renderer, tempSurface);
   SDL_FreeSurface(tempSurface);

   return texture;
}

void TextureManager::Draw(Game* game, SDL_Texture* tex, SDL_Rect src, SDL_Rect dest) {
   SDL_RenderCopy(game->renderer, tex, &src, &dest);
}