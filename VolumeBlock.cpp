#include "VolumeBlock.h"

void VolumeBlock::draw() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, this->texName);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, this->randTexName);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_1D, this->cmapTexName);
  glCallList(this->displayList);
}

void VolumeBlock::initDisplayList()
{
  GLfloat x = this->position.x,
          y = this->position.y,
          z = this->position.z;
  GLfloat halfSizeX = this->size.x/2.0f,
          halfSizeY = this->size.y/2.0f,
          halfSizeZ = this->size.z/2.0f;

  this->displayList = glGenLists(1);
  glNewList(this->displayList, GL_COMPILE);
  
  glBegin(GL_QUADS);
    // Front Face
    glTexCoord3f(0.0f, 0.0f, 1.0f); glVertex3f(x-halfSizeX, y-halfSizeY, z+halfSizeZ);
    glTexCoord3f(1.0f, 0.0f, 1.0f); glVertex3f(x+halfSizeX, y-halfSizeY, z+halfSizeZ);
    glTexCoord3f(1.0f, 1.0f, 1.0f); glVertex3f(x+halfSizeX, y+halfSizeY, z+halfSizeZ);
    glTexCoord3f(0.0f, 1.0f, 1.0f); glVertex3f(x-halfSizeX, y+halfSizeY, z+halfSizeZ);

    // Top Face
    glTexCoord3f(0.0f, 1.0f, 1.0f); glVertex3f(x-halfSizeX, y+halfSizeY, z+halfSizeZ);
    glTexCoord3f(1.0f, 1.0f, 1.0f); glVertex3f(x+halfSizeX, y+halfSizeY, z+halfSizeZ);
    glTexCoord3f(1.0f, 1.0f, 0.0f); glVertex3f(x+halfSizeX, y+halfSizeY, z-halfSizeZ);
    glTexCoord3f(0.0f, 1.0f, 0.0f); glVertex3f(x-halfSizeX, y+halfSizeY, z-halfSizeZ);
    
    // Left Face
    glTexCoord3f(0.0f, 0.0f, 0.0f); glVertex3f(x-halfSizeX, y-halfSizeY, z-halfSizeZ);
    glTexCoord3f(0.0f, 0.0f, 1.0f); glVertex3f(x-halfSizeX, y-halfSizeY, z+halfSizeZ);
    glTexCoord3f(0.0f, 1.0f, 1.0f); glVertex3f(x-halfSizeX, y+halfSizeY, z+halfSizeZ);
    glTexCoord3f(0.0f, 1.0f, 0.0f); glVertex3f(x-halfSizeX, y+halfSizeY, z-halfSizeZ);

    // Back Face
    glTexCoord3f(1.0f, 0.0f, 0.0f); glVertex3f(x+halfSizeX, y-halfSizeY, z-halfSizeZ);
    glTexCoord3f(0.0f, 0.0f, 0.0f); glVertex3f(x-halfSizeX, y-halfSizeY, z-halfSizeZ);
    glTexCoord3f(0.0f, 1.0f, 0.0f); glVertex3f(x-halfSizeX, y+halfSizeY, z-halfSizeZ);
    glTexCoord3f(1.0f, 1.0f, 0.0f); glVertex3f(x+halfSizeX, y+halfSizeY, z-halfSizeZ);
    
    // Right Face
    glTexCoord3f(1.0f, 0.0f, 1.0f); glVertex3f(x+halfSizeX, y-halfSizeY, z+halfSizeZ);
    glTexCoord3f(1.0f, 0.0f, 0.0f); glVertex3f(x+halfSizeX, y-halfSizeY, z-halfSizeZ);
    glTexCoord3f(1.0f, 1.0f, 0.0f); glVertex3f(x+halfSizeX, y+halfSizeY, z-halfSizeZ);
    glTexCoord3f(1.0f, 1.0f, 1.0f); glVertex3f(x+halfSizeX, y+halfSizeY, z+halfSizeZ);
    
    // Bottom Face
    glTexCoord3f(0.0f, 0.0f, 0.0f); glVertex3f(x-halfSizeX, y-halfSizeY, z-halfSizeZ);
    glTexCoord3f(1.0f, 0.0f, 0.0f); glVertex3f(x+halfSizeX, y-halfSizeY, z-halfSizeZ);
    glTexCoord3f(1.0f, 0.0f, 1.0f); glVertex3f(x+halfSizeX, y-halfSizeY, z+halfSizeZ);
    glTexCoord3f(0.0f, 0.0f, 1.0f); glVertex3f(x-halfSizeX, y-halfSizeY, z+halfSizeZ);
    
  glEnd();

  glEndList();
}
