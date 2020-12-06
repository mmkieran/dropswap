
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
   else { tile->ID = -1; }

   tile->status = status_normal;
   tile->effect = visual_none;  //We don't need to save this because it only happens during render
   tile->effectTime = 0;
   tile->xpos = col * board->tileWidth;
   tile->ypos = (row - board->startH) * board->tileHeight;

   tile->idGarbage = -1;
   tile->garbage = nullptr;

   tileSetTexture(board, tile);

   tile->statusTime = 0;
   tile->falling = false;
   tile->chain = false;
}

void tileDraw(Board* board, Tile* tile, VisualEffect effect, int effectTime) {
   if (tile->type != tile_empty) {
      DrawInfo info;

      //Color transformations
      if (tile->status == status_disable || tileGetRow(board, tile) == board->wBuffer - 1) {
         for (int i = 0; i < 4; i++) { info.color[i] = 0.8; }
      }
      else { for (int i = 0; i < 4; i++) { info.color[i] = 1.0; } }

      if (effect == visual_countdown) {
         for (int i = 0; i < 4; i++) {
            float val = 1.0 * (effectTime - board->game->timer + board->game->timings.removeClear[0] / 4) / board->game->timings.removeClear[0];
            info.color[i] = val < 0 ? 0 : val;
         }
      }

      //Camera movements or mesh displacements
      Vec2 move = { 0, 0 };
      if (effect == visual_swapr) {
         move.x -= board->tileWidth * (effectTime - board->game->timer) / SWAPTIME;
      }
      else if (effect == visual_swapl) {
         move.x += board->tileWidth * (effectTime - board->game->timer) / SWAPTIME;
      }
      //Tremble on garbage landing
      if (board->visualEvents[visual_shake].active == true) {
         move.y += sin(board->game->timer) * 2;
      }
      info.cam = move;

      meshSetDrawRect(info, tile->xpos, tile->ypos, board->tileWidth, board->tileHeight, 0);
      meshDraw(board->game, tile->texture, info);
      if (tile->status == status_clear) {
         meshDraw(board->game, resourcesGetTexture(board->game->resources, Texture_cleared), info);
      }
   }
}

//Check if moving changes the row and copy it to the new position, init old position as empty
void tileAssignSlot(Board* board, Tile* tile) {
   int row = tileGetRow(board, tile);
   int col = tileGetCol(board, tile);

   int calcRow = (tile->ypos + board->tileHeight - 0.000001) / board->tileHeight + board->startH;
   if (calcRow != row) {
      Tile* dest = boardGetTile(board, calcRow, col);
      if (dest && dest->type == tile_empty) {
         *dest = *tile;
         tileSetTexture(board, dest);
         if (dest->type == tile_garbage && dest->garbage != nullptr) {  
            garbageSetStart(board->pile, dest);
         }
         tile->ID = -1;
         tileInit(board, tile, row, col, tile_empty);
         board->tileLookup[dest->ID] = dest;
      }
      else {
         DebugBreak();
      }  
   }
}