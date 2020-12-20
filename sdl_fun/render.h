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
   Texture_cursor3,
   Texture_cursor4,
   Texture_sword,
   Texture_wall,
   Texture_COUNT  //this one is used to get the count of all the textures
};

struct Mesh;
struct Texture;
struct FBO;
struct Graphic;

struct Animation {
   Texture* texture = nullptr;
   int frames;                                  //How many key frames are in the animation
   int delay;                                   //delay in milliseconds between frames
   int stride;                                  //pixel X distance to next sprite on texture sheet
   int rowStart;                                //pixel Y distance to top left of sprite sheet
   int width;                                   //The width of the texture on the texture sheet
   int height;                                  //The height of the texture on the texture sheet
   bool animated = false;                       //is it animated currently
   uint64_t timer = 0;
};

struct Graphic {
   Texture* texture = nullptr;
   Animation* animation = nullptr;
};

struct Rect {
   float x, y;                                  //Top left corner of rectangle
   int h, w;                                    //Rectangle height and width
};

struct DrawInfo {
   Rect rect;                                   //Determines the size of the mesh square
   int rot = 0;                                 //Rotation of mesh
   Vec2 cam = { 0, 0 };                         //Has the camera moved it
   float color[4] = { 0,0,0,0 };                //Does it have color transformations applied
};

struct Sprite {
   DrawInfo info;
   int dir = 0;                                 //Clockwise angle from up (0) that the sprite is heading 
   float speed = 0;                            //Pixel speed the sprite is moving
   Graphic render;                              //Struct to hold texture or animation pointer
   int stop = 0;
   int end = 0;                                 //Time in milliseconds when it will die
};

enum VisualEffect {
   visual_none = 0,
   visual_dark,                                 //Darken disabled tiles (buffer row and spawning tiles from cleared garbage)
   visual_shake,                                //Shake all tiles on the screen
   visual_swapl,                                //Used to interpolate position of tile when swapping left
   visual_swapr,                                //Used to interpolate position of tile when swapping right
   visual_countdown,                            //Used to fade out cleared tiles
   visual_clear,                                //Used to generate the chain message on the board
   visual_landing,                              //Currently not used
   visual_COUNT
};

struct VisualEvent {
   bool active = false;
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

struct Texture {
   GLuint handle;
   int w, h;  //pixels
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
void meshSetDrawRect(DrawInfo &info, float x, float y, int w, int h, int rot);
void meshDraw(Game* game, Texture* texture, DrawInfo info);

Animation* animationCreate(int frames, int delay, int stride, int rowStart, int width, int height, bool animated);
void animationDraw(Game* game, Animation* animation, DrawInfo info);
Animation* animationDestroy(Animation* animation);

void rendererSetTarget(int botLeftX, int botLeftY, int width, int height);
void rendererSetScissor(int x, int y, int width, int height);
void rendererEnableScissor();
void rendererDisableScissor();
void rendererClear(float r, float g, float b, float a);

FBO* rendererCreateFBO(Game* game, int width, int height);
int rendererDestroyFBO(FBO* fbo);
void rendererEnableFBO(FBO* fbo);
void rendererDisableFBO();