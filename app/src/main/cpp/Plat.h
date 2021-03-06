//
// Created by cj on 2018/9/3.
//

#ifndef NAVMES3_PLAT_H
#define NAVMES3_PLAT_H
#include "imglab/vecmath.h"
#include "common.h"
#include "imglab/ImgProcess.h"
#include <GLES3/gl31.h>
#include <EGL/egl.h>


class Plat {
public:
    Plat();
    virtual ~Plat();

    bool init();
    void cleanup();
    /* \brief render Plat image
     * \param bReload set true to reload texture
     *
     */
    void draw(bool bReload = false);

    /* \brief update basic matrics
     * \param pojection
     */
    void update(Mat4& pojection );

    /* \brief use video file to simulate, called before init
     * \param width width of the image
     * \param height height of the image
     * \param depth bytes per pixel = 4
     * \param szFile YUYV file
     */
private:
    bool initVertexData();
    void updateTextureData();
    bool loadTexture();


    Mat4 m_matMvp;
    const EGLContext mEglContext;
    nfFloat3D* mpVertexBuf;         /*<! vertex coordinates, 3D */
    nfFloat2D* mpUvBuf;             /*<! texture UV coordinates, 2D */
    unsigned int mNumToDraw;    /*<! number of elements to draw */

    nfPByte  mpTexImg;   /*<! texture image data */
    int mTexWidth;
    int mTexHeight;
    /*<! OpenGL ID */
    GLuint mProgramId;          /*<! Shader program ID */
    GLuint mTextureDataId;      /*<! texture buffer ID */
    GLuint mVertexAttrib;      /*<! ID of Vertex position attribute */
    GLuint mUvAttrib;         /*<! ID of texture attrib in shader program */
    GLuint mTextureUniform;      /*<! ID of texture uniform */
    GLuint mMvpMatrixUniform;        /*<! ID of matrix uniform in shader program */
    GLuint mVaoId;              /*<! ID of Vertex array object*/
    GLuint mVertexBufId[3];     /*<! buffer ID: 0=Vertex, 1=UV texture, buffer */


};


#endif //NAVMES3_PLAT_H
