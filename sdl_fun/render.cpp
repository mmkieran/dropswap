#include "render.h"
#include "stb_image.h"
#include <gl/GL.h>


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

void openglInit() {
   
   GLuint vao;
   glGenVertexArrays(1, &vao);  //Make a vertex array object... stores the links between attributes and vbos
   glBindVertexArray(vao);

   //disable the Z-buffer.  We don't want this, because we're doing a 2D engine.
   glDisable(GL_DEPTH_TEST);

   GLuint shaderProgram = createProgram();
   glUseProgram(shaderProgram);

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

   glBindFragDataLocation(shaderProgram, 0, "outColor");  //I'm a little confused about this

   glLinkProgram(shaderProgram);  //link program

   glDeleteShader(vertexShader);
   glDeleteShader(fragShader);

   return shaderProgram;
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

void destroyTexture(Texture* texture) {
   if (!texture) { return; }

   glDeleteTextures(1, &texture->handle);
   delete texture;
}

Texture* loadTextureFromFile(const char* filename) {
   int width, height;  //retrieve height and width of image
   int nChannels = 4; //get number of channels
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

struct Mesh {
   GLuint vbo;
   int ptCount;

   float* vertices;
};

Mesh* createMesh(int verts) {
   Mesh* mesh = new Mesh;
   mesh->vertices = (float*)malloc(sizeof(float)*verts);
   mesh->ptCount = verts;

   return mesh;
}

void destroyMesh(Mesh* mesh) {
   free(mesh->vertices);
   delete mesh;
}


Square* createSquare(Game* game) {

   Square* square = new Square;

   //Take the positions and texture coordinates and intertwine them into one array
   float vertices[24];

   //todo make this smarter if we have more than 2 attributes
   int i = 0;
   int j = 0;
   while (i < 12) {
      if (square->positions) {
         vertices[j] = square->positions[i];
         vertices[j + 1] = square->positions[i + 1];
      }
      if (square->texcoords) {
         vertices[j + 2] = square->texcoords[i];
         vertices[j + 3] = square->texcoords[i + 1];
      }

      i += 2;
      j += 4;
   }

   glGenBuffers(1, &square->vbo);

   glBindBuffer(GL_ARRAY_BUFFER, square->vbo);  //Make vbo active so we can copy the vertex data

   //copy data from vertices to buffer
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);  //point to position attribute
   glEnableVertexAttribArray(0);

   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));  //point to texture coords attribute
   glEnableVertexAttribArray(1);

   //square->texture = game->textures[1];  //todo put loading a texture in a texture manager
   //glBindTexture(GL_TEXTURE_2D, square->texture->handle);

   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it

   return square;
};

void bindTexture(Square* square) {
   glBindBuffer(GL_ARRAY_BUFFER, square->vbo);
   glBindTexture(GL_TEXTURE_2D, square->texture->handle);
   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it
}

void drawSquare(Square* square) {

   //Take the positions and texture coordinates and intertwine them into one array
   float vertices[24];

   //todo make this smarter if we have more than 2 attributes
   int i = 0;
   int j = 0;
   while (i < 12) {
      if (square->positions) {
         vertices[j] = square->positions[i];
         vertices[j + 1] = square->positions[i + 1];
      }
      if (square->texcoords) {
         vertices[j + 2] = square->texcoords[i];
         vertices[j + 3] = square->texcoords[i + 1];
      }

      i += 2;
      j += 4;
   }

   glBindBuffer(GL_ARRAY_BUFFER, square->vbo);

   //copy data from vertices to buffer
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   
   glDrawArrays(GL_TRIANGLES, 0, square->ptCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it
}

void destroySquare(Square* square) {
   delete square;
}

void clearRenderer(float r, float g, float b, float a) {
   glClearColor(r, g, b, a);
   glClear(GL_COLOR_BUFFER_BIT);
}

void copyToRenderer(Mesh* mesh) {
   glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);

   //copy data from vertices to buffer
   glBufferData(GL_ARRAY_BUFFER, sizeof(mesh->vertices), mesh->vertices, GL_STATIC_DRAW);

   glDrawArrays(GL_TRIANGLES, 0, mesh->ptCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);  //unbind it
}