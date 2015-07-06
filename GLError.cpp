#include "GLError.h"

void printOpenGLError()
{
  GLenum errCode;
  const GLubyte *errString;

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    errString = gluErrorString(errCode);
    cerr << "OpenGL Error: " << errString << endl;
  }
}

void printShaderInfoLog(GLuint shader)
{
  int infologLen   = 0;
  int charsWritten = 0;
  GLchar *infoLog;

  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
  
  if (infologLen > 1) {
    infoLog = new GLchar[infologLen];
    if (infoLog == NULL) {
      cerr << "Error: Could not allocate infoLog buffer" << endl;
      exit(1);
    }
    glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
    cerr << "InfoLog:" << endl << infoLog << endl << endl;
    delete infoLog;
  }
  printOpenGLError();
}

void printProgramInfoLog(GLuint program)
{
  int infologLen   = 0;
  int charsWritten = 0;
  GLchar *infoLog;

  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);

  if (infologLen > 1) {
    infoLog = new GLchar[infologLen];
    if (infoLog == NULL) {
      cerr << "Error: Could not allocate infoLog buffer" << endl;
      exit(1);
    }
    glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog);
    cerr << "InfoLog:" << endl << infoLog << endl << endl;
    delete infoLog;
  }
  printOpenGLError();  
}

