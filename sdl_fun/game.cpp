
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
            cursor = new Cursor("assets/cursor.png", 0, 0);
            board = boardCreate(BOARD_HEIGHT, BOARD_WIDTH);
            boardFillTiles(board);

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
            cursor->SetXPosition(x - 64);
            break;
         }

      case SDLK_RIGHT:
         x = cursor->GetXPosition();
         if (x >= 4 * 64) { break; }
         else {
            cursor->SetXPosition(x + 64);
            break;
         }

      case SDLK_UP:
         y = cursor->GetYPosition();
         if (y <= 0) { break; }
         else {
            cursor->SetYPosition(y - 64);
            break;
         }

      case SDLK_DOWN:
         y = cursor->GetYPosition();
         if (y >= 11 * 64) { break; }
         else {
            cursor->SetYPosition(y + 64);
            break;
         }

      case SDLK_SPACE:
         boardSwap(board, cursor);
      }
   }
}

void Game::update(){
   Uint32 current = SDL_GetTicks();
   double calc = (current % 10000);
   static bool boardMoved = false;

   if ( calc > 9990 && boardMoved == false) {
      boardMoveUp(board);
      boardMoved = true;
   }
   else if (calc > 1000 && calc < 9990 && boardMoved == true) {
      boardMoved = false;
   }
   //This is just some debug stuff
   Game::count += 1;
   //printf("%d I guess it's working...\n", Game::count);
   cursor->Update();
   //boardUpdate(board);

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