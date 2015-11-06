#include <omega.h>
#include <omegaGl.h>

// Include Omegalib FreeImage for multipage TIFF loading
#define FREEIMAGE_BIGENDIAN
#include "FreeImage.h"

#include "VolumeBlock.h"

using namespace std;
using namespace omega;

///////////////////////////////////////////////////////////////////////////////
// Volume Renderer Engine Module (CPU stuff)
class VolumeRenderModule : public EngineModule
{
public:
    VolumeRenderModule() :
        EngineModule("VolumeRenderModule"),
        rasterf(NULL),
        rasterUpdated(false),
        // Default values (good for the sample rabbit brain data)
        sizeX(.84f),
        sizeY(1.0f),
        sizeZ(.64f),
        scale(9.0f)
    {
        FreeImage_Initialise();
    }

    virtual void initializeRenderer(Renderer* r);

    void loadTiff(const String& file)
    {
        size_t npixels;

        String filePath;
        if(!DataManager::findFile(file, filePath))
        {
            ofwarn("[VolumeRenderModule] Could not find file: %1%", %file);
            return;
        }

        FIMULTIBITMAP* tif = FreeImage_OpenMultiBitmap(FIF_TIFF, filePath.c_str(), FALSE, TRUE, TRUE, 0);
        if(!tif)
        {
            ofwarn("[VolumeRenderModule] File opening failed: %1%", %file);
            return;
        }

        // Read tif info
        dircount = FreeImage_GetPageCount(tif);
        FIBITMAP* tifp = FreeImage_LockPage(tif, 0);
        w = FreeImage_GetWidth(tifp);
        h = FreeImage_GetHeight(tifp);
        bps = FreeImage_GetBPP(tifp);
        FreeImage_UnlockPage(tif, tifp, FALSE);
        ofmsg("[VolumeRenderModule: %1%] pages:<%2%>  width:<%3%>  height:<%4%>  bps:<%5%>",
            %file %dircount %w %h %bps);

        npixels = w * h * dircount;
        rasterf = (float*)malloc(npixels * sizeof(float));
        if(!rasterf)
        {
            oerror("[VolumeRenderModule] Out of Memory");
            return;
        }

        // Read all pages
        uint16_t min = 65000, max = 0;
        for(int dirIx = 0; dirIx < dircount; dirIx++)
        {
            FIBITMAP* tifp = FreeImage_LockPage(tif, dirIx);

            for(int scanlineIx = h - 1; scanlineIx >= 0; scanlineIx--)
            {
                uint16_t* raster = (uint16_t*)FreeImage_GetScanLine(tifp, scanlineIx);

                for(int col = 0; col < w; col++)
                {
                    uint16_t curVal = raster[col];
                    rasterf[dirIx*w*h + scanlineIx*w + col] = float(curVal);
                    if(curVal > max) max = curVal;
                    if(curVal < min) min = curVal;
                }
            }
            FreeImage_UnlockPage(tif, tifp, FALSE);
        }

        // Normalize values to range [0, 1]
        uint16_t range = max - min;
        for(int i = 0; i < npixels; i++)
            rasterf[i] = (rasterf[i] - float(min)) / float(range);

        ofmsg("[VolumeRenderModule: %1%] min:<%2%>  max:<%3%>", %file %min %max);

        // Compute the normals
        float maxMag = 0.f;
        norms = (float*)malloc(npixels * 4 * sizeof(float));
        for(int z = 0; z < dircount; z++)
        {
            for(int y = 0; y < h; y++)
            {
                for(int x = 0; x < w; x++)
                {
                    int curIx = 4 * (z*h*w + y*w + x);
                    int curfIx = z*h*w + y*w + x;
                    int nextIxX = x < (w - 1) ? z*h*w + y*w + x + 1 : curfIx;
                    int prevIxX = x > 0 ? z*h*w + y*w + x - 1 : curfIx;
                    int nextIxY = y < (h - 1) ? z*h*w + (y + 1)*w + x : curfIx;
                    int prevIxY = y > 0 ? z*h*w + (y - 1)*w + x : curfIx;
                    int nextIxZ = z < (dircount - 1) ? (z + 1)*h*w + y*w + x : curfIx;
                    int prevIxZ = z > 0 ? (z - 1)*h*w + y*w + x : curfIx;
                    float nx = norms[curIx + 0] = (1 / sizeX) * (rasterf[nextIxX] - rasterf[prevIxX]);
                    float ny = norms[curIx + 1] = (1 / sizeY) * (rasterf[nextIxY] - rasterf[prevIxY]);
                    float nz = norms[curIx + 2] = (1 / sizeZ) * (rasterf[nextIxZ] - rasterf[prevIxZ]);
                    float nm = norms[curIx + 3] = sqrt(nx*nx + ny*ny + nz*nz);
                    if(nm == 0)
                        norms[curIx + 0] = norms[curIx + 1] = norms[curIx + 2] = 0.f;
                    else {
                        norms[curIx + 0] /= nm;
                        norms[curIx + 1] /= nm;
                        norms[curIx + 2] /= nm;
                        if(nm > maxMag) maxMag = nm;
                    }
                    //if (nm != 0) cerr << "nm: " << norms[curIx+3] << endl;
                }
            }
        }

        for(int z = 0; z < dircount; z++) {
            for(int y = 0; y < h; y++) {
                for(int x = 0; x < w; x++) {
                    int curIx = 4 * (z*h*w + y*w + x);
                    //norms[curIx + 3] /= maxMag;
                }
            }
        }

        // Signal render passes that the raster data has been updated.
        rasterUpdated = true;

        FreeImage_CloseMultiBitmap(tif);
    }

    virtual void update(const UpdateContext& context)
    {
        // After a frame all render passes had a chance to update their
        // textures. reset the raster update flag.
        //rasterUpdated = false;
    }

    // Data physical size
    float sizeX;
    float sizeY;
    float sizeZ;
    float scale;

    // Volume data store
    float* rasterf;
    float* norms;
    uint32_t w, h;
    uint16_t bps;
    int dircount;
    bool rasterUpdated;
};

///////////////////////////////////////////////////////////////////////////////
// Volume Render Pass (GPU Stuff)
class VolumeRenderPass : public RenderPass
{
public:
    VolumeRenderPass(Renderer* client, VolumeRenderModule* vrm) :
        RenderPass(client, "VolumeRenderPass"),
        module(vrm) {}

    virtual void initialize()
    {
        DrawInterface* di = getClient()->getRenderer();
        volRendShaderProg = di->getOrCreateProgram(
            "volRendShaderProg",
            "volrend/volumerenderer.vs",
            "volrend/volumerenderer.fs");
        glUseProgram(volRendShaderProg);

        volTexLoc = glGetUniformLocation(volRendShaderProg, "volumeTex");
        randTexLoc = glGetUniformLocation(volRendShaderProg, "randTex");
        cmapTexLoc = glGetUniformLocation(volRendShaderProg, "cmapTex");
        amapTexLoc = glGetUniformLocation(volRendShaderProg, "amapTex");
        volNormTexLoc = glGetUniformLocation(volRendShaderProg, "normalTex");

        volScaleLoc = glGetUniformLocation(volRendShaderProg, "volScale");
        sampleRateLoc = glGetUniformLocation(volRendShaderProg, "sampleRate");
        xSliceBoundsLoc = glGetUniformLocation(volRendShaderProg, "xSliceBounds");
        ySliceBoundsLoc = glGetUniformLocation(volRendShaderProg, "ySliceBounds");
        zSliceBoundsLoc = glGetUniformLocation(volRendShaderProg, "zSliceBounds");

        glGenTextures(1, &volBlock.texName);

        volBlock.size.x = module->sizeX; // * BLOCK_SCALE;   // rabbit brain
        volBlock.size.y = module->sizeY; // * BLOCK_SCALE;   // rabbit brain
        volBlock.size.z = module->sizeZ; // * BLOCK_SCALE;   // rabbit brain

        glUniform1i(volTexLoc, 0);
        //glUniform1f(volBlock.texCoordRLoc, 0.0f);
        glUniform3f(volScaleLoc, 1 / .84f, 1 / 1.0f, 1 / .64f);
        glUniform1f(sampleRateLoc, 1.0f);
        glUniform2f(xSliceBoundsLoc, 0.0f, 1);
        glUniform2f(ySliceBoundsLoc, 0, 1);
        glUniform2f(zSliceBoundsLoc, 0, 1);

        // random texture (randomly offset starting point of rays to reduce aliasing)
        GLfloat raster_rand[128 * 128];
        srand(time(NULL));
        for(int i = 0; i<(128 * 128); i++)
        {
            raster_rand[i] = rand() / float(RAND_MAX);
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

            // color look up texture
      const int cmapWidth = 6;
      //const int cmapOffset = .2*cmapWidth;

        GLubyte raster_cmap[cmapWidth][4];
        // 0 -> (0,0,0)
        raster_cmap[0][0] = GLubyte(255 * 0);
        raster_cmap[0][1] = GLubyte(255 * 0);
        raster_cmap[0][2] = GLubyte(255 * 0);
        raster_cmap[0][3] = GLubyte(255 * 1);

        // 25500 -> (0,0.3,0.3)
        raster_cmap[1][0] = GLubyte(255 * 0);
        raster_cmap[1][1] = GLubyte(255 * 0.3);
        raster_cmap[1][2] = GLubyte(255 * 0.3);
        raster_cmap[1][3] = GLubyte(255 * 1);

        // 26500 -> (0.5,0.5,0.5)
        raster_cmap[2][0] = GLubyte(255 * 0.5);
        raster_cmap[2][1] = GLubyte(255 * 0.5);
        raster_cmap[2][2] = GLubyte(255 * 0.5);
        raster_cmap[2][3] = GLubyte(255 * 1);

        // 27500 -> (1,1,1)
        raster_cmap[3][0] = GLubyte(255 * 1);
        raster_cmap[3][1] = GLubyte(255 * 1);
        raster_cmap[3][2] = GLubyte(255 * 1);
        raster_cmap[3][3] = GLubyte(255 * 1);

        // 28500 -> (1,0.2,0.2)
        raster_cmap[4][0] = GLubyte(255 * 1);
        raster_cmap[4][1] = GLubyte(255 * 0.2);
        raster_cmap[4][2] = GLubyte(255 * 0.2);
        raster_cmap[4][3] = GLubyte(255 * 1);

        // 65535 -> (1,0,0)
        raster_cmap[5][0] = GLubyte(255 * 1);
        raster_cmap[5][1] = GLubyte(255 * 0);
        raster_cmap[5][2] = GLubyte(255 * 0);
        raster_cmap[5][3] = GLubyte(255 * 1);

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

        //float sldr = .278f;
        const int amapWidth = 6;//2048;
        GLfloat raster_amap[amapWidth];

        // 0 -> 0
        raster_amap[0] = 0;
        // 25000 -> 0
        raster_amap[1] = 0;
        // 25500 -> 0
        raster_amap[2] = 0.1;
        // 26301 -> 0.4
        raster_amap[3] = 0.4;
        // 39916 -> 0.94
        raster_amap[4] = 0.94;
        // 65535 -> 1
        raster_amap[5] = 1.0;

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

        glUseProgram(0);
        oglError;

        RenderPass::initialize();
    }

    virtual void render(Renderer* client, const DrawContext& context)
    {
        // Do we have new raster data?
        if(module->rasterUpdated)
        {
            oglError;
            //glPushAttrib(GL_TEXTURE_BIT);
            glEnable(GL_TEXTURE_3D);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, volBlock.texName);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage3D(GL_TEXTURE_3D, 0, GL_ALPHA16,
                module->w, module->h, module->dircount, // width, height, depth
                0, GL_ALPHA, GL_FLOAT, module->rasterf);

            volBlock.size.x *= module->scale;   // rabbit brain
            volBlock.size.y *= module->scale;   // rabbit brain
            volBlock.size.z *= module->scale;   // rabbit brain
            oglError;

            volBlock.initDisplayList();
            oglError;

            glActiveTexture(GL_TEXTURE4);
            glGenTextures(1, &volBlock.texNormName);
            glBindTexture(GL_TEXTURE_3D, volBlock.texNormName);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8_SNORM,
                module->w, module->h, module->dircount,                // width, height, depth
                0, GL_RGBA, GL_FLOAT, module->norms);
            glUseProgram(volRendShaderProg);
            glUniform1i(volNormTexLoc, 4);
            oglError;

            module->rasterUpdated = false;
            //glPopAttrib();
        }

        if(context.task == DrawContext::SceneDrawTask)
        {
            glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
            glEnable(GL_TEXTURE_1D);
            client->getRenderer()->beginDraw3D(context);
            glUseProgram(volRendShaderProg);
            glEnable (GL_BLEND);
            glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            volBlock.draw();
            glUseProgram(0);
            client->getRenderer()->endDraw();
            glPopAttrib();
        }
    }

private:
    VolumeRenderModule* module;

    VolumeBlock volBlock;
    GLuint volRendShaderProg;
    GLuint sampleRateLoc;
    GLuint xSliceBoundsLoc;
    GLuint ySliceBoundsLoc;
    GLuint zSliceBoundsLoc;

    GLuint volTexLoc;
    //GLuint texCoordRLoc;
    GLuint randTexLoc;
    GLuint cmapTexLoc;
    GLuint amapTexLoc;
    GLuint volNormTexLoc;
    GLuint volScaleLoc;
};

///////////////////////////////////////////////////////////////////////////////
void VolumeRenderModule::initializeRenderer(Renderer* r)
{
    r->addRenderPass(new VolumeRenderPass(r, this));
}

///////////////////////////////////////////////////////////////////////////////
VolumeRenderModule* initialize()
{
    VolumeRenderModule* vrm = new VolumeRenderModule();
    ModuleServices::addModule(vrm);
    vrm->doInitialize(Engine::instance());
    return vrm;
}

///////////////////////////////////////////////////////////////////////////////
// Python API
#include "omega/PythonInterpreterWrapper.h"
BOOST_PYTHON_MODULE(volrend)
{
    // OmegaViewer
    PYAPI_REF_BASE_CLASS(VolumeRenderModule)
        PYAPI_METHOD(VolumeRenderModule, loadTiff)
        ;

    def("initialize", initialize, PYAPI_RETURN_REF);
}
