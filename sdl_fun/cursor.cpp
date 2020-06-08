#include "cursor.h"
#include "texture_manager.h"

Cursor::Cursor(const char* texturesheet, int x, int y, int cursor_width, int cursor_height) {
   objTexture = TextureManager::LoadTexture(texturesheet);
   xpos = x;
   ypos = y;

   height = cursor_height;
   width = cursor_height;
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

   destRect.w = width * 2;
   destRect.h = height;
}

void Cursor::Render() {
   SDL_RenderCopy(Game::renderer, objTexture, &srcRect, &destRect);
}

