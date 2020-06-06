#include "map.h"
#include "texture_manager.h"

int tileWidth = 64;
int tileHeight = 64;

int lvl1[12][6] = { 
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0},
   {0, 0, 2, 0, 0, 0},
   {0, 0, 0, 2, 0, 0},
   {0, 0, 0, 0, 0, 1},
   {0, 0, 0, 0, 1, 0},
   {0, 0, 0, 1, 0, 0},
   {0, 0, 1, 0, 0, 0},
   {0, 1, 0, 0, 0, 0},
   {1, 0, 0, 0, 0, 0},

};

Map::Map() {
   dirt = TextureManager::LoadTexture("assets/dirt.png");
   grass = TextureManager::LoadTexture("assets/grass.png");
   water = TextureManager::LoadTexture("assets/water.png");

   LoadMap(lvl1);
   src.x = src.y = 0;
   src.w = dest.w = tileWidth;
   src.h = dest.h = tileHeight;

   dest.x = dest.y = 0;
}

void Map::LoadMap(int arr[12][6]) {
   for (int row = 0; row < 12; row++) {
      for (int col = 0; col < 6; col++) {
         map[row][col] = arr[row][col];
      }
   }
}

void Map::DrawMap() {
   int type = 0;

   for (int row = 0; row < 12; row++) {
      for (int col = 0; col < 6; col++) {
         type = map[row][col];
         dest.x = col * tileWidth;
         dest.y = row * tileHeight;

         switch (type) {
         case 0:
            TextureManager::Draw(water, src, dest);
            break;
         case 1:
            TextureManager::Draw(grass, src, dest);
            break;
         case 2:
            TextureManager::Draw(dirt, src, dest);
            break;
         default:
            break;
         }
      }
   }
}
