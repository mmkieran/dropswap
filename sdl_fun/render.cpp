#include "game.h"

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


const char* vertexSource = R"glsl(
#version 150 core

in vec2 position; 

void main() {
   gl_Position = vec4(position, 0.0, 1.0);
}
)glsl";


const char* fragSource = R"glsl(
#version 150 core
out vec4 outColor;

void main() {
   outColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)glsl";


GLuint createVShader() {
   GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertexShader, 1, &vertexSource, NULL);

   glCompileShader(vertexShader);

   GLint status;
   glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);

   if (status == GL_TRUE) {
      return;
   }
   else {
      char buffer[512];
      glGetShaderInfoLog(vertexShader, 512, NULL, buffer);

      return vertexShader;
   }
}

GLuint createFShader() {
   GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);  //create object
   glShaderSource(fragShader, 1, &fragSource, NULL); //add source to it

   glCompileShader(fragShader);  //compile shader

   GLint status;
   glGetShaderiv(fragShader, GL_COMPILE_STATUS, &status);  //check if it compiled

   if (status == GL_TRUE) {
      return;
   }
   else {
      char buffer[512];
      glGetShaderInfoLog(fragShader, 512, NULL, buffer);

      return fragShader;
   }
}

void createProgram() {
   GLuint shaderProgram = glCreateProgram();
   glAttachShader(shaderProgram, vertexShader);
   glAttachShader(shaderProgram, fragShader);
}