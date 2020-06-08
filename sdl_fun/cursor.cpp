#include "cursor.h"
#include "texture_manager.h"

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

