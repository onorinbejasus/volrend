#ifndef __VOLUMEBLOCK_H__
#define __VOLUMEBLOCK_H__

#include <omegaGl.h>

class Vec3f {
public:
  Vec3f() :
    x(0.0f),
    y(0.0f),
    z(0.0f)
  {};
  Vec3f(float _x, float _y, float _z) :
    x(_x),
    y(_y),
    z(_z)
  {};

  float x, y, z;
};



class VolumeBlock {
public:
  VolumeBlock() :
    size(1.0f, 1.0f, 1.0f),
    position(0.0f, 0.0f, 0.0f),
    displayList(0),
    texName(0),
    randTexName(0),
    shaderObj(0)
  {};
  ~VolumeBlock() {};

  void draw();
  void initDisplayList();

  Vec3f size;
  Vec3f position;
  GLuint displayList;
  GLuint texName;
  GLuint texNormName;
  GLuint randTexName;
  GLuint cmapTexName;
  GLuint amapTexName;
  GLuint texCoordRLoc;
  GLuint shaderObj;
};

#endif
