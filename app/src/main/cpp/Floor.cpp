//
// Created by cj on 2018/9/3.
//
#include <GLES3/gl31.h>
#include <EGL/egl.h>
#include <cstdlib>

#include "GlHelper.h"
#include "Floor.h"


#define LOG_TAG "FLOOR"
#include "common.h"

static const char VERTEX_SHADER[] =
        "uniform mat4 mvp_matrix;\n"
                "attribute vec3 vertexPosition;\n"
                "attribute vec2 vertexUv;\n"
                "varying vec2 v_texcoord;\n"
                "void main() {\n"
                "  gl_Position = mvp_matrix*vec4(vertexPosition, 1);\n"
                "  v_texcoord = vertexUv;\n"
                "}\n";

static const char FRAGMENT_SHADER[] =
        "precision mediump float;\n"
                "varying vec2 v_texcoord;\n"
                "uniform sampler2D texture;\n"
                "void main() {\n"
                "  gl_FragColor = texture2D(texture, v_texcoord);\n"
                "}\n";

#define SAFE_FREE(p) if(p){free(p); p=NULL;}
Floor::Floor()
        :   mEglContext(eglGetCurrentContext()),
            mpVertexBuf(0), /*<! vertex coordinates, 3D */
            mpUvBuf(0),     /*<! texture UV coordinates, 2D */
            mpTexImg(0),   /*<! texture image data */
            mNumToDraw(0),  /*<! number of elements to draw */
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
Floor ::~Floor()
{
    cleanup();
}
void Floor ::cleanup()
{
    SAFE_FREE(mpVertexBuf);
    SAFE_FREE(mpUvBuf);
    SAFE_FREE(mpTexImg);
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

const nfFloat3D gpVertexBuf[4] = {
        {-10,  0, 10}, { 10, 0, 10}, {-10, 0,  -10},{ 10, 0, -10},
};
static const nfFloat2D gpUvTexture[4] = {
        {0.0f,0.0f}, {1,0}, {0,1}, {1,1}
};
static const unsigned short gpIndices[6] = {
        0,1,2,2,1,3
};

bool Floor ::initVertexData()
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

    mNumToDraw = 6;
    return true;
}
bool Floor ::init()
{
     mProgramId = CreateProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    if (!mProgramId)
        return false;
    mVertexAttrib = glGetAttribLocation(mProgramId, "vertexPosition");
    mUvAttrib =  glGetAttribLocation(mProgramId, "vertexUv");
    mMvpMatrixUniform = glGetUniformLocation(mProgramId, "mvp_matrix");
    mTextureUniform = glGetUniformLocation(mProgramId, "texture");
    initVertexData();

    glGenTextures(1, &mTextureDataId);
    updateTextureData();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return true;
}
bool Floor ::loadTexture()
{
return true;
}

nfPByte Floor ::allocTextureImage(int width, int height, int depth)
{
    SAFE_FREE(mpTexImg);
    mTexWidth = width;
    mTexHeight = height;
    mTexDepth = depth;
    mpTexImg = (nfPByte) malloc(mTexWidth*mTexHeight*mTexDepth);
    return mpTexImg;
}
static unsigned char cc = 110;
void Floor ::updateTextureData()
{

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureDataId);
    //reload image
    /*
    for (int i=0; i<TEX_HEIGHT; i++) {
        for (int j=0; j<TEX_WIDTH; j++) {
            mpTexImg[i*mTexWidth*4+ j*4 ] = 128; //R
            mpTexImg[i*mTexWidth*4+ j*4 +1] = (i+j)%255;
            mpTexImg[i*mTexWidth*4+ j*4 +2] = (cc)%255; //B
            mpTexImg[i*mTexWidth*4+ j*4 +3] = 255;
        }
    }*/
    cc++;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, mpTexImg);

}
/* \brief render floor image
 * \param bReload set true to reload texture
 *
 */
void Floor ::draw(bool bReload)
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

    //glDrawArrays(GL_TRIANGLE_STRIP, 0, mNumToDraw);
    glDrawElements(GL_TRIANGLES, mNumToDraw, GL_UNSIGNED_SHORT, 0 );

    glDisableVertexAttribArray(mVertexAttrib);
    glDisableVertexAttribArray(mUvAttrib);
}

/* \brief update basic matrics
 * \param pojection

 */
void Floor ::update(Mat4& pojection )
{
    m_matMvp = pojection;

}
