#pragma once
#include "game.h"

enum TileEnum {
   empty = 0,
   circle,
   diamond,
   utriangle,
   dtriangle,
   star,
   heart,
   silver
};

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

class Cursor {
public:
   Cursor(const char* texturesheet, int x, int y);
   ~Cursor();

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

class Tile {
public:
   Tile(TileEnum type, int x, int y);
   ~Tile();

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