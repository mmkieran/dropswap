
#include <iostream>
#include <stdlib.h>

#include "game.h"
#include "render.h"

Game *game = nullptr;

const int FPS = 60;
const int frameDelay = 1000 / FPS;
Uint32 frameStart;
int frameTime;


int main(int argc, char* args[])
{
   game = gameCreate("Game test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 900, false);
   if (!game) {
      printf("Failed to create game...\n");
      return -1;
   }

   //Vertex Buffer Object
   GLuint vbo;
   glGenBuffers(1, &vbo);

   glBindBuffer(GL_ARRAY_BUFFER, vbo);  //Make vbo active so we can copy the vertex data

   //copy data from vertices to buffer
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   GLuint shaderProgram = createProgram();
   glUseProgram(shaderProgram);

   //GLint positionAttribute = glGetAttribLocation(shaderProgram, "position");
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);  //point to position
   glEnableVertexAttribArray(0);

   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2*sizeof(float) ) );  //point to texture coords
   glEnableVertexAttribArray(1);

   //glActiveTexture(GL_TEXTURE0);
   Texture* texture = loadTextureFromFile("assets/utriangle.png");
   glBindTexture(GL_TEXTURE_2D, texture->handle);

   while (gameRunning(game)) {
      frameStart = SDL_GetTicks();

      gameHandleEvents(game);

      if (!game->paused) {
         gameUpdate(game);
         gameRender(game);
      }
      else {
         game->pauseLength += SDL_GetTicks() - frameStart;  //Use this to correct timeDelta after pause
      }

      frameTime = SDL_GetTicks() - frameStart;
      if (frameDelay >= frameTime) {  //Wait so we get a steady frame rate
         SDL_Delay(frameDelay - frameTime);
      }

      game->timeDelta = SDL_GetTicks() - frameStart;
      game->timer += game->timeDelta;
   }

   glDeleteProgram(shaderProgram);
   destroyTexture(texture);

   gameDestroy(game);
   return 0;
}
