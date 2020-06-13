
#include <stdio.h>
#include <thread>
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

            //Setting up the board
            game->bHeight = 12;
            game->bWidth = 6;

            game->tWidth = 64;
            game->tHeight = 64;

            game->board = boardCreate(game); //todo: don't pass in height/width
            game->board->cursor = new Cursor(game, "assets/cursor.png", 0, 0);
            game->textures.empty();
            game->board->game = game;
            gameLoadTextures(game);

            game->board->updateBoard = true; 
            game->board->updateFalling = true;
            game->board->gracePeriod = 0;

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
         if (x >= 4 * game->tWidth) { break; }
         else {
            game->board->cursor->SetXPosition(x + game->tWidth);
            break;
         }

      case SDLK_UP:
         y = game->board->cursor->GetYPosition();
         if (y <= 0) { break; }
         else {
            game->board->cursor->SetYPosition(y - game->tHeight);
            break;
         }

      case SDLK_DOWN:
         y = game->board->cursor->GetYPosition();
         if (y >= 11 * game->tHeight) { break; }
         else {
            game->board->cursor->SetYPosition(y + game->tHeight);
            break;
         }

      case SDLK_SPACE:
         boardSwap(game->board, game->board->cursor);
         break;

      case SDLK_r:
         if (game->board->gracePeriod <= 0) {
            boardMoveUp(game->board, game->board->cursor);
            break;
         }
         break;
      }
   }
}

void gameUpdate(Game* game){

   // Check time interval
   Uint32 current = SDL_GetTicks();
   int boardTime = 5000;
   int boardInterval = (current % boardTime);

   //check if the board needs to be moved
   if ( boardInterval > (boardTime - 10) && game->board->updateBoard == true && game->board->gracePeriod <= 0) {
      boardMoveUp(game->board, game->board->cursor);
      game->board->updateBoard = false;
   }
   else if (boardInterval > (boardTime * 0.1) && boardInterval < (boardTime - 10) && game->board->updateBoard == false) {
      game->board->updateBoard = true;
   }

   //check if blocks need to fall
   int fallTime = 100;
   int fallInterval = (current % fallTime);

   //check if the board needs to be moved
   if (fallInterval > (fallTime - 10) && game->board->updateFalling == true) {
      boardUpdateFalling(game->board);
      game->board->updateFalling = false;
   }
   else if (fallInterval > (fallTime * 0.1) && fallInterval < (fallTime - 10) && game->board->updateFalling == false) {
      game->board->updateFalling = true;
   }

   game->board->cursor->Update();

}

void gameRender(Game* game){
   SDL_RenderClear(game->renderer);
   //Draw game objects
   boardRender(game, game->board);
   game->board->cursor->Render(game);

   //Finish drawing and present
   SDL_RenderPresent(game->renderer);

}

void gameDestroy(Game* game){
   delete game->board->cursor;
   boardDestroy(game->board);
   SDL_DestroyWindow(game->window);
   SDL_DestroyRenderer(game->renderer);
   //free(game);
   delete game;
   SDL_Quit();

   printf("Cleanup successful.\n");
}

bool gameRunning(Game* game) {
   return game->isRunning;
}


//void startTimer(int time) {
//   std::this_thread::sleep_for(std::chrono::milliseconds(time));
//   printf("Woke up after: %d\n", time);
//}