#pragma once

#include <imgui/GL/gl3w/gl3w.h>

struct Vec2;
struct Mat4x4;

enum ShaderStage;

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

void openglInit();

GLuint createShader(ShaderStage shaderStage);
void destroyShaders(GLuint shader);

GLuint createProgram();
void destroyProgram(GLuint program);

Texture* createTexture(unsigned char* image, int width, int height);
void destroyTexture(Texture* texture);

Texture* loadTextureFromFile(const char* filename);

Square* createSquare();
void destroySquare(Square* square);

void drawSquare(Square* square);

void clearRenderer(float r, float g, float b, float a);