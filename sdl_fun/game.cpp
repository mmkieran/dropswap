
#include "game.h"
#include <stdio.h>
#include <thread>
#include <chrono>
#include "texture_manager.h"


Cursor* cursor = nullptr;
Board* board = nullptr;
SDL_Renderer* Game::renderer = nullptr;


Game::Game() {}
Game::~Game() {}

void Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen){
   int flags = 0;
   renderer = nullptr;
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

            //Loading up our game elements
            bHeight = 12;
            bWidth = 6;

            tWidth = 64;
            tHeight = 64;

            cursor = new Cursor("assets/cursor.png", 0, 0, tHeight, tWidth);

            board = boardCreate(bHeight, bWidth, tHeight, tWidth);
            boardFillTiles(board);
            updateBoard = true;
            updateFalling = true;
            board->gracePeriod = false;
            //todo: remove this later
            std::thread test(startTimer, 3000);
            test.detach();

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
            cursor->SetXPosition(x - tWidth);
            break;
         }

      case SDLK_RIGHT:
         x = cursor->GetXPosition();
         if (x >= 4 * tWidth) { break; }
         else {
            cursor->SetXPosition(x + tWidth);
            break;
         }

      case SDLK_UP:
         y = cursor->GetYPosition();
         if (y <= 0) { break; }
         else {
            cursor->SetYPosition(y - tHeight);
            break;
         }

      case SDLK_DOWN:
         y = cursor->GetYPosition();
         if (y >= 11 * tHeight) { break; }
         else {
            cursor->SetYPosition(y + tHeight);
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
   int boardTime = 10000;
   int boardInterval = (current % boardTime);

   //check if the board needs to be moved
   if ( boardInterval > (boardTime - 10) && updateBoard == true && board->gracePeriod == true) {
      boardMoveUp(board, cursor);
      updateBoard = false;
   }
   else if (boardInterval > (boardTime * 0.1) && boardInterval < (boardTime - 10) && updateBoard == false) {
      updateBoard = true;
   }

   //check if blocks need to fall
   int fallTime = 200;
   int fallInterval = (current % fallTime);

   //check if the board needs to be moved
   if (fallInterval > (fallTime - 10) && updateFalling == true) {
      boardUpdateFalling(board);
      updateFalling = false;
   }
   else if (fallInterval > (fallTime * 0.1) && fallInterval < (fallTime - 10) && updateFalling == false) {
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

void startTimer(int time) {
   std::this_thread::sleep_for(std::chrono::milliseconds(time));
   printf("Woke up after: %d", time);
}