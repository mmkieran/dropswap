#include "render.h"
#include "stb_image.h"

//I really recommend having your own struct that looks like this!
struct Vec2
{
   float x, y;
};


//4x4 matrix - a standard for graphics.
struct Mat4x4
{
   float values[16];
};


enum ShaderStage {
   fragment_shader,
   vertex_shader
};


const char* vertexSource = R"glsl(
#version 150 core

in vec2 position; 
in vec2 texCoord;

out vec2 v_texCoord;

void main() {
   gl_Position = vec4(position, 0.0, 1.0);
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

   glBindFragDataLocation(shaderProgram, 0, "outColor");  //I'm a little confused about this

   glLinkProgram(shaderProgram);  //link program

   glDeleteShader(vertexShader);
   glDeleteShader(fragShader);

   return shaderProgram;
}

void deleteShaders(GLuint shader) {
   //glDetachShader(program, shader);
   glDeleteShader(shader);
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

void destroyTexture(Texture* texture) {
   if (!texture) { return; }

   glDeleteTextures(1, &texture->handle);
   delete texture;
}

Texture* loadTextureFromFile(const char* filename) {
   int width, height;  //retrieve height and width of image
   int nChannels = 4; //get number of channels
   int reqChannels = 4;  //required number of channels

   unsigned char* image = stbi_load(filename, &width, &height, &nChannels, reqChannels);
   if (!image) {
      printf("Failed to load image: %s...\n", filename);
      return nullptr;
   }

   printf("%d...\n", nChannels);
   Texture* texture = createTexture(image, width, height);

   stbi_image_free(image);

   return texture;
}



//for later
float vertices[] =
{
   -0.5f, 0.5f,
   -0.5f, -0.5f,
   0.5f, -0.5f,

   0.5f, 0.5f,
   0.5f, -0.5f,
   -0.5f, 0.5f,
};

float texcoords[] =
{
   0.0f, 1.0f,
   0.0f, 0.0f,
   1.0f, 0.0f,

   1.0f, 1.0f,
   1.0f, 0.0f,
   0.0f, 1.0f,
};