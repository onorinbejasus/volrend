#ifndef __GLERROR_H__
#define __GLERROR_H__

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

void printOpenGLError();
void printShaderInfoLog(GLuint shader);
void printProgramInfoLog(GLuint program);

#endif
