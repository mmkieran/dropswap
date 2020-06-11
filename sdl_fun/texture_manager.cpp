#include "texture_manager.h"
#include <stdio.h>
#include <SDL_image.h>
#include <map>
#include "game.h"

//#include "tile.h"

//std::map<TileEnum, SDL_Texture> textures;

SDL_Texture* TextureManager::LoadTexture(const char* fileName) {
   SDL_Surface* tempSurface = IMG_Load(fileName);
   if (!tempSurface) { printf("IMG_Load: %s\n", IMG_GetError()); }
   SDL_Texture* texture = SDL_CreateTextureFromSurface(Game::renderer, tempSurface);
   SDL_FreeSurface(tempSurface);
   return texture;
}

void TextureManager::Draw(SDL_Texture* tex, SDL_Rect src, SDL_Rect dest) {
   SDL_RenderCopy(Game::renderer, tex, &src, &dest);
}