
#include <stdio.h>
#include <chrono>

#include "game.h"
#include "board.h"
#include "texture_manager.h"
#include "cursor.h"
#include "tile.h"

Game* gameCreate(const char* title, int xpos, int ypos, int width, int height, bool fullscreen){
   //Game* game = (Game*)malloc(sizeof(Game));
   Game* game = new Game;

   int flags = 0;

   if (fullscreen) {
      flags = SDL_WINDOW_FULLSCREEN;
   }

   if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
      printf("SDL subsystems initialized...\n");
      
      game->window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
      if (game->window) {
         printf("Window created.\n");

         game->renderer = SDL_CreateRenderer(game->window, -1, 0);
         if (game->renderer) {
            SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
            printf("Renderer made.\n");

            //Setting up the game settings
            game->bHeight = 12;
            game->bWidth = 6;

            game->tWidth = 64;
            game->tHeight = 64;

            game->timer = 0;

            //setting up board
            game->board = boardCreate(game); 
            game->board->cursor = new Cursor(game, "assets/cursor.png", (game->bWidth / 2 - 1) * game->tWidth, (game->bHeight / 2 + 1) * game->tHeight);
            game->board->game = game;
            gameLoadTextures(game);

            game->board->gracePeriod = 0;
            game->board->paused = false;
            game->board->pauseLength = 0;

            //todo: remove this later or fix algorithm so there are no matches at the start
            boardFillTiles(game->board);

            game->isRunning = true;
            return game;
         }
      }
   }
   else {
      game->isRunning = false;
      return nullptr;
   }
}

void gameLoadTextures(Game* game) {

   game->textures.push_back( TextureManager::LoadTexture(game, "assets/circle.png"));
   game->textures.push_back( TextureManager::LoadTexture(game, "assets/diamond.png"));
   game->textures.push_back( TextureManager::LoadTexture(game, "assets/utriangle.png"));
   game->textures.push_back( TextureManager::LoadTexture(game, "assets/dtriangle.png"));
   game->textures.push_back( TextureManager::LoadTexture(game, "assets/star.png"));
   game->textures.push_back( TextureManager::LoadTexture(game, "assets/heart.png"));
   game->textures.push_back( TextureManager::LoadTexture(game, "assets/grass.png"));

   //game->textures[tile_diamond] = TextureManager::LoadTexture(game, "assets/diamond.png");
   //game->textures[tile_utriangle] = TextureManager::LoadTexture(game, "assets/utriangle.png");
   //game->textures[tile_dtriangle] = TextureManager::LoadTexture(game, "assets/dtriangle.png");
   //game->textures[tile_star] = TextureManager::LoadTexture(game, "assets/star.png");
   //game->textures[tile_circle] = TextureManager::LoadTexture(game, "assets/circle.png");
   //game->textures[tile_heart] = TextureManager::LoadTexture(game, "assets/heart.png");
   //game->textures[tile_silver] = TextureManager::LoadTexture(game, "assets/grass.png");
}

void gameHandleEvents(Game* game){
   SDL_Event event;
   SDL_PollEvent(&event);
   switch (event.type) {
   case SDL_QUIT:
      game->isRunning = false;
      break;

   case SDL_KEYDOWN:
      int x = 0;
      int y = 0;
      switch (event.key.keysym.sym) {

      case SDLK_LEFT:
         x = game->board->cursor->GetXPosition();
         if (x <= 0) { break; }
         else {
            game->board->cursor->SetXPosition(x - game->tWidth);
            break;
         }

      case SDLK_RIGHT:
         x = game->board->cursor->GetXPosition();
         if (x >= (game->bWidth -2) * game->tWidth) { break; }
         else {
            game->board->cursor->SetXPosition(x + game->tWidth);
            break;
         }

      case SDLK_UP:
         y = game->board->cursor->GetYPosition();
         if (y - game->tHeight <= 0) { break; }
         else {
            game->board->cursor->SetYPosition(y - game->tHeight);
            break;
         }

      case SDLK_DOWN:
         y = game->board->cursor->GetYPosition();
         if (y >= 11 * game->tHeight + game->board->offset) { break; }
         else {
            game->board->cursor->SetYPosition(y + game->tHeight);
            break;
         }

      case SDLK_SPACE:
         boardSwap(game->board, game->board->cursor);
         break;

      case SDLK_r:
         if (game->board->gracePeriod <= 0) {
            boardMoveUp(game->board);
            break;
         }
         break;
      }
   }
}

void gameUpdate(Game* game){

   if (game->board->pauseLength > 0) {
      game->board->pauseLength -= game->timeDelta;

      if (game->board->pauseLength < 0) {
         game->board->paused = false;
         game->board->pauseLength = 0;
      }
   }
   else {
      game->board->paused = false;
   }

   //Update board
   if (game->board->paused == false) {
      
      if (game->board->moveTimer + 100 <= SDL_GetTicks()) {
         boardMoveUp(game->board);
         game->board->moveTimer = SDL_GetTicks();
      }
      else {

      }
   }

   //Update falling blocks
   if (game->board->fallTimer + 100 <= SDL_GetTicks()) {
      boardUpdateFalling(game->board);
      game->board->fallTimer = SDL_GetTicks();
   }

   boardClearBlocks(game->board);

   game->board->cursor->Update(game);


}

void gameRender(Game* game){
   SDL_RenderClear(game->renderer);
   //Draw game objects
   boardRender(game, game->board);

   //Finish drawing and present
   SDL_RenderPresent(game->renderer);

}

void gameDestroy(Game* game){
   delete game->board->cursor;
   boardDestroy(game->board);
   SDL_DestroyWindow(game->window);
   SDL_DestroyRenderer(game->renderer);
   SDL_Quit();
   //free(game);
   delete game;

   printf("Cleanup successful.\n");
}

bool gameRunning(Game* game) {
   return game->isRunning;
}