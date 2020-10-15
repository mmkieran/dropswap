#pragma once

#include "game.h"

#include <imgui/GL/gl3w/gl3w.h>

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
   Texture_metal,
   Texture_cursor1,
   Texture_cursor2,
   Texture_COUNT  //this one is used to get the count of all the textures
};

struct Mesh;
struct Texture;
struct FBO;

struct Animation {
   Texture* texture = nullptr;
   int frames;  //How many key frames are in the animation
   int delay;  //delay in milliseconds between frames
   int stride; //pixel X distance to next sprite on texture sheet
   int rowStart; //pixel Y distance to top left of sprite sheet
   int width;
   int height;
   bool animated = false;  //is it animated currently
   uint64_t timer = 0;
};

struct Graphic {
   Texture* texture = nullptr;
   Animation* animation = nullptr;
};

enum VisualEffect {
   visual_none = 0,
   visual_dark,
   visual_shake,
   visual_swapl,  //left swap
   visual_swapr,  //right swap
   visual_clear,
   visual_landing,
   visual_COUNT
};

struct VisualEvent {
   VisualEffect effect = visual_none;
   uint64_t end = 0;
   Vec2 pos;
};

struct FBO {  //Frame Buffer Object
   GLuint handle;
   GLuint texture;
   float w = 0;
   float h = 0;
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

void shaderSetVec4(GLuint location, float* vec4);
void shaderSetVec4UniformByName(GLuint program, const char* name, float* vec4);

void originToWorld(Game* game, float xOrigin, float yOrigin, float width, float height);
void worldToDevice(Game* game, float xOrigin, float yOrigin, float width, float height);

Mat4x4 worldToTextureCoords(Game* game, float width, float height);
Mat4x4 textureOriginToWorld(Game* game, float width, float height);

Texture* textureCreate(unsigned char* image, int width, int height);
Texture* textureLoadFromFile(const char* filename);
void textureChangeInterp(Texture* texture, bool nearest);
void textureTransform(Game* game, Texture* texture, float sourceX, float sourceY, int sourceW, int sourceH);
void textureDestroy(Texture* texture);

Mesh* meshCreate();
Mesh* meshDestroy(Mesh* mesh);
void meshDraw(Board* board, Texture* texture, float destX, float destY, int destW, int destH, VisualEffect effect = visual_none, int effectTime = 0);

Animation* animationCreate(int frames, int delay, int stride, int rowStart, int width, int height, bool animated);
void animationDraw(Board* board, Animation* animation, float destX, float destY, int destW, int destH);
Animation* animationDestroy(Animation* animation);

void rendererSetTarget(int botLeftX, int botLeftY, int width, int height);
void rendererSetScissor(int x, int y, int width, int height);
void rendererEnableScissor();
void rendererDisableScissor();
void rendererClear(float r, float g, float b, float a);

FBO* rendererCreateFBO(Game* game);
int rendererDestroyFBO(FBO* fbo);
void rendererEnableFBO(FBO* fbo);
void rendererDisableFBO();