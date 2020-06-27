//this file is meant to provide a dual purpose:
//#1 - be a fully functional, usable OpenGL library
//#2 - act as a tutorial so it can be read from top-to-bottom to understand the process of setting up and using OpenGL, so that you can make your own extensions later.

//or wherever your GL3W is.
#include <imgui/GL/gl3w/gl3w.h>
#include "gl/gl.h"

#include <cstdlib> //need malloc and free
#include <string>
#include <vector>


//for texture loading.
#define STB_IMAGE_IMPLEMENTATION  //note: You define this macro in one file.  any other .cpp file that includes stb_image should not define this macro.
#include <stb_image.h>


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


//I've been using more namespaces lately personally...  anyway, opengl namespace!
namespace ogl
{

   //some typedefs to know what type of OpenGL object you're working with.  Just makes things easier to read.
   typedef GLuint VBOHandle; //VBO == vertex buffer object.  It's mesh data (triangle positions and texture coordinates).
   typedef GLuint UBOHandle;  //UBO = uniform buffer object.  It's data that gets loaded up to the GPU.  Camera data, for example.
   typedef GLuint TextureHandle;
   typedef GLuint ShaderHandle;

   //initialize function, call this first!
   void init()
   {
      //startup opengl...
      //at this point, you should already have a window created from SDL2.
      gl3wInit();  //startup gl3w, if you haven't already.

      //create a dummy VAO (vertex array object.)  This does absolutely nothing, but makes core contexts of OpenGL not have errors because having one created is required.
      GLuint vao;
      glGenVertexArrays(1, &vao);
      glBindVertexArray(vao);

      //disable the Z-buffer.  We don't want this, because we're doing a 2D engine.
      glDisable(GL_DEPTH_TEST);

   }

   //here's how you load up textures!
   struct Texture
   {
      TextureHandle handle;
      int xPixels, yPixels;
   };


   //create a texture from a raw chunk of bytes.
   Texture* textureCreate(void* image, int w, int h)
   {
      auto out = new Texture;

      int pixelSize = 4; //4 elements: R,G,B,A.  

      int pixelWidth = w * pixelSize;

      //ok - I flip my images because otherwise things are upside-down...  You may not need to do this.
      auto flipped = (unsigned char*)malloc(pixelWidth * h);

      //row by row copy.
      for (int i = 0; i < h; ++i)
      {
         memcpy(flipped + i * pixelWidth, (unsigned char*)image + (h - i - 1) * pixelWidth, pixelWidth);
      }

      TextureHandle tex;

      auto filter = GL_LINEAR;  //this will blur things if the texture size doesn't match the screen size.  GL_NEAREST disables filtering (blurring) and will just grab the nearest pixel.  Depends on how pixelated you want it to look.

      //create the texture handle and set it up!
      glGenTextures(1, &tex);
      glBindTexture(GL_TEXTURE_2D, tex);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  //assumes image bytes are sparsely packed.  Some bitmaps align pixels so that rows are all multiples of a certain number of bytes and dumb stuff.  Assume that's not the case.

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  //textures repeat.  GL_CLAMP_TO_EDGE if you want to clamp them.  Depends on what you want, but generally doesn't matter for simple cases
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);

      //the format of the texture.
      int internalFormat, colorFormat;
      internalFormat = GL_RGBA8;
      colorFormat = GL_RGBA;

      //load up the actual data.
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, colorFormat, GL_UNSIGNED_BYTE, flipped);
      glBindTexture(GL_TEXTURE_2D, 0);

      free(flipped);

      out->handle = tex;
      out->xPixels = w;
      out->yPixels = h;

      return out;
   }
   Texture* textureCreateFromFile(const char* path)
   {
      //use stb image to load this.
      int w, h;
      int comp = 4;  //kmm 8 bit components per pixel
      int reqComp = 4;  //kmm Requested components
      auto imageData = stbi_load(path, &w, &h, &comp, reqComp);

      //comp is the actual comps.

      if (!imageData)
      {
         //didn't load for whatever reason, oops.  Check your file path?
         return nullptr;
      }

      Texture* out = textureCreate(imageData, w, h);

      //loaded up into opengl now, so we can free the buffer.
      stbi_image_free(imageData);

      return out;
   }
   void textureDestroy(Texture* texture)
   {
      if (!texture) return;
      glDeleteTextures(1, &texture->handle);
      delete texture;
   }
   void textureBind(Texture* texture)
   {
      glBindTexture(GL_TEXTURE_2D, texture->handle);
   }

   //okay, let's load up a shader.


   //in order to use shaders, we'll have to set what data in C++ links up to certain variables in the shader.
   //this code lets us create a mapping table to do a lot of that work automatically.
   struct VertexAttribute
   {
      enum Type
      {
         Position, TexCoord,
         COUNT
      };

      Type idx;
      const char* name;
      size_t size;
   };


   typedef VertexAttribute::Type e_VertexAttribute;

   //our actual mapping table.  
   static const VertexAttribute _attributeTable[]
   {
      {VertexAttribute::Position, "position", sizeof(Vec2)},
      {VertexAttribute::TexCoord, "texcoord", sizeof(Vec2)}
   };

   //we have vertex shaders and fragment (pixel) shaders.  These get linked into a single final shader object.
   struct ShaderProgram
   {
      enum e_ShaderProgram
      {
         Vertex, Fragment, COUNT
      };
   };

   typedef ShaderProgram::e_ShaderProgram e_ShaderProgram;

   //wrapper around a single stage in the shader as we make them.
   struct ShaderStage
   {
      GLuint handle;
   };


   static int _convertShaderTypeToOpenGL(e_ShaderProgram type)
   {
      switch (type)
      {
      case ShaderProgram::Vertex: return GL_VERTEX_SHADER;
      case ShaderProgram::Fragment: return GL_FRAGMENT_SHADER;
      }

      //?!  uhh...
      return 0;
   }


   //low-level wrapper for 
   static GLuint _compileProgram(const char* code, int shaderType)
   {
      //create and compile a shader stage object.
      auto shader = glCreateShader(shaderType);

      auto str = code;
      glShaderSource(shader, 1, (const GLchar* const*)&str, 0);
      glCompileShader(shader);

      //make sure it actually worked!
      int status;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
      if (status == GL_FALSE)
      {
         //if we're in here, the shader did not compile.  Figure out why.
         GLint maxLength = 0;
         glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

         // The maxLength includes the NULL character
         std::string errorLog(maxLength, ' ');
         glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

         //note: do something here with this error, like the message box call below, you'll want it...


         //MessageBoxA(NULL, errorLog.c_str(), "shader fail", MB_OK);
         //assert(false && "Can't compile?");

         glDeleteShader(shader);
         return 0;
      }

      //return that shader.
      return shader;
   }

   //create a single shader stage from text.
   ShaderStage* shaderStageCreate(e_ShaderProgram shaderStage, const char* shaderText)
   {
      auto out = new ShaderStage;

      out->handle = _compileProgram(shaderText, _convertShaderTypeToOpenGL(shaderStage));
      if (!out->handle)
      {
         delete out;
         return out;
      }

      return out;
   }
   //clean up a shader stage.
   void shaderStageDestroy(ShaderStage* stage)
   {
      if (!stage) return;

      glDeleteShader(stage->handle);
      delete stage;
   }

   //an actual shader object!
   struct Shader
   {
      ShaderHandle handle;
   };

   static void _linkShaderToBuffers(Shader* shader); //will do this later.
   Shader* shaderCreate(ShaderStage* vertex, ShaderStage* fragment)  //requires a vertex and a fragment stage.
   {
      if (!vertex || !fragment) return NULL;

      auto out = new Shader;
      auto prg = glCreateProgram();

      //set up all the attribute mappings, so our positions and texture coordinages go into the shader.
      for (size_t idx = 0; idx < VertexAttribute::COUNT; ++idx)
      {
         glBindAttribLocation(prg, idx, _attributeTable[idx].name);
      }

      //attach the stages...
      glAttachShader(prg, vertex->handle);
      glAttachShader(prg, fragment->handle);
      glLinkProgram(prg); //link them together, we have a shader!  (if this worked.)

      //check if it worked, and give a message if it failed.
      int status;
      glGetProgramiv(prg, GL_LINK_STATUS, &status);
      if (status == GL_FALSE)
      {
         GLint maxLength = 0;
         glGetProgramiv(prg, GL_INFO_LOG_LENGTH, &maxLength);

         // The maxLength includes the NULL character
         std::string errorLog(maxLength, ' ');
         glGetProgramInfoLog(prg, maxLength, &maxLength, &errorLog[0]);

         //show this error string somehow here!

         //MessageBoxA(NULL, errorLog.c_str(), "link fail", MB_OK);
         //assert(false && "Can't compile?");
         glDeleteProgram(prg);
         return 0;
      }


      out->handle = prg;

      //link this up to buffers.  Not
      _linkShaderToBuffers(out);


      return out;
   }
   void shaderDestroy(Shader* shader)
   {
      delete shader;
   }


   //link to a single shader value.
   typedef uint32_t ShaderDataSlot;

   //bind a shader as the current active shader.
   void shaderBind(Shader* shader)
   {
      static Shader* lastShader = nullptr;
      if (shader == lastShader) return;
      lastShader = shader;

      //if we pass in null, disable all shaders.
      if (!shader)
      {
         glUseProgram(0);
         return;
      }

      //otherwise, set the linked fully created shader from earlier.
      glUseProgram(shader->handle);
   }


   //shader setters
   ShaderDataSlot shaderGetUniform(Shader* shader, const char* uniformName)
   {
      //never cached, so don't be dumb
      return glGetUniformLocation(shader->handle, uniformName);
   }

   //wrappers to write values into a shader.
   void shaderSetTexture(ShaderDataSlot name, int textureSlot)
   {
      glUniform1i(name, textureSlot);
   }

   void shaderSetFloat(ShaderDataSlot name, float val)
   {
      glUniform1f(name, val);
   }
   void shaderSetFloat2(ShaderDataSlot name, float const*  val)
   {
      glUniform2fv(name, 1, val);
   }
   void shaderSetFloat3(ShaderDataSlot name, float const* val)
   {
      glUniform3fv(name, 1, val);
   }
   void shaderSetFloat4(ShaderDataSlot name, float const* val)
   {
      glUniform4fv(name, 1, val);
   }
   void shaderSetMat4(ShaderDataSlot name, float const* matrix)
   {
      glUniformMatrix4fv(name, 1, GL_FALSE, matrix);
   }

   struct ShaderDataMapping
   {
      std::string name;
      size_t sz, baseSz;
      void* buffer;
      UBOHandle handle;
   };

   static std::vector<ShaderDataMapping*> dataMappings;

   //data mappings. (UBOs.)
   ShaderDataMapping* shaderDataMappingCreate(const char* name, size_t sz)
   {
      auto out = new ShaderDataMapping;

      out->name = name;
      out->baseSz = sz;

      //these mappings have to be multiples of 4 floats...
      if (sz % (sizeof(float) * 4))
      {
         sz += (sizeof(float) * 4) - (sz % (sizeof(float) * 4));
      }

      out->sz = sz;
      out->buffer = malloc(out->sz);
      memset(out->buffer, 0, out->sz);

      glGenBuffers(1, &out->handle);
      glBindBuffer(GL_UNIFORM_BUFFER, out->handle);
      glBufferData(GL_UNIFORM_BUFFER, sz, 0, GL_STREAM_DRAW); //this is going to be updated a lot!  So we set the lifetime to STREAM_DRAW.
      glBindBuffer(GL_UNIFORM_BUFFER, 0);

      glBindBufferBase(GL_UNIFORM_BUFFER, (GLuint)dataMappings.size(), out->handle);

      dataMappings.push_back(out);

      return out;
   }
   void shaderDataMappingDestroy(ShaderDataMapping* mapping)
   {
      if (!mapping) return;

      //remove this from the mappings array.
      auto it = std::find(begin(dataMappings), end(dataMappings), mapping);
      if (it != end(dataMappings)) dataMappings.erase(it);

      free(mapping->buffer);

      delete mapping;
   }

   //update a mapping...
   void shaderDataMappingSet(ShaderDataMapping* mapping, void const * data)
   {
      memcpy(mapping->buffer, data, mapping->baseSz);
      glBindBuffer(GL_UNIFORM_BUFFER, mapping->handle);

      auto ptr = glMapBufferRange(GL_UNIFORM_BUFFER, 0, mapping->sz, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
      memcpy(ptr, mapping->buffer, mapping->sz);
      glUnmapBuffer(GL_UNIFORM_BUFFER);

      glBindBuffer(GL_UNIFORM_BUFFER, 0);
   }

   void _linkShaderToBuffers(Shader* shader)
   {
      //make a given shader use the data mappings.
      int slot = -1;
      for (auto&& ubo : dataMappings)
      {
         ++slot;
         auto idx = glGetUniformBlockIndex(shader->handle, ubo->name.c_str());
         if (idx != GL_INVALID_INDEX)
         {
            glUniformBlockBinding(shader->handle, idx, slot);
         }
      }
   }

   //okay, we have this architecture, but how about a basic example?

   struct CameraDefault
   {
      Mat4x4 viewMatrix;
   };

   //a basic camera data mapping
   ShaderDataMapping* shaderDataMappingCreateDefault()
   {
      auto buffer = shaderDataMappingCreate("uCamera", sizeof(CameraDefault));
      return buffer;
   }

   //update the camera with new values.
   void shaderDataMappingCreateDefaultUpdate(ShaderDataMapping* mapping, CameraDefault* camera)
   {
      shaderDataMappingSet(mapping, (void*)camera);
   }

   //a basic shader, that draws a transformed textured 2D mesh.  Assumes you already made the default mapping.
   Shader* shaderCreateDefault()
   {

      static const char* defaultVertexShader =

         R"(#version 140

//our mesh data.
in vec2 position;
in vec2 texcoord;

//camera!  in a UBO.  This is our default mapping, that gets read in here.
layout(std140) uniform uCamera {
   uniform mat4 uViewMatrix;
};

//vertex transform - use shaderSetMat4 to set this.
uniform mat4 uModelMatrix;
//texture transform - use shaderSetMat4 to set this.
uniform mat4 uTextureMatrix;

out vec2 v_texcoord;

void main() {
   v_texcoord = (uTextureMatrix*vec4(texcoord, 0.0, 1.0)).xy;
   gl_Position = uViewMatrix*(uModelMatrix*vec4(position, 0.0, 1.0));
}

)";


      static const char* defaultFragmentShader =

         R"(#version 140

uniform sampler2D uDiffuse; //our texture handle.  this is which slot it's bound to.  Just always set this to zero with shaderSetTexture.
in vec2 v_texcoord; //our texture coord

out vec4 outputColor;  //the color we output to the screen! :D

void main() {
   outputColor = texture(uDiffuse, v_texcoord);  //get the texture color at the current texture coordinate.
}
)";

      auto vertStage = shaderStageCreate(ShaderProgram::Vertex, defaultVertexShader);
      auto fragStage = shaderStageCreate(ShaderProgram::Fragment, defaultFragmentShader);

      //make the actual shader!
      auto shader = shaderCreate(vertStage, fragStage);

      //can clean up the stages now.
      shaderStageDestroy(vertStage);
      shaderStageDestroy(fragStage);

      return shader;
   }



   //a mesh struct for holding the raw data textured triangles.  We'll use this to render squares later.
   struct MeshDefinition
   {
      size_t vertexCount = 0;
      float const* positions2D = nullptr;  //vertexCount * 2
      float const* textureUV = nullptr; //vertexCount * 2
   };



   //a mesh that's been compiled into an OpenGL VBO - it's ready to draw!
   struct Mesh
   {
      bool attributes[VertexAttribute::COUNT] = { 0 };
      VBOHandle vbo;
      int pointCount, stride;
   };


   static MeshDefinition _createSquare(); //create a square!  
   Mesh* meshCreate(MeshDefinition const* mesh);

   static void _bindVBO(Mesh* geom)
   {
      static Mesh* lastDrawn = nullptr;  //only update this if this mesh is different from the last mesh.

      if (geom == lastDrawn) return;
      lastDrawn = geom;

      //actual bind code:   this sets up the VBO to be drawn.
      int current = 0;
      int offset = 0;
      glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
      for (int i = 0; i < VertexAttribute::COUNT; ++i)
      {
         if (geom->attributes[i])
         {
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, _attributeTable[i].size / sizeof(float), GL_FLOAT, GL_FALSE, geom->stride, (void*)offset);
            offset += _attributeTable[i].size;
         }
         else
         {
            glDisableVertexAttribArray(i);
         }
      }
   }


   void squareDraw(Vec2 position, Vec2 size, Texture* texture)
   {
      //create this square mesh only once.
      static MeshDefinition squareData = _createSquare();
      static Mesh* squareMesh = meshCreate(&squareData);

      //bind the VBO...
      _bindVBO(squareMesh);

      //bind the texture...
      textureBind(texture);

      //now draw it! :D
      glDrawArrays(GL_TRIANGLES, 0, squareMesh->pointCount);
   }


   MeshDefinition _createSquare()
   {
      //create a square that's centered around 0 - it'll make it easier for us to draw it later.
      MeshDefinition mesh;

      static float positions[] =
      {
         -0.5f, 0.5f,
         -0.5f, -0.5f,
         0.5f, -0.5f,

         0.5f, 0.5f,
         0.5f, -0.5f,
         -0.5f, 0.5f,
      };


      static float texcoords[] =
      {
         0.0f, 1.0f,
         0.0f, 0.0f,
         1.0f, 0.0f,

         1.0f, 1.0f,
         1.0f, 0.0f,
         0.0f, 1.0f,
      };

      mesh.vertexCount = 6;
      mesh.positions2D = positions;
      mesh.textureUV = texcoords;

      return mesh;
   }
   Mesh* meshCreate(MeshDefinition const* mesh)
   {
      auto out = new Mesh;

      //interleave the data...
      size_t byteCount = 0;
      size_t stride = 0;

      //we're going to copy the multiple pieces of data (texture coords and positions) into a single buffer.

      //figure out our total number of bytes, and the number of bytes per vertex (our stride.)
      if (mesh->positions2D)
      {
         byteCount += mesh->vertexCount * sizeof(float) * 2;
         stride += sizeof(float) * 2;
      }

      if (mesh->textureUV)
      {
         byteCount += mesh->vertexCount * sizeof(float) * 2;
         stride += sizeof(float) * 2;
      }

      //make the buffer...
      auto buffer = malloc(byteCount);
      auto cursor = (char*)buffer;

      //write all vertices into the buffer.
      for (size_t i = 0; i < mesh->vertexCount; ++i)
      {
         if (mesh->positions2D)
         {
            memcpy(cursor, &mesh->positions2D[i * 2], sizeof(float) * 2);
            cursor += sizeof(float) * 2;
         }
         if (mesh->textureUV)
         {
            memcpy(cursor, &mesh->textureUV[i * 2], sizeof(float) * 2);
            cursor += sizeof(float) * 2;
         }
      }

      //send it over to opengl.
      out->stride = stride;
      out->pointCount = mesh->vertexCount;

      out->attributes[VertexAttribute::Position] = mesh->positions2D != nullptr;
      out->attributes[VertexAttribute::TexCoord] = mesh->textureUV != nullptr;

      glGenBuffers(1, &out->vbo);

      glBindBuffer(GL_ARRAY_BUFFER, out->vbo);
      glBufferData(GL_ARRAY_BUFFER, byteCount, buffer, GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, 0);

      free(buffer);
      return out;
   }

   //a couple matrix helpers.

   //make a camera matrix.
   Mat4x4 camera2DMatrix(float left, float right, float bottom, float top)
   {

      float farVal = 1.0f; //these Z values don't really matter in 2D.
      float nearVal = -1.0f;

      float tx = -(right + left) / (right - left);
      float ty = -(top + bottom) / (top - bottom);
      float tz = -(farVal + nearVal) / (farVal - nearVal);

      Mat4x4 matrix;
      auto out = matrix.values;

      out[0] = 2 / (right - left);
      out[1] = 0.0;
      out[2] = 0.0;
      out[3] = 0.0;

      out[4] = 0.0;
      out[5] = 2 / (top - bottom);
      out[6] = 0.0;
      out[7] = 0.0;

      out[8] = 0.0;
      out[9] = 0.0;
      out[10] = -2 / (farVal - nearVal);
      out[11] = 0.0;

      out[12] = tx;
      out[13] = ty;
      out[14] = tz;
      out[15] = 1.0;

      return matrix;
   }

   //make an identity matrix (probably what you want to always set to the texture matrix...)
   Mat4x4 identityMatrix()
   {
      Mat4x4 out;
      for (int i = 0, idx = 0; i < 4; ++i)
      {
         for (int j = 0; j < 4; ++j, ++idx)
         {
            out.values[idx] = (i == j) ? 1 : 0;
         }
      }
      return out;
   }

   Mat4x4 rotationMatrix(Vec2 direction)
   {
      auto out = identityMatrix();
      out.values[0] = direction.x;
      out.values[1] = direction.y;
      out.values[4] = -direction.y;
      out.values[5] = direction.x;
      return out;
   }

   Mat4x4 rotationMatrix(float angleInRadians)
   {
      return rotationMatrix({ cosf(angleInRadians), sinf(angleInRadians) });
   }

   Mat4x4 translationMatrix(Vec2 position)
   {
      auto out = identityMatrix();
      out.values[12] = position.x;
      out.values[13] = position.y;
      return out;
   }

   Mat4x4 scaleMatrix(Vec2 scale)
   {
      auto out = identityMatrix();
      out.values[0] = scale.x;
      out.values[5] = scale.y;
      return out;
   }

   Mat4x4 matrixMultiply(Mat4x4 const& left, Mat4x4 const& right)
   {
      Mat4x4 out = { 0 };

      for (int i = 0; i < 4; ++i) {
         for (int j = 0; j < 4; ++j) {
            float sum = 0;
            for (int k = 0; k < 4; ++k) {
               sum = sum + left.values[i + k * 4] * right.values[k + j * 4];
            }
            out.values[i + j * 4] = sum;
         }
      }
      return out;
   }

   //one call that does everything!
   Mat4x4 rigidBodyTransform(Vec2 position, Vec2 scale, float rotationAngle)
   {
      //this effectively applies backwards, so we do translate, then rotation, then scale.
      //that applies by doing the scale, then rotating, then translating, which will work as you'd expect.
      auto firstPart = matrixMultiply(translationMatrix(position), rotationMatrix(rotationAngle));
      return matrixMultiply(firstPart, scaleMatrix(scale));
   }

}