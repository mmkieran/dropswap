#pragma once

#include <imgui/GL/gl3w/gl3w.h>
#include "game.h"

enum TextureWrap {
   repeat,
   mirror,
};

enum ShaderStage {
   fragment_shader,
   vertex_shader
};

struct Texture {
   GLuint handle;
   int w, h;  //pixels
};

struct Mesh {
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

int openglContext();

GLuint vaoCreate();
void vaoDestroy(GLuint vao);

GLuint shaderCreate(ShaderStage shaderStage);
void shaderDestroy(GLuint shader);

GLuint shaderProgramCreate();
void shaderUseProgram(GLuint program);
void shaderDestroyProgram(GLuint program);

GLuint shaderGetUniform(GLuint shaderHandle, const char* name);

void shaderSetMat4(GLuint location, float* mat);

void shaderSetMat4UniformByName(GLuint program, const char* name, float* mat);

void originToWorld(Game* game, float xOrigin, float yOrigin, float width, float height);
void worldToDevice(Game* game, float xOrigin, float yOrigin, float width, float height);

Texture* textureCreate(unsigned char* image, int width, int height);
void textureAttach(Mesh* mesh);
void textureParams(Texture* texture, TextureWrap wrap);
void textureDestroy(Texture* texture);

Texture* textureLoadFromFile(const char* filename);

Mesh* meshCreate(Game* game);
void meshDestroy(Mesh* mesh);

void meshDraw(Game* game, Mesh* mesh, float destX, float destY, int destW, int destH);

void rendererSetTarget(int botLeftX, int botLeftY, int width, int height);
void rendererClear(float r, float g, float b, float a);