#pragma once

#include <imgui/GL/gl3w/gl3w.h>

typedef enum ShaderStage ShaderStage;
typedef struct Game Game;

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

int openglInit();

GLuint createVAO();
void destroyVAO(GLuint vao);

GLuint createShader(ShaderStage shaderStage);
void destroyShaders(GLuint shader);

GLuint createProgram();
void useProgram(GLuint program);
void destroyProgram(GLuint program);

GLuint shaderGetUniform(GLuint shaderHandle, const char* name);

void shaderSetMat4(GLuint location, float* mat);

void shaderSetMat4UniformByName(GLuint program, const char* name, float* mat);

Texture* createTexture(unsigned char* image, int width, int height);
void bindTexture(Square* square);
void destroyTexture(Texture* texture);

Texture* loadTextureFromFile(const char* filename);

Square* createSquare(Game* game);
void destroySquare(Square* square);

void drawSquare(Square* square);

void clearRenderer(float r, float g, float b, float a);