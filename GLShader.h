#ifndef __GLSHADER_H__
#define __GLSHADER_H__

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include "GLError.h"

#include <iostream>
#include <fstream>
#include <strings.h>

using namespace std;

enum GLSHADER_ERROR_CODES {
  GLSHADER_NO_ERROR,
  GLSHADER_FILE_READ_ERROR,
  GLSHADER_COMPILE_ERROR,
  GLSHADER_LINK_ERROR
};
static int GLSHADERerrno = GLSHADER_NO_ERROR;


class GLShader {
public:
  GLShader() :
    sourceLoaded(false),
    compiled(false)
  {};
  virtual ~GLShader() {};
  
  virtual bool loadFromFile(const char *filename);
  virtual bool compile();

  GLuint handle;

  GLint sourceLoaded;
  GLint compiled;
  int  errCode;
};

class GLVertexShader : public GLShader {
public:
  GLVertexShader() {};
  GLVertexShader(const char *filename);
  virtual ~GLVertexShader() {};

  virtual bool loadFromFile(const char *filename);
};

class GLFragmentShader : public GLShader {
public:
  GLFragmentShader() {};
  GLFragmentShader(const char *filename);
  virtual ~GLFragmentShader() {};

  virtual bool loadFromFile(const char *filename);
};

class GLShaderProgram {
public:
  GLShaderProgram() :
    handle(0),
    linked(false)
  {};
  GLShaderProgram(GLVertexShader &_vertShader, GLFragmentShader &_fragShader);
  void setUniform(string name, GLfloat *val, int count);
  void setUniform(string name, GLint *val, int count);
  void setTextureUnit(string texname, int texunit);
  void bindTexture(GLenum target, string texname, GLuint texid, int texunit);
  void bindTexture2D(string texname, GLuint texid, int texunit) {
      bindTexture(GL_TEXTURE_2D, texname, texid, texunit);
  };
  void bindTexture3D(string texname, GLuint texid, int texunit) {
      bindTexture(GL_TEXTURE_3D, texname, texid, texunit);
  };
  void bindTextureRECT(string texname, GLuint texid, int texunit) {
      bindTexture(GL_TEXTURE_RECTANGLE, texname, texid, texunit);
  };
  virtual ~GLShaderProgram() {};

  virtual bool link();
  virtual inline void activate();

  GLuint handle;

  GLVertexShader   vertShader;
  GLFragmentShader fragShader;

  GLint linked;
};

#endif
