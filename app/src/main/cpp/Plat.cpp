//
// Created by cj on 2018/9/3.
//
#include <GLES3/gl31.h>
#include <EGL/egl.h>
#include <cstdlib>

#include "GlHelper.h"
#include "Plat.h"


#define LOG_TAG "FLOOR"
#include "common.h"
#include "CameraSource.h"

extern TexProcess gTexProcess;
extern CameraSource gCameraSource;

static const char VERTEX_SHADER[] =
                "attribute vec3 vertexPosition;\n"
                "attribute vec2 vertexUv;\n"
                "varying vec2 v_texcoord;\n"
                "void main() {\n"
                "  gl_Position =vec4(vertexPosition, 1);\n"
                "  v_texcoord = vertexUv;\n"
                "}\n";

static const char FRAGMENT_SHADER_UYVY[] =
        "precision mediump float;\n"
                "varying vec2 v_texcoord;\n"
                "uniform sampler2D texture;\n"
                "void main() {\n"
                "    vec2 v_new_texcoord;\n"
                "    vec4 rgbColor;\n"
                "    float y,u,v;\n"
                "    v_new_texcoord.x=v_texcoord.x/2.0;\n"
                "    v_new_texcoord.y=v_texcoord.y;\n"
                "    rgbColor =texture2D(texture, v_new_texcoord);\n"
                "    u=rgbColor.r*256.0;\n"
                "    v=rgbColor.b*256.0\n;"
                "    if(mod(v_texcoord.x,2.0)==1.0)\n"
                "       y=rgbColor.a*256.0;\n"
                "    else\n"
                "       y=rgbColor.g*256.0\n;"
                "    rgbColor.r = clamp((1.164*(y-16.0)+1.596*(v-128.0))/256.0,0.0, 1.0);\n"
                "    rgbColor.g = clamp((1.164*(y-16.0)-0.391*(u-128.0)-0.813*(v-128.0))/256.0,0.0, 1.0);\n"
                "    rgbColor.b = clamp((1.164*(y-16.0)+2.018*(u-128.0))/256.0,0.0, 1.0);\n"
                "    rgbColor.a =1.0;\n"
                "    gl_FragColor = rgbColor;\n"
                //  Modified by Jason Huang for UYVY to RGB32 CSC 20181211
                //"  gl_FragColor = texture2D(texture, v_texcoord);\n"
                "}\n";
//YVYU=rgba
static const char FRAGMENT_SHADER[] =
        "precision mediump float;\n"
                "varying vec2 v_texcoord;\n"
                "uniform sampler2D texture;\n"
                "void main() {\n"
                "    vec2 v_new_texcoord;\n"
                "    vec4 rgbColor;\n"
                "    float y,u,v;\n"
                "    v_new_texcoord.x=v_texcoord.x/2.0;\n"
                "    v_new_texcoord.y=v_texcoord.y;\n"
                "    rgbColor =texture2D(texture, v_new_texcoord);\n"
                "    v=rgbColor.g*256.0;\n"
                "    u=rgbColor.a*256.0\n;"
                "    if(mod(v_texcoord.x,2.0)==1.0)\n"
                "       y=rgbColor.b*256.0;\n"
                "    else\n"
                "       y=rgbColor.r*256.0\n;"
                "    rgbColor.r = clamp((1.164*(y-16.0)+1.596*(v-128.0))/256.0,0.0, 1.0);\n"
                "    rgbColor.g = clamp((1.164*(y-16.0)-0.391*(u-128.0)-0.813*(v-128.0))/256.0,0.0, 1.0);\n"
                "    rgbColor.b = clamp((1.164*(y-16.0)+2.018*(u-128.0))/256.0,0.0, 1.0);\n"
                "    rgbColor.a =1.0;\n"
                "    gl_FragColor = rgbColor;\n"
                "}\n";
//#define SAFE_FREE(p) if(p){free(p); p=NULL;}
Plat::Plat()
        :   mEglContext(eglGetCurrentContext()),
            mpVertexBuf(0), /*<! vertex coordinates, 3D */
            mpUvBuf(0),     /*<! texture UV coordinates, 2D */
            mNumToDraw(0),  /*<! number of elements to draw */
            mpTexImg(0),   /*<! texture image data */
            mProgramId(-1), /*<! Shader program ID */
            mTextureDataId(-1),
            mVertexAttrib(-1),      /*<! ID of Vertex position uniform */
            mUvAttrib(-1),    /*<! ID of texture uniform in shader program */
            mTextureUniform(-1),      /*<! ID of texture uniform */
            mMvpMatrixUniform(-1),       /*<! ID of matrix uniform in shader program */
            mVaoId(-1) /*<! ID of Vertex array object*/

{
    mVertexBufId[0] = mVertexBufId[1] = -1;
    mVertexBufId[2] = -1;
}
Plat ::~Plat()
{
    cleanup();
}
void Plat ::cleanup()
{
    SAFE_FREE(mpVertexBuf);
    SAFE_FREE(mpUvBuf);


    if(mVertexBufId[0]) {
        glDeleteBuffers(3, mVertexBufId);
    }

    if (mVaoId) {
        glDeleteVertexArrays(1, &mVaoId);
    }

    if(mProgramId)                   /*<! Shader program ID */
        glDeleteProgram(mProgramId);
    if(mTextureDataId)               /*<! ID of texture data */
        glDeleteTextures(1, &mTextureDataId);
}
#define SX  1 //1.0f/354.0f
#define SY  1 //1.0f/540.0f

/*static const nfFloat3D gpVertexBuf[12] = {
        {-SX,  SY, 0}, { 0, SY, 0}, {-SX, 0, 0},{ 0, 0, 0},
        {0,  SY, 0}, { SX, SY, 0}, {0, 0, 0},{ SX, 0, 0},
        {-SX,  0, 0}, { SX, 0, 0}, {-SX, -SY, 0},{ SX, -SY, 0},

};*/
static const nfFloat3D gpVertexBuf[12] = {
        {0,  SY, 0}, { SX, SY, 0}, {0, 0, 0},{ SX, 0, 0},
        {-SX,  SY, 0}, { 0, SY, 0}, {-SX, 0, 0},{ 0, 0, 0},
        {-SX,  0, 0}, { SX, 0, 0}, {-SX, -SY, 0},{ SX, -SY, 0},

};
static const nfFloat2D gpUvTexture[12] = {
        {0.04375f,0.4427f},{0.10416f,0.27083f}, {0.443055f,0.40625f},{0.36111f,0.28645f},      // right camera
        {0.88541f, 0.04166f},{0.92013f, 0.1875f},{0.6076325f, 0.020833f},{0.52719f, 0.13541f}, // left camera
        {0.95486f, 0.286458f},{0.55555f, 0.286458f},{0.91666f, 0.44791f},{0.5625f, 0.44791f},  //rear camera
};
static const unsigned short gpIndices[18] = {
        0,2,1,2,3,1,
        4,6,5,6,7,5,
        8,10,9,10,11,9,
};

bool Plat ::initVertexData()
{

    glGenVertexArrays(1, &mVaoId);
    glBindVertexArray(mVaoId);

    glGenBuffers(3, mVertexBufId);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gpVertexBuf), &gpVertexBuf[0], GL_STATIC_DRAW);

//
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gpUvTexture), &gpUvTexture[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexBufId[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gpIndices), &gpIndices[0], GL_STATIC_DRAW);

    mNumToDraw = sizeof(gpIndices)/sizeof(gpIndices[0]);

    return true;
}
bool Plat ::init()
{
    mTexWidth = gCameraSource.Width();
    mTexHeight = gCameraSource.Height();

    mProgramId = CreateProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    if (!mProgramId)
        return false;
    mVertexAttrib = glGetAttribLocation(mProgramId, "vertexPosition");
    mUvAttrib =  glGetAttribLocation(mProgramId, "vertexUv");
    mTextureUniform = glGetUniformLocation(mProgramId, "texture");
    initVertexData();

    glGenTextures(1, &mTextureDataId);
    updateTextureData();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    return true;
}

void Plat ::updateTextureData()
{
    loadTexture();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureDataId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, mpTexImg);

}
bool Plat ::loadTexture()
{
    mpTexImg = gCameraSource.GetFrameData();
    return true;
}

/* \brief render Plat image
 * \param bReload set true to reload texture
 *
 */
void Plat ::draw(bool bReload)
{    glUseProgram(mProgramId);
    checkGlError("RendererES2::glUseProgram");
    updateTextureData();
    glUniform1i(mTextureUniform, 0);//"texture" to use first texture data
    glBindBuffer(GL_ARRAY_BUFFER, mTextureDataId);


    glEnableVertexAttribArray(mVertexAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[0]);
    glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(nfFloat3D), (const GLvoid*)0);

    glEnableVertexAttribArray(mUvAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[1]);
    glVertexAttribPointer(mUvAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(nfFloat2D), (const GLvoid*)0);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexBufId[2]);

    glUniformMatrix4fv(mMvpMatrixUniform , 1, GL_FALSE, m_matMvp.Ptr());

    glDrawElements(GL_TRIANGLES, mNumToDraw, GL_UNSIGNED_SHORT, 0 );

    glDisableVertexAttribArray(mVertexAttrib);
    glDisableVertexAttribArray(mUvAttrib);
}

/* \brief update basic matrics
 * \param pojection

 */
void Plat ::update(Mat4& pojection )
{
    m_matMvp = pojection;

}
