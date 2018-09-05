//
// Created by cj on 2018/9/3.
//

#ifndef NAVMES3_CUBE_H
#define NAVMES3_CUBE_H
#include "imglab/vecmath.h"
#include "common.h"
#include <GLES3/gl31.h>
#include <EGL/egl.h>


class Cube {
public:
    Cube();
    virtual ~Cube();

    bool init();
    void cleanup();
    /* \brief render floor image
     * \param bReload set true to reload texture
     *
     */
    void draw(bool bReload = false);

    /* \brief update basic matrics
     * \param pojection

     */
    void update(Mat4& pojection );
private:
    bool initVertexData();
    void updateTextureData();
    Mat4 m_matMvp;
    const EGLContext mEglContext;
    nfFloat3D* mpVertexBuf;         /*<! vertex coordinates, 3D */
    nfFloat2D* mpUvBuf;             /*<! texture UV coordinates, 2D */
    nfPByte  mpTextImg;   /*<! texture image data */
    unsigned int mNumToDraw;    /*<! number of elements to draw */

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


#endif //NAVMES3_CUBE_H
