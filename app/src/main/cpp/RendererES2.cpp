/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "navmjni.h"
#include <EGL/egl.h>
#include "GlHelper.h"

static const char VERTEX_SHADER_org[] =
    "#version 100\n"
    "uniform mat2 scaleRot;\n"
    "uniform vec2 offset;\n"
    "attribute vec2 pos;\n"
    "attribute vec4 color;\n"
    "varying vec4 vColor;\n"
    "void main() {\n"
    "    gl_Position = vec4(scaleRot*pos + offset, 0.0, 1.0);\n"
    "    vColor = color;\n"
    "}\n";

static const char FRAGMENT_SHADER_org[] =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec4 vColor;\n"
    "void main() {\n"
    "    gl_FragColor = vColor;\n"
    "}\n";
static const char VERTEX_SHADER[] =
                 "uniform mat2 scaleRot;\n"
                "uniform vec2 offset;\n"
                "attribute vec2 pos;\n"
                "attribute vec2 vertexUv;\n"
                "varying vec2 v_texcoord;\n"
                "void main() {\n"
                "    gl_Position = vec4(scaleRot*pos + offset, 0.0, 1.0);\n"
                "  v_texcoord = vertexUv;\n"
                "}\n";

static const char FRAGMENT_SHADER[] =
                 "precision mediump float;\n"
                "varying vec2 v_texcoord;\n"
                "uniform sampler2D texture;\n"
                "void main() {\n"
                "  gl_FragColor = texture2D(texture, v_texcoord);\n"
                "}\n";
#define TEX_WIDTH   256
#define TEX_HEIGHT  256
typedef struct _nfFloatPoint {
    float x;
    float y;
}nfFloatPoint;
static const nfFloatPoint gUvTexture[4] = {
    {0.0f,1.0f}, {1,1}, {0,0}, {1,0}
};
class RendererES2: public Renderer {
public:
    RendererES2();
    virtual ~RendererES2();
    bool init();
    void UpdateTexture();
private:
    virtual float* mapOffsetBuf();
    virtual void unmapOffsetBuf();
    virtual float* mapTransformBuf();
    virtual void unmapTransformBuf();
    virtual void draw(unsigned int numInstances);

    const EGLContext mEglContext;
    GLuint mProgram;
    GLuint mVB;
    GLint mPosAttrib;
 //   GLint mColorAttrib;
    GLint mScaleRotUniform;
    GLint mOffsetUniform;

    float mOffsets[2*MAX_INSTANCES];
    float mScaleRot[4*MAX_INSTANCES];   // array of 2x2 column-major matrices
    //cj texture test
    unsigned char  mpTextImg[TEX_WIDTH*TEX_HEIGHT*4];   /*<! texture image data */
    GLuint mTexturePos;         /*<! ID of texture uniform in shader program */
    GLuint mTextureDataId;      /*<! ID of texture data */
    GLuint mTextureAttrib;             /*<! texture UV coordinates, 2D */
    GLuint mTextureBufId;
};

Renderer* createES2Renderer() {
    RendererES2* renderer = new RendererES2;
    if (!renderer->init()) {
        delete renderer;
        return NULL;
    }
    return renderer;
}

RendererES2::RendererES2()
:   mEglContext(eglGetCurrentContext()),
    mProgram(0),
    mVB(0),
    mPosAttrib(-1),
    mScaleRotUniform(-1),
    mOffsetUniform(-1),
    mTextureDataId(0)
{
    //    mColorAttrib(-1),
}

bool RendererES2::init() {
    mProgram = CreateProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    checkGlError("RendererES2::init() CreateProgram ");
    if (!mProgram)
        return false;
    ALOGV("RendererES2::init()  ------------");
    mPosAttrib = glGetAttribLocation(mProgram, "pos");
//j    mColorAttrib = glGetAttribLocation(mProgram, "color");
    mScaleRotUniform = glGetUniformLocation(mProgram, "scaleRot");
    mOffsetUniform = glGetUniformLocation(mProgram, "offset");

    glGenBuffers(1, &mVB);
    glBindBuffer(GL_ARRAY_BUFFER, mVB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD), &QUAD[0], GL_STATIC_DRAW);

    //
    mTextureAttrib =  glGetAttribLocation(mProgram, "vertexUv");
    glGenBuffers(1, &mTextureBufId);
    glBindBuffer(GL_ARRAY_BUFFER, mTextureBufId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gUvTexture), &gUvTexture[0], GL_STATIC_DRAW);


    mTexturePos = glGetUniformLocation(mProgram, "texture");
    glGenTextures(1, &mTextureDataId);

    UpdateTexture();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


    ALOGV("Using OpenGL ES 2.0 renderer");
    return true;
}
static unsigned char cc = 0;
void RendererES2::UpdateTexture()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureDataId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, mpTextImg);

    for (int i=0; i<TEX_HEIGHT; i++) {
        for (int j=0; j<TEX_WIDTH; j++) {
            mpTextImg[i*TEX_WIDTH*4+ j*4 ] = 128; //R
            mpTextImg[i*TEX_WIDTH*4+ j*4 +1] = (i+j)%255;
            mpTextImg[i*TEX_WIDTH*4+ j*4 +2] = (cc)%255; //B
            mpTextImg[i*TEX_WIDTH*4+ j*4 +3] = 255;
        }
    }
    cc++;

}
RendererES2::~RendererES2() {
    /* The destructor may be called after the context has already been
     * destroyed, in which case our objects have already been destroyed.
     *
     * If the context exists, it must be current. This only happens when we're
     * cleaning up after a failed init().
     */
    if (eglGetCurrentContext() != mEglContext)
        return;
    glDeleteBuffers(1, &mVB);

    glDeleteBuffers(1, &mTextureBufId);
    if(mTextureDataId)               /*<! ID of texture data */
        glDeleteTextures(1, &mTextureDataId);

    glDeleteProgram(mProgram);
}

float* RendererES2::mapOffsetBuf() {
    return mOffsets;
}

void RendererES2::unmapOffsetBuf() {
}

float* RendererES2::mapTransformBuf() {
    return mScaleRot;
}

void RendererES2::unmapTransformBuf() {
}

void RendererES2::draw(unsigned int numInstances) {
    glUseProgram(mProgram);
    checkGlError("RendererES2::glUseProgram");
//cj
    // Bind our texture in Texture Unit 0
    /*
    glActiveTexture(GL_TEXTURE0);
    checkGlError("glActiveTexture(GL_TEXTURE0)");
    glBindTexture(GL_TEXTURE_2D, mTextureDataId);
    checkGlError("glBindTexture(GL_TEXTURE_2D");
    // Set our "myTextureSampler" sampler to use Texture Unit 0
     */
    UpdateTexture();
    glUniform1i(mTexturePos, 0);
    checkGlError("glUniform1i(mTexturePos, 0)");

    glBindBuffer(GL_ARRAY_BUFFER, mTextureBufId);

    glVertexAttribPointer(mTextureAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(nfFloatPoint), (const GLvoid*)0);

    glEnableVertexAttribArray(mTextureAttrib);

//cj


    glBindBuffer(GL_ARRAY_BUFFER, mVB);
    glVertexAttribPointer(mPosAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, pos));
     glEnableVertexAttribArray(mPosAttrib);

    for (unsigned int i = 0; i < numInstances; i++) {
        glUniformMatrix2fv(mScaleRotUniform, 1, GL_FALSE, mScaleRot + 4*i);
        glUniform2fv(mOffsetUniform, 1, mOffsets + 2*i);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}
