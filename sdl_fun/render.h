#pragma once

#include <imgui/GL/gl3w/gl3w.h>

struct Vec2;
struct Mat4x4;

enum ShaderStage;

struct Texture {
   GLuint handle;
   int w, h;  //pixels
};

//const char* vertexSource;
//const char* fragSource;

void openglInit();

GLuint createShader(ShaderStage shaderStage);
void deleteShaders(GLuint shader);

GLuint createProgram();
void deleteProgram(GLuint program);

Texture* createTexture(unsigned char* image, int width, int height);
void destroyTexture(Texture* texture);

Texture* loadTextureFromFile(const char* filename);

Square* createSquare();
void deleteSquare(Square* square);