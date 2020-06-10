
#include "game.h"
#include <stdio.h>
#include "texture_manager.h"
#include "cursor.h"
#include "board.h"

Cursor* cursor;
Board* board;

SDL_Renderer* Game::renderer = nullptr;

int BOARD_HEIGHT = 12;
int BOARD_WIDTH = 6;

int TILE_WIDTH = 64;
int TILE_HEIGHT = 64;

Uint32 gameStart = SDL_GetTicks();

Game::Game() {}
Game::~Game() {}

void Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen){
   int flags = 0;
   if (fullscreen) {
      flags = SDL_WINDOW_FULLSCREEN;
   }

   if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
      printf("SDL subsystems initialized...\n");
      
      window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
      if (window) {
         printf("Window created.\n");

         renderer = SDL_CreateRenderer(window, -1, 0);
         if (renderer) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            printf("Renderer made.\n");

            //Loading our player
            cursor = new Cursor("assets/cursor.png", 0, 0, TILE_HEIGHT, TILE_WIDTH);
            board = boardCreate(BOARD_HEIGHT, BOARD_WIDTH, TILE_HEIGHT, TILE_WIDTH);
            boardFillTiles(board);
            updateBoard = true;
            updateFalling = true;
            

            isRunning = true;
            return;
         }
      }
   }
   isRunning = false;
}

void Game::handleEvents(){
   SDL_Event event;
   SDL_PollEvent(&event);
   switch (event.type) {
   case SDL_QUIT:
      isRunning = false;
      break;

   case SDL_KEYDOWN:
      int x = 0;
      int y = 0;
      switch (event.key.keysym.sym) {

      case SDLK_LEFT:
         x = cursor->GetXPosition();
         if (x <= 0) { break; }
         else {
            cursor->SetXPosition(x - TILE_WIDTH);
            break;
         }

      case SDLK_RIGHT:
         x = cursor->GetXPosition();
         if (x >= 4 * TILE_WIDTH) { break; }
         else {
            cursor->SetXPosition(x + TILE_WIDTH);
            break;
         }

      case SDLK_UP:
         y = cursor->GetYPosition();
         if (y <= 0) { break; }
         else {
            cursor->SetYPosition(y - TILE_HEIGHT);
            break;
         }

      case SDLK_DOWN:
         y = cursor->GetYPosition();
         if (y >= 11 * TILE_HEIGHT) { break; }
         else {
            cursor->SetYPosition(y + TILE_HEIGHT);
            break;
         }

      case SDLK_SPACE:
         boardSwap(board, cursor);
         break;

      case SDLK_r:
         boardMoveUp(board, cursor);
         break;
      }
   }
}

void Game::update(){

   // Check time interval
   Uint32 current = SDL_GetTicks();
   int boardInterval = (current % 10000);

   //check if the board needs to be moved
   if ( boardInterval > 9990 && updateBoard == true) {
      boardMoveUp(board, cursor);
      updateBoard = false;
   }
   else if (boardInterval > 1000 && boardInterval < 9990 && updateBoard == false) {
      updateBoard = true;
   }

   //check if blocks need to fall
   int fallTime = 100;
   int fallInterval = (current % fallTime);

   //check if the board needs to be moved
   if (fallInterval > (fallTime * 0.9) && updateFalling == true) {
      boardUpdateFalling(board);
      updateFalling = false;
   }
   else if (fallInterval > (fallTime * 0.1) && fallInterval < (fallTime * 0.9) && updateFalling == false) {
      updateFalling = true;
   }

   cursor->Update();

}

void Game::render(){
   SDL_RenderClear(renderer);
   //Draw game objects
   boardRender(board);
   cursor->Render();

   //Finish drawing and present
   SDL_RenderPresent(renderer);

}

void Game::clean(){
   boardDestroy(board);
   SDL_DestroyWindow(window);
   SDL_DestroyRenderer(renderer);
   SDL_Quit();

   printf("Cleanup successful.\n");
}

bool Game::running() {
   return isRunning;
}
