#include "cursor.h"
#include "texture_manager.h"

Cursor::Cursor(Game* game, const char* texturesheet, int x, int y) {
   objTexture = TextureManager::LoadTexture(game, texturesheet);
   xpos = x;
   ypos = y;

   height = game->tHeight;
   width = game->tWidth;
}

Cursor::~Cursor() {}

void Cursor::SetXPosition(int x) { xpos = x; }
void Cursor::SetYPosition(int y) { 
   if (y < 0) { ypos = 0; }
   else {ypos = y; }
}

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

void Cursor::Render(Game* game) {
   SDL_RenderCopy(game->renderer, objTexture, &srcRect, &destRect);
}

