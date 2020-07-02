#include "texture_manager.h"
#include <stdio.h>
#include "render.h"
#include <vector>


//Need a place to store textures and other assets
//Need a way to retrieve textures and assets
//Should clean them up when it's done

struct Resources {
   std::vector <Texture*> textures;  //todo maybe use a hashmap?
   //audio?
   //others?
};

Resources* initResources() {
   Resources* resources = new Resources;

   resources->textures.push_back(loadTextureFromFile("assets/circle.png"));
   resources->textures.push_back(loadTextureFromFile("assets/diamond.png"));
   resources->textures.push_back(loadTextureFromFile("assets/utriangle.png"));
   resources->textures.push_back(loadTextureFromFile("assets/dtriangle.png"));
   resources->textures.push_back(loadTextureFromFile("assets/star.png"));
   resources->textures.push_back(loadTextureFromFile("assets/heart.png"));
   resources->textures.push_back(loadTextureFromFile("assets/empty.png"));  //silver
   resources->textures.push_back(loadTextureFromFile("assets/skull.png")); //clear

   return resources;
}

void destroyResources(Resources* resources) {
   delete resources;
}

Texture* resourcesGetTexture(Resources* resources, int index) {
   return resources->textures[index];
}

//Texture* getTexture(Tile* tile) {
//   //hard code this for now
//switch (tile->type) {
//case tile_empty:
//   tile->texture = nullptr;
//   //tile->texture = board->game->textures[6];  //debug
//   break;
//case tile_circle:
//   tile->texture = board->game->textures[0];
//   break;
//case tile_diamond:
//   tile->texture = board->game->textures[1];
//   break;
//case tile_utriangle:
//   tile->texture = board->game->textures[2];
//   break;
//case tile_dtriangle:
//   tile->texture = board->game->textures[3];
//   break;
//case tile_star:
//   tile->texture = board->game->textures[4];
//   break;
//case tile_heart:
//   tile->texture = board->game->textures[5];
//   break;
//case tile_silver:
//   tile->texture = board->game->textures[6];
//   break;
//default:
//   tile->texture = nullptr;
//}