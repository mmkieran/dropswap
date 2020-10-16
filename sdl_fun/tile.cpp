
#include "tile.h"
#include "resources.h"

//Map the tile type to the texture for the renderer
std::map <TileType, TextureEnum> typeMap = {
   {tile_empty     , Texture_empty},
   {tile_circle    , Texture_circle},
   {tile_diamond   , Texture_diamond},
   {tile_utriangle , Texture_utriangle},
   {tile_dtriangle , Texture_dtriangle},
   {tile_star      , Texture_star},
   {tile_heart     , Texture_heart},
   {tile_silver    , Texture_silver},
   {tile_cleared   , Texture_cleared},

};

//Gets the correct texture for the tile based on its type
void tileSetTexture(Board* board, Tile* tile) {
   TextureEnum type = typeMap[tile->type];

   //Special garbage case for metals... yuck
   if (tile->type == tile_garbage) {
      if (tile->idGarbage >= 0) {
         Garbage* garbage = garbageGet(board->pile, tile->idGarbage);
         if (garbage && garbage->metal == true) {
            tile->texture = resourcesGetTexture(board->game->resources, Texture_metal);
         }
         else { tile->texture = resourcesGetTexture(board->game->resources, Texture_g); }
      }
      else {
         tile->texture = resourcesGetTexture(board->game->resources, Texture_g);
      }
   }

   //Everything else
   else {
      tile->texture = resourcesGetTexture(board->game->resources, type);
   }
}

void tileInit(Board* board, Tile* tile, int row, int col, TileType type) {
   tile->type = type;
   tile->status = status_normal;
   tile->effect = visual_none;  //We don't need to save this because it only happens during render
   tile->effectTime = 0;
   tile->xpos = col * board->tileWidth;
   tile->ypos = (row - board->startH) * board->tileHeight;

   tile->idGarbage = -1;
   tile->garbage = nullptr;

   tileSetTexture(board, tile);

   tile->clearTime = 0;
   tile->statusTime = 0;
   tile->falling = false;
   tile->chain = false;
}

void tileDraw(Board* board, Tile* tile, VisualEffect effect, int effectTime) {
   if (tile->type != tile_empty) {
      if (tile->status == status_disable || tileGetRow(board, tile) == board->wBuffer - 1) {
         effect = visual_dark;
      }
      meshDraw(board, tile->texture, tile->xpos, tile->ypos, board->tileWidth, board->tileHeight, effect, effectTime);
   }
}