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
#include "CameraSource.h"

CameraSource gCameraSource;

extern TexProcess gTexProcess;
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


Floor::Floor()
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
Floor ::~Floor()
{
    cleanup();
}
void Floor ::cleanup()
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


bool Floor ::initVertexData()
{
    vector <nfFloat3D> vert;
    vector <unsigned short> ind;
    vector <nfFloat2D> uvs;
    gTexProcess.createVertices(vert, ind);
    if (0)
        gTexProcess.updateUvNoFisheye(uvs);
    else
        gTexProcess.updateUv(uvs);
    gTexProcess.reloadIndices(ind);
    glGenVertexArrays(1, &mVaoId);
    glBindVertexArray(mVaoId);


    glGenBuffers(3, mVertexBufId);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[0]);
    glBufferData(GL_ARRAY_BUFFER, vert.size() * sizeof(nfFloat3D), &vert[0], GL_STATIC_DRAW);

//
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[1]);
    glBufferData(GL_ARRAY_BUFFER, uvs.size()* sizeof(nfFloat2D), &uvs[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexBufId[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ind.size()* sizeof(unsigned short), &ind[0], GL_STATIC_DRAW);

    mNumToDraw = ind.size();

    LOGI("-------- v=%d, U=%d i=%d", vert.size(), uvs.size(), ind.size());
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

    if(!gCameraSource.init())
        return false;
    mTexWidth = gCameraSource.Width();
    mTexHeight = gCameraSource.Height();
    mTexDepth = 32; //RGBA32
    mpTexImg = NULL; //buffer is provided by CameraSource

    glGenTextures(1, &mTextureDataId);
    updateTextureData();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    return true;
}

/* \brief use video file to simulate, called before init
 *
 */
void Floor ::setSimVideoFile(int width, int height, int depth, const char* szFile)
{
    gCameraSource.setSimFileYuv(width, height, 2, szFile);
    LOGE("Use sim YUV file %s as video input %dx%d", szFile, width, height);
}
void Floor ::setSimVideoFileRgb(int width, int height, int depth, const char* szFile)
{

    gCameraSource.setSimFileRgb32(width, height, 4, szFile);
    LOGE("Use sim RGB file %s as video input %dx%d", szFile, width, height);
}
void Floor ::updateTextureData()
{
    loadTexture();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureDataId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, mpTexImg);

}
bool Floor ::loadTexture()
{
    mpTexImg = gCameraSource.GetFrameData();
    return true;
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
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexBufId[2]);

    glUniformMatrix4fv(mMvpMatrixUniform , 1, GL_FALSE, m_matMvp.Ptr());

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
