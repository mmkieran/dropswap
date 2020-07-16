#include "render.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "mymath.h"
#include "resources.h"

#include <gl/GL.h>
#include <SDL.h>
#include <math.h>


const char* vertexSource = R"glsl(
#version 150 core

in vec2 position; 
in vec2 texCoord;

out vec2 v_texCoord;

uniform mat4 transform;
uniform mat4 projection;
uniform mat4 deviceCoords;

void main() {
   gl_Position = deviceCoords*transform*projection*vec4(position, 0.0, 1.0);
   v_texCoord = texCoord;
}
)glsl";


const char* fragSource = R"glsl(
#version 150 core

in vec2 v_texCoord;

out vec4 outColor;

uniform sampler2D myTexture;

void main() {
   //outColor = vec4(1.0, 0.5, 0.5, 1.0);
   outColor = texture(myTexture, v_texCoord);
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

GLuint createVAO() {
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

void destroyVAO(GLuint vao) {
   if (vao != 0) {
      glDeleteVertexArrays(1, &vao);
   }
}

GLuint createShader(ShaderStage shaderStage) {
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

GLuint createProgram() {

   GLuint shaderProgram;

   GLuint vertexShader = createShader(vertex_shader);
   GLuint fragShader = createShader(fragment_shader);

   shaderProgram = glCreateProgram();
   glAttachShader(shaderProgram, vertexShader);
   glAttachShader(shaderProgram, fragShader);

   glBindFragDataLocation(shaderProgram, 0, "outColor");  //todo I'm a little confused about this

   glLinkProgram(shaderProgram);  //link program

   glDeleteShader(vertexShader);
   glDeleteShader(fragShader);

   return shaderProgram;
}

void useShaderProgram(GLuint program) {
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

void shaderSetMat4UniformByName(GLuint program, const char* name, float* mat) {
   GLuint location = shaderGetUniform(program, name);
   shaderSetMat4(location, mat);
}

void destroyShaders(GLuint shader) {
   //glDetachShader(program, shader);
   glDeleteShader(shader);
}

void destroyProgram(GLuint program) {
   glDeleteProgram(program);
}


Texture* createTexture(unsigned char* image, int width, int height) {
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

void changeTexParams(Texture* texture, TextureWrap wrap) {
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

void destroyTexture(Texture* texture) {
   if (!texture) { return; }

   glDeleteTextures(1, &texture->handle);
   delete texture;
}

Texture* loadTextureFromFile(const char* filename) {
   int width, height;  //retrieve height and width of image
   int nChannels; //get number of channels
   int reqChannels = 4;  //required number of channels

   stbi_set_flip_vertically_on_load(true);  //my images are upside down :)
   unsigned char* image = stbi_load(filename, &width, &height, &nChannels, reqChannels);
   if (!image) {
      printf("Failed to load image: %s...\n", filename);
      return nullptr;
   }

   Texture* texture = createTexture(image, width, height);  //create texture using image data

   stbi_image_free(image);

   return texture;
}

Mesh* createMesh(Game* game) {

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

void bindTexture(Mesh* mesh) {
   glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
   glBindTexture(GL_TEXTURE_2D, mesh->texture->handle);
   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it
}

void drawMesh(Game* game, Mesh* mesh, float destX, float destY, int destW, int destH) {
   
   int width, height;
   SDL_GetWindowSize(game->window, &width, &height);

   //Vec2 scale = { destW / width, destH / height};
   Vec2 scale = { destW / game->windowWidth, destH / game->windowHeight};
   Vec2 dest = {round(destX) , round(destY)};  //todo rounding here feels bad for the vibration issue. Maybe a better place?
   
   Mat4x4 mat = transformMatrix(dest, 0.0f, scale);

   shaderSetMat4UniformByName(resourcesGetShader(game), "transform", mat.values);

   glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
   glBindTexture(GL_TEXTURE_2D, mesh->texture->handle);
   
   glDrawArrays(GL_TRIANGLES, 0, mesh->ptCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it
}

void destroyMesh(Mesh* mesh) {
   glDeleteBuffers(1, &mesh->vbo);
   delete mesh;
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

void clearRenderer(float r, float g, float b, float a) {
   glClearColor(r, g, b, a);
   glClear(GL_COLOR_BUFFER_BIT);
}

void setRenderTarget(int originX, int originY, int width, int height) {
   //Set the top left corner of the viewerport rectangle
   glViewport(originX, originY, width, height);
}

//Copies the VBO data to the graphics card... only do this when we changes the vertices
void copyToRenderer(Mesh* mesh) {
   glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);

   //copy data from vertices to buffer
   glBufferData(GL_ARRAY_BUFFER, sizeof(mesh->positions), mesh->positions, GL_STATIC_DRAW);

   glDrawArrays(GL_TRIANGLES, 0, mesh->ptCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it

}
