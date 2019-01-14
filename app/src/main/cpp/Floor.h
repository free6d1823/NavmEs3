//
// Created by cj on 2018/9/3.
//

#ifndef NAVMES3_FLOOR_H
#define NAVMES3_FLOOR_H
#include "imglab/vecmath.h"
#include "common.h"
#include "imglab/ImgProcess.h"
#include <GLES3/gl31.h>
#include <EGL/egl.h>

class CameraSource;
class Floor {
public:
    Floor();
    virtual ~Floor();

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

    /* \brief use video file to simulate, called before init
     * \param width width of the image
     * \param height height of the image
     * \param depth bytes per pixel = 4
     * \param szFile YUYV file
     */
    void setSimVideoFile(int width, int height, int depth, const char* szFile);
    /* \brief use video file to simulate, called before init
     * \param width width of the image
     * \param height height of the image
     * \param depth bytes per pixel = 4
     * \param szFile RGB32 file
     */
    void setSimVideoFileRgb(int width, int height, int depth, const char* szFile);
    void setOption(int nOption);
    virtual bool saveTexture(const char* filepath);
private:
    bool initVertexData();
    void updateTextureData();
    bool loadTexture();


    Mat4 m_matMvp;
    const EGLContext mEglContext;
    nfFloat3D* mpVertexBuf;         /*<! vertex coordinates, 3D */
    nfFloat2D* mpUvBuf;             /*<! texture UV coordinates, 2D */
    unsigned int mNumToDraw;    /*<! number of elements to draw */

    bool  mInit;

    nfPByte  mpTexImg;   /*<! texture image data */
    int mTexWidth;
    int mTexHeight;
    int mTexDepth;
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


#endif //NAVMES3_FLOOR_H
