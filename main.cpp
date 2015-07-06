#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glext.h>
#include <math.h>
#include <tiffio.h>
#include <iostream>

#include "GLShader.h"
#include "VolumeBlock.h"

#define BLOCK_SCALE 9.0
#define PI 3.14159265358979

using namespace std;

// globals - begin
VolumeBlock volBlock;

GLuint sampleRateLoc;
GLuint xSliceBoundsLoc;
GLuint ySliceBoundsLoc;
GLuint zSliceBoundsLoc;

GLuint volTexLoc;
GLuint texCoordRLoc;
GLuint randTexLoc;
GLuint cmapTexLoc;
GLuint amapTexLoc;
GLuint volNormTexLoc;
GLuint volScaleLoc;
// globals - end

void display()
{
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glRotatef(1.0, 0.0, 1.0, 0.0);
  volBlock.draw();
  
  glutSwapBuffers();
}

void reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, w/(GLfloat)h, .01, 100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0, 0.0, 10.0, // camera position
	    0.0, 0.0, 0.0,  // camera look at
	    0.0, 1.0, 0.0); // camera up vector
}

void idle()
{
  glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
    exit(0);
  }
}

void initGL()
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glEnable(GL_DEPTH_TEST);

  GLVertexShader   vertShader("volumerenderer.vs");
  GLFragmentShader fragShader("volumerenderer.fs");
  GLShaderProgram  volRendShaderProg(vertShader, fragShader);

  volTexLoc = glGetUniformLocationARB(volRendShaderProg.handle, "volumeTex");
  randTexLoc = glGetUniformLocationARB(volRendShaderProg.handle, "randTex");
  cmapTexLoc = glGetUniformLocationARB(volRendShaderProg.handle, "cmapTex");
  amapTexLoc = glGetUniformLocationARB(volRendShaderProg.handle, "amapTex");
  volNormTexLoc = glGetUniformLocationARB(volRendShaderProg.handle, "normalTex");

  volScaleLoc = glGetUniformLocationARB(volRendShaderProg.handle, "volScale");
  sampleRateLoc = glGetUniformLocationARB(volRendShaderProg.handle, "sampleRate");
  xSliceBoundsLoc = glGetUniformLocationARB(volRendShaderProg.handle, "xSliceBounds");
  ySliceBoundsLoc = glGetUniformLocationARB(volRendShaderProg.handle, "ySliceBounds");
  zSliceBoundsLoc = glGetUniformLocationARB(volRendShaderProg.handle, "zSliceBounds");
}

void loadTiff(char *filename)
{
  glGenTextures(1, &volBlock.texName);

  TIFF *tif = TIFFOpen(filename, "r");

  uint32 w, h;
  size_t npixels;
  uint16 *raster;
  uint16 bps, dt;
  int dircount = 0;
  GLfloat *rasterf;
  if (tif) {

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
    TIFFGetField(tif, TIFFTAG_DATATYPE, &dt);

    size_t scanlineSize = TIFFScanlineSize(tif);
    cout << "scanline size: " << scanlineSize << endl;
    cout << "w: " << w << " h: " << h << endl;
    cout << "bps: " << bps << " dt: " << dt << endl;
    cout << "sizeof(uint16): " << sizeof(uint16) << endl;

    do {
      dircount++;
    } while (TIFFReadDirectory(tif));
    cout << dircount << " directories in tif file." << endl;

    npixels = w * h * dircount;
    raster = (uint16*) _TIFFmalloc(npixels * sizeof(uint16));
    rasterf = (GLfloat*)malloc(npixels * sizeof(GLfloat));
    if (!raster) {
      cerr << "TDVolumeRenderer::init Out of memory." << endl;
      exit(0); // exit if no memory for the tiff
    }
    if (!rasterf) {
      cerr << "TDVolumeRenderer::init Out of memory." << endl;
      exit(0);
    }

    uint16 min = 65000,
      max = 0;

    for (int dirIx = 0; dirIx < dircount; dirIx++) {
      TIFFSetDirectory(tif, dirIx);

      for (int scanlineIx = h-1; scanlineIx >= 0; scanlineIx--) {
	TIFFReadScanline(tif, raster + dirIx*w*h + scanlineIx*w, scanlineIx);

	for (int col = 0; col < w; col++) {
	  uint16 curVal = raster[dirIx*w*h + scanlineIx*w + col];
	  rasterf[dirIx*w*h + scanlineIx*w + col] = float(curVal);
	  if (curVal > max) max = curVal;
	  if (curVal < min) min = curVal;
	}
      }
    }

    // Normalize values to range [0, 1]
    uint16 range = max - min;
    for (int i = 0; i < npixels; i++)
      rasterf[i] = (rasterf[i] - float(min))/float(range);

    cout << "min: " << min << endl
	 << "max: " << max << endl;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, volBlock.texName);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_ALPHA16,
		 w, h, dircount,                // width, height, depth
		 0, GL_ALPHA, GL_FLOAT, rasterf);

    TIFFClose(tif);
  }

  volBlock.size.x = .84; // * BLOCK_SCALE;   // rabbit brain 
  volBlock.size.y = 1.0; // * BLOCK_SCALE;   // rabbit brain
  volBlock.size.z = .64; // * BLOCK_SCALE;   // rabbit brain

  glUniform1i(volTexLoc, 0);
  glUniform1f(volBlock.texCoordRLoc, 0.0f);
  glUniform3f(volScaleLoc, 1/.84f, 1/1.0f, 1/.64f);
  glUniform1f(sampleRateLoc, 1.0f);
  glUniform2f(xSliceBoundsLoc, 0.0f, 1);
  glUniform2f(ySliceBoundsLoc, 0, 1);
  glUniform2f(zSliceBoundsLoc, 0, 1);

  // random texture (randomly offset starting point of rays to reduce aliasing)
  GLfloat raster_rand[128*128];
  srand(time(NULL));
  for (int i=0; i<(128*128); i++) {
    raster_rand[i] = rand()/float(RAND_MAX);
  }

  glActiveTexture(GL_TEXTURE1);
  glGenTextures(1, &volBlock.randTexName);
  glBindTexture(GL_TEXTURE_2D, volBlock.randTexName);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
	       128, 128,
	       0, GL_ALPHA, GL_FLOAT, raster_rand);

  glUniform1i(randTexLoc, 1);


  // color look up texture
  const int cmapWidth = 2048;
  const int cmapOffset = .2*cmapWidth;
  GLubyte raster_cmap[cmapWidth][4];
  double phaseShift = 120*(PI/180.0);
  for (int i=0; i<cmapWidth; i++) {
    double redRad = ((i+cmapOffset)/((cmapWidth-1)*.5))*(2*PI);  // Current radian (for red channel)                         
    raster_cmap[i][0] = GLubyte(255 * (cos(redRad)/2.0 + 0.5) );  // map cos from [-1, 1] to [0, 1]                          
    raster_cmap[i][1] = GLubyte(255 * (cos(redRad+phaseShift)/2.0 + 0.5) );
    raster_cmap[i][2] = GLubyte(255 * (cos(redRad+(2*phaseShift))/2.0 + 0.5) );
    raster_cmap[i][3] = 255;
  }
  glActiveTexture(GL_TEXTURE2);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glGenTextures(1, &volBlock.cmapTexName);
  glBindTexture(GL_TEXTURE_1D, volBlock.cmapTexName);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA,
	       cmapWidth,
	       0, GL_RGBA, GL_UNSIGNED_BYTE, raster_cmap);
  glUniform1i(cmapTexLoc, 2);
  glEnable(GL_TEXTURE_1D);

  float sldr = .278f;
  const int amapWidth = 2048;
  GLfloat raster_amap[amapWidth];
  for (int i=0; i<amapWidth; i++) {
    raster_amap[i] = 0.2 * exp(-100 * ((i/float(amapWidth))-sldr) * ((i/float(amapWidth))-sldr) );
  }
  glActiveTexture(GL_TEXTURE3);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glGenTextures(1, &volBlock.amapTexName);
  glBindTexture(GL_TEXTURE_1D, volBlock.amapTexName);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage1D(GL_TEXTURE_1D, 0, GL_ALPHA,
	       amapWidth,
	       0, GL_ALPHA, GL_FLOAT, raster_amap);
  glUniform1i(amapTexLoc, 3);
  glEnable(GL_TEXTURE_1D);

  GLfloat maxMag = 0.f;
  GLfloat *norms = (GLfloat*)malloc(npixels * 4 * sizeof(GLfloat));
  for (int z = 0; z < dircount; z++) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
	int curIx = 4*(z*h*w+y*w+x);
	int curfIx = z*h*w+y*w+x;
	int nextIxX = x < (w-1) ? z*h*w+y*w+x+1 : curfIx;
	int prevIxX = x > 0 ? z*h*w+y*w+x-1 : curfIx;
	int nextIxY = y < (h-1) ? z*h*w+(y+1)*w+x : curfIx;
	int prevIxY = y > 0 ? z*h*w+(y-1)*w+x : curfIx;
	int nextIxZ = z < (dircount-1) ? (z+1)*h*w+y*w+x : curfIx;
	int prevIxZ = z > 0 ? (z-1)*h*w+y*w+x : curfIx;
	float nx = norms[curIx + 0] = (1/volBlock.size.x) * (rasterf[nextIxX] - rasterf[prevIxX]);
	float ny = norms[curIx + 1] = (1/volBlock.size.y) * (rasterf[nextIxY] - rasterf[prevIxY]);
	float nz = norms[curIx + 2] = (1/volBlock.size.z) * (rasterf[nextIxZ] - rasterf[prevIxZ]);
	float nm = norms[curIx + 3] = sqrt(nx*nx + ny*ny + nz*nz);
	if (nm == 0)
	  norms[curIx + 0] = norms[curIx + 1] = norms[curIx + 2] = 0.f;
	else {
	  norms[curIx + 0] /= nm;
	  norms[curIx + 1] /= nm;
	  norms[curIx + 2] /= nm;
	  if (nm > maxMag) maxMag = nm;
	}
	//if (nm != 0) cerr << "nm: " << norms[curIx+3] << endl;                                                             
      }
    }
  }

  for (int z = 0; z < dircount; z++) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
	int curIx = 4*(z*h*w+y*w+x);
	//norms[curIx + 3] /= maxMag;
      }
    }
  }

  volBlock.size.x *= BLOCK_SCALE;   // rabbit brain
  volBlock.size.y *= BLOCK_SCALE;   // rabbit brain
  volBlock.size.z *= BLOCK_SCALE;   // rabbit brain
  
  volBlock.initDisplayList();

  glActiveTexture(GL_TEXTURE4);
  glGenTextures(1, &volBlock.texNormName);
  glBindTexture(GL_TEXTURE_3D, volBlock.texNormName);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8_SNORM,
	       w, h, dircount,                // width, height, depth                                                        
	       0, GL_RGBA, GL_FLOAT, norms);
  glUniform1i(volNormTexLoc, 4);
  glEnable(GL_TEXTURE_3D);
}

int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
  glutInitWindowSize(1280, 720);
  glutCreateWindow("Volume Render");

  initGL();
  char filename[] = "rabbit.tif";
  loadTiff(filename);

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);

  glutMainLoop();

  return 0;
}
