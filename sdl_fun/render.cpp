#include "render.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "mymath.h"
#include "resources.h"
#include "board.h"

#include <gl/GL.h>
#include <SDL.h>
#include <math.h>

struct Texture {
   GLuint handle;
   int w, h;  //pixels
};

struct Mesh {
   GLuint vbo;  //vbo handle
   Texture* texture;
   TextureEnum type;

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


const char* vertexSource = R"glsl(
#version 150 core

in vec2 position; 
in vec2 texCoord;

out vec2 v_texCoord;

uniform mat4 transform;
uniform mat4 projection;
uniform mat4 deviceCoords;
uniform mat4 texMatrix;
uniform mat4 uCamera;

void main() {
   gl_Position = deviceCoords*uCamera*transform*projection*vec4(position, 0.0, 1.0);
   v_texCoord = (texMatrix*vec4(texCoord, 0.0, 1.0)).xy;
}
)glsl";


const char* fragSource = R"glsl(
#version 150 core

in vec2 v_texCoord;
uniform vec4 colorTrans;
out vec4 outColor;

uniform sampler2D myTexture;

void main() {
   //outColor = vec4(1.0, 0.5, 0.5, 1.0);
   outColor = texture(myTexture, v_texCoord) * colorTrans;
}
)glsl";

int openglContext() {

   //todo check what can go wrong here

   // Create graphics context
   //ImGui also uses these
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);


   // GL 3.0 + GLSL 130
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

   return 0;
}

GLuint vaoCreate() {
   //Make a vertex array object... stores the links between attributes and vbos
   GLuint vao;
   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);

   //disable the Z-buffer.  We don't want this, because we're doing a 2D engine.
   //todo Why does it only work when I put this here?
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);  //for images with alpha values
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  //setup blending function

   return vao;
}

void vaoDestroy(GLuint vao) {
   if (vao != 0) {
      glDeleteVertexArrays(1, &vao);
   }
}

GLuint shaderCreate(ShaderStage shaderStage) {
   GLuint shader;
   if (shaderStage == vertex_shader) {
      shader = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(shader, 1, &vertexSource, NULL);
   }
   else if (shaderStage == fragment_shader) {
      shader = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(shader, 1, &fragSource, NULL); //add source to it
   }

   glCompileShader(shader);

   GLint status;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

   if (status != GL_TRUE) {
      char buffer[512];
      glGetShaderInfoLog(shader, 512, NULL, buffer);
      printf("%s", buffer);
      //todo message this somehow
      glDeleteShader(shader);
      return 0;
   }

   return shader;
}

GLuint shaderProgramCreate() {

   GLuint shaderProgram;

   GLuint vertexShader = shaderCreate(vertex_shader);
   GLuint fragShader = shaderCreate(fragment_shader);

   shaderProgram = glCreateProgram();
   glAttachShader(shaderProgram, vertexShader);
   glAttachShader(shaderProgram, fragShader);

   glBindFragDataLocation(shaderProgram, 0, "outColor");  //todo I'm a little confused about this

   glLinkProgram(shaderProgram);  //link program

   glDeleteShader(vertexShader);
   glDeleteShader(fragShader);

   return shaderProgram;
}

void shaderUseProgram(GLuint program) {
   glUseProgram(program);
}

GLuint shaderGetUniform(GLuint shaderHandle, const char* uniformName) {
   GLuint uniform = glGetUniformLocation(shaderHandle, uniformName);
   return uniform;
}

void shaderSetMat4(GLuint location, float* mat) {
   //I used row major order apparently, false uses column major order... see wiki :(
   glUniformMatrix4fv(location, 1, GL_TRUE, mat);
}

void shaderSetVec4(GLuint location, float* vec4) {
   //I used row major order apparently, false uses column major order... see wiki :(
   glUniform4fv(location, 1, vec4);
}

void shaderSetMat4UniformByName(GLuint program, const char* name, float* mat) {
   GLuint location = shaderGetUniform(program, name);
   shaderSetMat4(location, mat);
}

void shaderSetVec4UniformByName(GLuint program, const char* name, float* vec4) {
   GLuint location = shaderGetUniform(program, name);
   shaderSetVec4(location, vec4);
}

void shaderDestroy(GLuint shader) {
   //glDetachShader(program, shader);
   glDeleteShader(shader);
}

void shaderDestroyProgram(GLuint program) {
   glDeleteProgram(program);
}

Texture* textureCreate(unsigned char* image, int width, int height) {
   Texture* texture = new Texture;

   texture->w = width;
   texture->h = height;

   //create handle and setup params
   glGenTextures(1, &texture->handle);  //create a texture object
   glBindTexture(GL_TEXTURE_2D, texture->handle);  //and in the darkness bind it
   if (!texture->handle) {
      printf("Couldn't create texture?");
      return nullptr;
   }

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  //textures repeat on S axis
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  //Repeat on T axis

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  //Linear interpolation instead of nearest pixel when magnify (blurs)
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  //same for shrink

   int internalFormat = GL_RGBA8;  //RGBA with one byte per component?
   int colorFormat = GL_RGBA;

   //type of texture, pixel components, width, height, pixel stuff, type of data, raw bytes
   glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texture->w, texture->h, 0, colorFormat, GL_UNSIGNED_BYTE, image);
   glBindTexture(GL_TEXTURE_2D, 0);  //unbind it

   return texture;
}

void textureChangeInterp(Mesh* mesh, bool nearest = false) {

   int flag = GL_LINEAR;
   if (nearest){ flag = GL_NEAREST; }

   glBindTexture(GL_TEXTURE_2D, mesh->texture->handle);  //and in the darkness bind it

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  //textures repeat on S axis
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  //Repeat on T axis

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, flag);  //Linear interpolation instead of nearest pixel when magnify (blurs)
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, flag);  //same for shrink

   glBindTexture(GL_TEXTURE_2D, 0);  //unbind it
}

void textureAttach(Mesh* mesh) {
   glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
   glBindTexture(GL_TEXTURE_2D, mesh->texture->handle);
   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it
}

void textureParams(Texture* texture, TextureWrap wrap) {
   if (!texture->handle) {return; }

   GLint wrapType;

   if (wrap == mirror) {
      wrapType = GL_MIRRORED_REPEAT;
   }
   else {
      wrapType = GL_REPEAT;
   }

   glBindTexture(GL_TEXTURE_2D, texture->handle);  //and in the darkness bind it

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapType);  //textures repeat on S axis
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapType);  //Repeat on T axis

   //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  //Linear interpolation instead of nearest pixel when magnify (blurs)
   //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  //same for shrink

   glBindTexture(GL_TEXTURE_2D, 0);  //unbind it
}

void textureDestroy(Texture* texture) {
   if (!texture) { return; }

   glDeleteTextures(1, &texture->handle);
   delete texture;
}

Texture* textureLoadFromFile(const char* filename) {
   int width, height;  //retrieve height and width of image
   int nChannels; //get number of channels
   int reqChannels = 4;  //required number of channels

   stbi_set_flip_vertically_on_load(true);  //my images are upside down :)
   unsigned char* image = stbi_load(filename, &width, &height, &nChannels, reqChannels);
   if (!image) {
      printf("Failed to load image: %s...\n", filename);
      return nullptr;
   }

   Texture* texture = textureCreate(image, width, height);  //create texture using image data

   stbi_image_free(image);

   return texture;
}

Mesh* meshCreate(Game* game) {

   Mesh* mesh = new Mesh;

   //Take the positions and texture coordinates and intertwine them into one array
   float vertices[24];

   //todo make this smarter if we have more than 2 attributes
   int i = 0;
   int j = 0;
   while (i < 12) {
      if (mesh->positions) {
         vertices[j] = mesh->positions[i];
         vertices[j + 1] = mesh->positions[i + 1];
      }
      if (mesh->texcoords) {
         vertices[j + 2] = mesh->texcoords[i];
         vertices[j + 3] = mesh->texcoords[i + 1];
      }

      i += 2;
      j += 4;
   }

   glGenBuffers(1, &mesh->vbo);

   glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);  //Make vbo active so we can copy the vertex data

   //copy data from vertices to buffer
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);  //point to position attribute
   glEnableVertexAttribArray(0);

   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));  //point to texture coords attribute
   glEnableVertexAttribArray(1);

   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it

   return mesh;
};

static void meshEffectDarken(Board* board, VisualEffect effect) {
   //todo add effect handling here
   float vec4[] = { 1.0, 1.0, 1.0, 1.0 };

   if (effect == visual_dark) {
      for (int i = 0; i < 4; i++) {
         vec4[i] = 0.5;
      }
   }
   shaderSetVec4UniformByName(resourcesGetShader(board->game), "colorTrans", vec4);
}

static void meshEffectDisplace(Board* board, VisualEffect effect, int effectTime) {
   //VisualEffect effect = visual_none;
   //int end = 0;
   //for (int i = 0; i < board->visualEvents.size(); i++) {
   //   VisualEvent e = board->visualEvents[i];
   //   if (e.end <= board->game->timer) { board->visualEvents.erase(board->visualEvents.begin() + i); }
   //   else if (e.effect == visual_swapl && e.end > board->game->timer) {
   //      effect = visual_swapl;
   //      end = e.end;
   //   }
   //   else if (e.effect == visual_swapr && e.end > board->game->timer) {
   //      effect = visual_swapr;
   //      end = e.end;
   //   }
   //}

   Mat4x4 mat = identityMatrix();
   if (effect == visual_swapr) {
      Vec2 move = { 0.0f , 0.0f };
      move.x -= board->tileWidth * (effectTime - board->game->timer)/ SWAPTIME;
      mat = transformMatrix(move, 0.0f, { 1, 1 });
   }
   else if (effect == visual_swapl) {
      Vec2 move = { 0.0f , 0.0f };
      move.x += board->tileWidth * (effectTime - board->game->timer) / SWAPTIME;
      mat = transformMatrix(move, 0.0f, { 1, 1 });
   }

   shaderSetMat4UniformByName(resourcesGetShader(board->game), "uCamera", mat.values);
}

static void meshEffectShake(Board* board) {
   VisualEffect effect = visual_none;
   for (int i = 0; i < board->visualEvents.size(); i++) {
      VisualEvent e = board->visualEvents[i];
      if (e.end <= board->game->timer) { board->visualEvents.erase(board->visualEvents.begin() + i); }
      else if (e.effect == visual_shake && e.end > board->game->timer) {
         effect = visual_shake;
      }
   }

   Mat4x4 mat = identityMatrix();

   if (effect == visual_shake) {
      Vec2 move = {0.0f , 0.0f};
      move.y += sin(board->game->timer);
      mat = transformMatrix(move, 0.0f, { 1, 1 });
   }

   shaderSetMat4UniformByName(resourcesGetShader(board->game), "uCamera", mat.values);
}

void meshDraw(Board* board, Mesh* mesh, float destX, float destY, int destW, int destH, VisualEffect effect, int effectTime) {

   //Vec2 scale = { destW / width, destH / height};
   Vec2 scale = { destW / board->game->windowWidth, destH / board->game->windowHeight};
   Vec2 dest = {round(destX) , round(destY)};  //todo rounding here feels bad for the vibration issue. Maybe a better place?
   
   Mat4x4 mat = transformMatrix(dest, 0.0f, scale);
   shaderSetMat4UniformByName(resourcesGetShader(board->game), "transform", mat.values);
   
   meshEffectDarken(board, effect);
   meshEffectShake(board);
   meshEffectDisplace(board, effect, effectTime);

   glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
   glBindTexture(GL_TEXTURE_2D, mesh->texture->handle);
   
   glDrawArrays(GL_TRIANGLES, 0, mesh->ptCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it
}

Animation* animationCreate(int frames, int delay, int stride, int rowStart, int width, int height, bool animated) {
   Animation* animation = new Animation;

   animation->animated = animated;
   animation->delay = delay;
   animation->frames = frames;
   animation->stride = stride;
   animation->rowStart = rowStart;
   animation->width = width;
   animation->height = height;
   animation->timer = 0;

   return animation;
}

void animationDraw(Board* board, Animation* animation, Mesh* mesh, float destX, float destY, int destW, int destH) {

   int currentFrame = (board->game->timer / animation->delay) % animation->frames;
   Vec2 src = { (animation->stride * currentFrame), animation->height };
   Vec2 size = {animation->width, animation->height};

   if (animation->animated == true) {
      textureTransform(board->game, mesh, src.x, src.y, size.x, size.y);
   }

   meshDraw(board, mesh, destX, destY, destW, destH);

   textureTransform(board->game, mesh, 0, 0, mesh->texture->w, mesh->texture->h);  //set the texture transform back
}

Animation* animationDestroy(Animation* animation) {
   if (animation) {
      delete animation;
   }
   return nullptr;
}

void textureTransform(Game* game, Mesh* mesh, float sourceX, float sourceY, int sourceW, int sourceH) {
   //This is for changing where the texture is sampled from the original image

   //todo this feels unnecessary, just figure out the math later, lol
   Mat4x4 projection = textureOriginToWorld(game, mesh->texture->w, mesh->texture->h);
   Mat4x4 device = worldToTextureCoords(game, mesh->texture->w, mesh->texture->h);

   Vec2 scale = { (float)sourceW / mesh->texture->w, (float)sourceH / mesh->texture->h };
   Vec2 dest = { sourceX, sourceY };

   Mat4x4 transform = transformMatrix(dest, 0.0f, scale);

   Mat4x4 intermediate = multiplyMatrix(transform, projection);
   Mat4x4 mat = multiplyMatrix(device, intermediate);

   shaderSetMat4UniformByName(resourcesGetShader(game), "texMatrix", mat.values);
}

TextureEnum meshGetTexture(Mesh* mesh) {
   if (mesh->texture) { return mesh->type; }
   else { return Texture_empty; }
}

void meshSetTexture(Game* game, Mesh* mesh, TextureEnum texture) {
   mesh->texture = resourcesGetTexture(game->resources, texture);
   mesh->type = texture;
}

Mesh* meshDestroy(Mesh* mesh) {
   if (mesh) {
      glDeleteBuffers(1, &mesh->vbo);
      delete mesh;
   }
   return nullptr;
}

void originToWorld(Game* game, float xOrigin, float yOrigin, float width, float height) {

   //device coordinates
   Vec2 botRight = { 1.0f, -1.0f };
   Vec2 topLeft = { -1.0f, 1.0f };

   //world coordinates
   Vec2 worldTopLeft = { xOrigin, yOrigin };
   Vec2 worldBotRight = { width, height };

   //I like to draw the meshes at the top left corner
   Vec2 movement = { (worldTopLeft.x - topLeft.x - 0.5f), (worldTopLeft.y - topLeft.y + 0.5f) };
   //Multiply by two because it's only a quarter of the screen... should i just change the vertices?
   Vec2 scale = { (worldTopLeft.x - worldBotRight.x) / (topLeft.x - botRight.x)*2.0f,  (worldTopLeft.y - worldBotRight.y) / (topLeft.y - botRight.y)*2.0f };

   Mat4x4 mMove = translateMatrix(movement);
   Mat4x4 mScale = scaleMatrix(scale);

   Mat4x4 mat = multiplyMatrix(mScale, mMove);

   shaderSetMat4UniformByName(resourcesGetShader(game), "projection", mat.values);
}

void worldToDevice(Game* game, float xOrigin, float yOrigin, float width, float height) {

   //device coordinates
   Vec2 botRight = { 1.0f, -1.0f };
   Vec2 topLeft = { -1.0f, 1.0f };

   //world coordinates
   Vec2 worldTopLeft = { xOrigin, yOrigin };
   Vec2 worldBotRight = { width, height };

   Vec2 movement = { (topLeft.x - worldTopLeft.x), (topLeft.y - worldTopLeft.y) };
   Vec2 scale = { (topLeft.x - botRight.x) / (worldTopLeft.x - worldBotRight.x), (topLeft.y - botRight.y) / (worldTopLeft.y - worldBotRight.y) };

   Mat4x4 mMove = translateMatrix(movement);
   Mat4x4 mScale = scaleMatrix(scale);

   Mat4x4 mat = multiplyMatrix(mMove, mScale);

   shaderSetMat4UniformByName(resourcesGetShader(game), "deviceCoords", mat.values);
}

Mat4x4 worldToTextureCoords(Game* game, float width, float height) {
   //texture coordinates
   Vec2 botRight = { 1.0f, 0.0f };
   Vec2 topLeft = { 0.0f, 1.0f };

   //world coordinates
   Vec2 worldTopLeft = { 0, 0 };
   Vec2 worldBotRight = { width, height };

   Vec2 movement = { (topLeft.x - worldTopLeft.x), (topLeft.y - worldTopLeft.y) };
   Vec2 scale = { (topLeft.x - botRight.x) / (worldTopLeft.x - worldBotRight.x), (topLeft.y - botRight.y) / (worldTopLeft.y - worldBotRight.y) };

   Mat4x4 mMove = translateMatrix(movement);
   Mat4x4 mScale = scaleMatrix(scale);

   Mat4x4 mat = multiplyMatrix(mMove, mScale);

   return mat;
}

Mat4x4 textureOriginToWorld(Game* game, float width, float height) {

   //device coordinates
   Vec2 botRight = { 1.0f, 0.0f };
   Vec2 topLeft = { 0.0f, 1.0f };

   //world coordinates
   Vec2 worldTopLeft = { 0, 0 };
   Vec2 worldBotRight = { width, height };

   //I like to draw the meshes at the top left corner
   Vec2 movement = { (worldTopLeft.x - topLeft.x), (worldTopLeft.y - topLeft.y) };
   Vec2 scale = { (worldTopLeft.x - worldBotRight.x) / (topLeft.x - botRight.x),  (worldTopLeft.y - worldBotRight.y) / (topLeft.y - botRight.y) };

   Mat4x4 mMove = translateMatrix(movement);
   Mat4x4 mScale = scaleMatrix(scale);

   Mat4x4 mat = multiplyMatrix(mScale, mMove);
   
   return mat;
}

void rendererClear(float r, float g, float b, float a) {
   glClearColor(r, g, b, a);
   glClear(GL_COLOR_BUFFER_BIT);
}

void rendererSetTarget(int originX, int originY, int width, int height) {
   //Set the top left corner of the viewerport rectangle
   glViewport(originX, originY, width, height);
}

//Copies the VBO data to the graphics card... only do this when we changes the vertices
void rendererCopyTo(Mesh* mesh) {
   glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);

   //copy data from vertices to buffer
   glBufferData(GL_ARRAY_BUFFER, sizeof(mesh->positions), mesh->positions, GL_STATIC_DRAW);

   glDrawArrays(GL_TRIANGLES, 0, mesh->ptCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it

}
