
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
   if (tile->type != tile_empty) {
      tile->ID = board->uniqueID;
      board->uniqueID++;
   }
   else {
      board->tileLookup.erase(tile->ID);
      tile->ID = -1;
   }

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

//Check if moving changes the row and copy it to the new position, init old position as empty
void tileSetY(Board* board, Tile* tile, double dist, bool abs) {
   if (abs == false) { tile->ypos += dist; }
   else if (abs == true) { tile->ypos = dist; }

   int row = tileGetRow(board, tile);
   int col = tileGetCol(board, tile);

   int calcRow = (tile->ypos + board->tileHeight - 0.000001) / board->tileHeight + board->startH;
   if (calcRow != row) {
      Tile* dest = boardGetTile(board, calcRow, col);
      if (dest && dest->type == tile_empty) {
         *dest = *tile;
         tileSetTexture(board, dest);
         if (dest->type == tile_garbage && dest->garbage != nullptr) {  //todo we could use tile index instead  
            garbageSetStart(board->pile, dest);
         }
         tileInit(board, tile, row, col, tile_empty);
      }
      else {
         DebugBreak();
      }  
   }
}