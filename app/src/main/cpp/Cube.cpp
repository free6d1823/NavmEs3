//
// Created by cj on 2018/9/3.
//
#include <GLES3/gl31.h>
#include <EGL/egl.h>
#include <cstdlib>

#include "GlHelper.h"
#include "Cube.h"
#define LOG_TAG "CUBE"
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
Cube::Cube()
        :   mEglContext(eglGetCurrentContext()),
            mpVertexBuf(0), /*<! vertex coordinates, 3D */
            mpUvBuf(0),     /*<! texture UV coordinates, 2D */
            mpTextImg(0),   /*<! texture image data */
            mNumToDraw(0),  /*<! number of elements to draw */
            mProgramId(-1), /*<! Shader program ID */
            mTextureDataId(-1),
            mVertexAttrib(-1),      /*<! ID of Vertex position uniform */
            mUvAttrib(-1),    /*<! ID of texture uniform in shader program */
            mTextureUniform(-1),      /*<! ID of texture uniform */
            mMvpMatrixUniform(-1),       /*<! ID of matrix uniform in shader program */
            mVaoId(-1)  /*<! ID of Vertex array object*/

{
    mVertexBufId[0] = mVertexBufId[1] = -1;
    mVertexBufId[2] = -1;
}
Cube ::~Cube()
{
    cleanup();
}
void Cube ::cleanup()
{
    SAFE_FREE(mpVertexBuf);
    SAFE_FREE(mpUvBuf);
    SAFE_FREE(mpTextImg);
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

const nfFloat3D gpVertexBuf[] = {
        {-1.0f, -1.0f,  1.0f},{ 1.0f, -1.0f,  1.0f},{-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, //face 0
        {1.0f, -1.0f,  1.0f},{1.0f, -1.0f, -1.0f},{1.0f,  1.0f,  1.0f},{1.0f,  1.0f, -1.0f}, //face 1
        {1.0f, -1.0f, -1.0f},{-1.0f, -1.0f, -1.0f},{ 1.0f,  1.0f, -1.0f},{-1.0f,  1.0f, -1.0f}, //face 2
        {-1.0f, -1.0f, -1.0f},{-1.0f, -1.0f,  1.0f},{-1.0f,  1.0f, -1.0f},{-1.0f,  1.0f,  1.0f}, //face 3
        {-1.0f, -1.0f, -1.0f},{ 1.0f, -1.0f, -1.0f},{-1.0f, -1.0f,  1.0f},{1.0f, -1.0f,  1.0f}, //face 4
        {-1.0f,  1.0f,  1.0f},{1.0f,  1.0f,  1.0f},{-1.0f,  1.0f, -1.0f},{1.0f,  1.0f, -1.0f}, //face 5

};
static const nfFloat2D gpUvTexture[ ] = {
        {0.0f,0.0f}, {0.33f,0.0f}, {0,0.5f}, {.33f,0.5f}, //face 0
        {0.0f, 0.5f},{0.33f, 0.5f},{0.0f, 1.0f},{0.33f, 1.0f}, //face 1
        {0.66f, 0.5f},{1.0f, 0.5f},{0.66f, 1.0f},{1.0f, 1.0f}, //face 2
        {0.66f, 0.0f},{1.0f, 0.0f},{0.66f, 0.5f},{1.0f, 0.5f}, //face 3
        {0.33f, 0.0f},{0.66f, 0.0f},{0.33f, 0.5f},{0.66f, 0.5f}, //face 4
        {0.33f, 0.5f},{0.66f, 0.5f},{0.33f, 1.0f},{0.66f, 1.0f}, //face 5


};
static const unsigned short gpIndices[34] = {
        0,  1,  2,  3,  3,     // Face 0 - triangle strip ( v0,  v1,  v2,  v3)
        4,  4,  5,  6,  7,  7, // Face 1 - triangle strip ( v4,  v5,  v6,  v7)
        8,  8,  9, 10, 11, 11, // Face 2 - triangle strip ( v8,  v9, v10, v11)
        12, 12, 13, 14, 15, 15, // Face 3 - triangle strip (v12, v13, v14, v15)
        16, 16, 17, 18, 19, 19, // Face 4 - triangle strip (v16, v17, v18, v19)
        20, 20, 21, 22, 23      // Face 5 - triangle strip (v20, v21, v22, v23)
};

#define TEX_WIDTH   256
#define TEX_HEIGHT  256
bool Cube ::initVertexData()
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

    mNumToDraw = 34;
    return true;
}
bool Cube ::init()
{
    mpTextImg = (nfPByte) malloc(TEX_WIDTH*4*TEX_HEIGHT);
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
static unsigned char cc = 110;
void Cube ::updateTextureData()
{

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureDataId);

    for (int i=0; i<TEX_HEIGHT/2; i++) {
        for (int j=0; j<TEX_WIDTH/3; j++) {
            mpTextImg[i*TEX_WIDTH*4+ j*4 ] = 128; //R
            mpTextImg[i*TEX_WIDTH*4+ j*4 +1] = 228;
            mpTextImg[i*TEX_WIDTH*4+ j*4 +2] = (cc)%255; //B
            mpTextImg[i*TEX_WIDTH*4+ j*4 +3] = 255;
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH/3)*4 ] = cc%255; //R
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH/3)*4 +1] = 228;
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH/3)*4 +2] = 255-i; //B
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH/3)*4 +3] = 255;
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH*2/3)*4 ] = cc%255; //R
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH*2/3)*4 +1] = 228;
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH*2/3)*4 +2] = i; //B
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH*2/3)*4 +3] = 255;
        }
    }
    for (int i=TEX_HEIGHT/2; i<TEX_HEIGHT; i++) {
        for (int j=0; j<TEX_WIDTH/3; j++) {
            mpTextImg[i*TEX_WIDTH*4+ j*4 ] = 128; //R
            mpTextImg[i*TEX_WIDTH*4+ j*4 +1] = (cc)%255;
            mpTextImg[i*TEX_WIDTH*4+ j*4 +2] = 12; //B
            mpTextImg[i*TEX_WIDTH*4+ j*4 +3] = 255;
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH/3)*4 ] = cc%255; //R
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH/3)*4 +1] = 255-i;
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH/3)*4 +2] = 200; //B
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH/3)*4 +3] = 255;
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH*2/3)*4 ] = cc%255; //R
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH*2/3)*4 +1] = 228;
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH*2/3)*4 +2] = i; //B
            mpTextImg[i*TEX_WIDTH*4+ (j+TEX_WIDTH*2/3)*4 +3] = 255;
        }
    }

    cc++;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, mpTextImg);

}
/* \brief render floor image
 * \param bReload set true to reload texture
 *
 */
void Cube ::draw(bool bReload)
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

    glDrawElements(GL_TRIANGLE_STRIP, mNumToDraw, GL_UNSIGNED_SHORT, 0 );

    glDisableVertexAttribArray(mVertexAttrib);
    glDisableVertexAttribArray(mUvAttrib);
}

/* \brief update basic matrics
 * \param pojection

 */
void Cube ::update(Mat4& pojection )
{
    m_matMvp = pojection ;

}
