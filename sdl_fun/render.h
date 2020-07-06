#pragma once

#include <imgui/GL/gl3w/gl3w.h>
#include "game.h"

enum ShaderStage {
   fragment_shader,
   vertex_shader
};

struct Texture {
   GLuint handle;
   int w, h;  //pixels
};

struct Square {
   GLuint vbo;  //vbo handle
   Texture* texture;

   int ptCount = 6;  //to draw it

   float positions[12] =
   {
      -0.5f, 0.5f,
      -0.5f, -0.5f,
      0.5f, -0.5f,

      0.5f, 0.5f,
      0.5f, -0.5f,
      -0.5f, 0.5f,
   };


   float texcoords[12] =
   {
      0.0f, 1.0f,
      0.0f, 0.0f,
      1.0f, 0.0f,

      1.0f, 1.0f,
      1.0f, 0.0f,
      0.0f, 1.0f,
   };

};

//const char* vertexSource;
//const char* fragSource;

int openglContext();

GLuint createVAO();
void destroyVAO(GLuint vao);

GLuint createShader(ShaderStage shaderStage);
void destroyShaders(GLuint shader);

GLuint createProgram();
void useShaderProgram(GLuint program);
void destroyProgram(GLuint program);

GLuint shaderGetUniform(GLuint shaderHandle, const char* name);

void shaderSetMat4(GLuint location, float* mat);

void shaderSetMat4UniformByName(GLuint program, const char* name, float* mat);

void originToWorld(Game* game, float xOrigin, float yOrigin, float width, float height);
void worldToDevice(Game* game, float xOrigin, float yOrigin, float width, float height);

Texture* createTexture(unsigned char* image, int width, int height);
void bindTexture(Square* square);
void destroyTexture(Texture* texture);

Texture* loadTextureFromFile(const char* filename);

Square* createSquare(Game* game);
void destroySquare(Square* square);

void drawSquare(Game* game, Square* square, float destX, float destY, float destW, float destH);

void setRenderTarget(int botLeftX, int botLeftY, int width, int height);
void clearRenderer(float r, float g, float b, float a);