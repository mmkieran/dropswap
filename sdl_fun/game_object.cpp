#include "game_object.h"
#include "texture_manager.h"

GameObject::GameObject(const char* texturesheet, int x, int y) {
	objTexture = TextureManager::LoadTexture(texturesheet);
   xpos = x;
   ypos = y;
}

void GameObject::SetXPosition(int x) { xpos = x; }
void GameObject::SetYPosition(int y) { ypos = y; }

int GameObject::GetXPosition() { return xpos; }
int GameObject::GetYPosition() { return ypos; }

void GameObject::Update() {

	srcRect.h = 64;
	srcRect.w = 64;

	srcRect.x = 0;
	srcRect.y = 0;

	destRect.x = xpos;
	destRect.y = ypos;

	destRect.w = srcRect.w;
	destRect.h = srcRect.h;
}

void GameObject::Render() {
   SDL_RenderCopy(Game::renderer, objTexture, &srcRect, &destRect);
}

Cursor::Cursor(const char* texturesheet, int x, int y) {
   objTexture = TextureManager::LoadTexture(texturesheet);
   xpos = x;
   ypos = y;
}

void Cursor::SetXPosition(int x) { xpos = x; }
void Cursor::SetYPosition(int y) { ypos = y; }

int Cursor::GetXPosition() { return xpos; }
int Cursor::GetYPosition() { return ypos; }

void Cursor::Update() {

   srcRect.h = 32;
   srcRect.w = 32;

   srcRect.x = 0;
   srcRect.y = 0;

   destRect.x = xpos;
   destRect.y = ypos;

   destRect.w = srcRect.w * 4;
   destRect.h = srcRect.h * 2;
}

void Cursor::Render() {
   SDL_RenderCopy(Game::renderer, objTexture, &srcRect, &destRect);
}

Tile::Tile(TileEnum type, int x, int y) {
   const char* texturesheet;

   switch (type) {
   case empty:
      break;
   case circle:
      const char* texturesheet = "assets/grass.png";
      break;
   case diamond:
      const char* texturesheet = "assets/dirt.png";
      break;
   case utriangle:
      const char* texturesheet = "assets/water.png";
      break;
   case dtriangle:
      break;
   case star:
      break;
   case heart:
      break;
   case silver:
      break;
   }

   objTexture = TextureManager::LoadTexture(texturesheet);
   xpos = x;
   ypos = y;
}

void Tile::SetXPosition(int x) { xpos = x; }
void Tile::SetYPosition(int y) { ypos = y; }

int Tile::GetXPosition() { return xpos; }
int Tile::GetYPosition() { return ypos; }

void Tile::Update() {

   srcRect.h = 32;
   srcRect.w = 32;

   srcRect.x = 0;
   srcRect.y = 0;

   destRect.x = xpos;
   destRect.y = ypos;

   destRect.w = srcRect.w * 2;
   destRect.h = srcRect.h * 2;
}

void Tile::Render() {
   SDL_RenderCopy(Game::renderer, objTexture, &srcRect, &destRect);
}