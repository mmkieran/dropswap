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

enum TextureEnum {
   Texture_empty = 0,
   Texture_circle,
   Texture_diamond,
   Texture_utriangle,
   Texture_dtriangle,
   Texture_star,
   Texture_heart,
   Texture_silver,
   Texture_garbage,
   Texture_cleared,
   Texture_cursor,
   Texture_frame,
   Texture_g,
   Texture_COUNT  //this one is used to get the count of all the textures
};

struct Mesh;
struct Texture;
//struct Animation;
//struct Graphic;

struct Animation {
   Texture* texture;
   int frames;  //How many key frames are in the animation
   int delay;  //delay in milliseconds between frames
   int stride; //pixel X distance to next sprite on texture sheet
   int rowStart; //pixel Y distance to top left of sprite sheet
   int width;
   int height;
   bool animated;  //is it animated currently
   uint64_t timer;
};

struct Graphic {
   Texture* texture;
   Animation* animation;
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

Mat4x4 worldToTextureCoords(Game* game, float width, float height);
Mat4x4 textureOriginToWorld(Game* game, float width, float height);

Texture* textureCreate(unsigned char* image, int width, int height);
void textureAttach(Mesh* mesh);
void textureChangeInterp(Mesh* mesh, bool nearest);
void textureParams(Texture* texture, TextureWrap wrap);
void textureDestroy(Texture* texture);
Texture* textureLoadFromFile(const char* filename);

Mesh* meshCreate(Game* game);
void meshDestroy(Mesh* mesh);
void meshDraw(Game* game, Mesh* mesh, float destX, float destY, int destW, int destH);
TextureEnum meshGetTexture(Mesh* mesh);
void meshSetTexture(Game* game, Mesh* mesh, TextureEnum texture);
void textureTransform(Game* game, Mesh* mesh, float sourceX, float sourceY, int sourceW, int sourceH);

Animation* animationCreate(int frames, int delay, int stride, int rowStart, int width, int height, bool animated);
void animationDraw(Game* game, Animation* animation, Mesh* mesh, float destX, float destY, int destW, int destH);
void animationDestroy(Animation* animation);

void rendererSetTarget(int botLeftX, int botLeftY, int width, int height);
void rendererClear(float r, float g, float b, float a);