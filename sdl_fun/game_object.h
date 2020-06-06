#pragma once
#include "game.h"

class GameObject {
public:
	GameObject(const char* texturesheet, int x, int y);
	~GameObject();

   void SetXPosition(int x);
   void SetYPosition(int y);

   int GetXPosition();
   int GetYPosition();

	void Update();
	void Render();

private:
	int xpos;
	int ypos;

	SDL_Texture* objTexture;

   SDL_Rect srcRect;
   SDL_Rect destRect;
};