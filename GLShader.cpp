#include "GLShader.h"

bool GLShader::loadFromFile(const char *filename)
{
  if (!filename) {
    this->sourceLoaded = false;
    GLSHADERerrno = GLSHADER_FILE_READ_ERROR;
    cerr << "GLShader Error: no filename specified" << endl;
    return 0;
  }

  fstream file(filename, fstream::in);
  file.seekg(0, ios::end);
  int fileLength = file.tellg();
  file.seekg(0, ios::beg);

  GLchar *src = new char[fileLength+1];
  bzero(src, fileLength+1);
  file.read(src, fileLength);
  file.close();
  
  glShaderSource(handle, 1, (const GLchar **)(&src), NULL);
  printOpenGLError();
  
  this->sourceLoaded = true;
  return 1;
}

bool GLShader::compile()
{
  glCompileShader(handle);
  compiled = true;
  printOpenGLError();
  glGetShaderiv(handle, GL_COMPILE_STATUS, &compiled);
  printShaderInfoLog(handle);


  if (!compiled) {
	  GLSHADERerrno = GLSHADER_COMPILE_ERROR;
	  cerr << "GLShader Error: could not compile" << endl;
  }
  return compiled;
}

GLVertexShader::GLVertexShader(const char *filename)
{
  loadFromFile(filename);
  compile();
}

bool GLVertexShader::loadFromFile(const char *filename)
{
  handle = glCreateShader(GL_VERTEX_SHADER);

  return GLShader::loadFromFile(filename);
}

GLFragmentShader::GLFragmentShader(const char *filename)
{
  loadFromFile(filename);
  compile();
}

bool GLFragmentShader::loadFromFile(const char *filename)
{
  handle = glCreateShader(GL_FRAGMENT_SHADER);

  return GLShader::loadFromFile(filename);
}

GLShaderProgram::GLShaderProgram(GLVertexShader &_vertShader, GLFragmentShader &_fragShader)
{
  this->vertShader = _vertShader;
  this->fragShader = _fragShader;

  handle = glCreateProgram();
  if (handle == 0) {
	  cerr << "GLShaderProgram could not create program handle." << endl;
	  printOpenGLError();
  }
  glAttachShader(this->handle, vertShader.handle);
  glAttachShader(this->handle, fragShader.handle);

  this->linked = false;
  if (link()) {
    activate();
    this->linked = true;
  } else {
	  cerr << "Cannot link GLShaderProgram." << endl;
  }
}

bool GLShaderProgram::link()
{
  glLinkProgram(handle);
  printOpenGLError();
  glGetProgramiv(handle, GL_LINK_STATUS, &linked);
  printProgramInfoLog(handle);
  printOpenGLError();
  
  if (linked != GL_TRUE) {
    GLSHADERerrno = GLSHADER_LINK_ERROR;
    cerr << "GLShaderProgram::link error for program handle: " << handle << endl;
    return false;
  }

  this->activate();

  return true;
}

inline void GLShaderProgram::activate()
{
  glUseProgram(handle);
}

void GLShaderProgram::setUniform(string name, GLfloat* val, int count)
{
	GLint id = glGetUniformLocation(handle, name.c_str());
	if (id == -1) {
		cerr << "Warning: Invalid uniform parameter " << name << endl;
		return;
	}
	switch (count) {
		case 1:
			glUniform1fv(id, 1, val);
			break;
		case 2:
			glUniform2fv(id, 1, val);
			break;
		case 3:
			glUniform3fv(id, 1, val);
			break;
		case 4:
			glUniform4fv(id, 1, val);
			break;
	}
}

void GLShaderProgram::setUniform(string name, GLint* val, int count)
{
	GLint id = glGetUniformLocation(handle, name.c_str());
	if (id == -1) {
		cerr << "Warning: Invalid uniform parameter " << name << endl;
		return;
	}
	switch (count) {
		case 1:
			glUniform1iv(id, 1, val);
			break;
		case 2:
			glUniform2iv(id, 1, val);
			break;
		case 3:
			glUniform3iv(id, 1, val);
			break;
		case 4:
			glUniform4iv(id, 1, val);
			break;
	}
}

void GLShaderProgram::setTextureUnit(std::string texname, int texunit)
{
	GLint linked;
	glGetProgramiv(handle, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE) {
		cerr << "Error: setTextureUnit needs program to be linked." << endl;
		exit(1);
	}
	GLint id = glGetUniformLocation(handle, texname.c_str());
	if (id == -1) {
		std::cerr << "Warning: Invalid texture " << texname << std::endl;
		return;
	}
	glUniform1i(id, texunit);
}

void GLShaderProgram::bindTexture(GLenum target, std::string texname, GLuint texid, int texunit)
{
	glActiveTexture(GL_TEXTURE0 + texunit);
	glBindTexture(target, texid);
	setTextureUnit(texname, texunit);
	glActiveTexture(GL_TEXTURE0);
}

