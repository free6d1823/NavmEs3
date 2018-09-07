//
// Created by cj on 2018/9/3.
//

#ifndef NAVMES3_CAR_H
#define NAVMES3_CAR_H
#include "imglab/vecmath.h"
#include "./imglab/ImgProcess.h"
#include "common.h"
#include <GLES3/gl31.h>
#include <EGL/egl.h>

#define VERT_BUF   0
#define TEXT_BUF    1
#define NORM_BUF    2
#define INDE_BUF    3
#define TOTAL_BUF   4

class Parts
{
public:
    Parts();
    virtual ~Parts();
    /* \brief assigned vertex data from memory
     * \param data: vertex,uv,normal, index arrays
     * \param length: length of data in bytes
     * \return false if format of data is incorrect
     */
    bool loadObject(void* data, unsigned int length);
    void transform(Mat4& transform){mMatTransform = transform;}
    void setState(Mat4& newState){mMatModle = newState;}
    void draw(GLuint program, Mat4& pojection );
protected:

    GLuint mVertexBufId[TOTAL_BUF];     /*<! buffer ID: 0=Vertex, 1=UV texture, buffer */

    int mNumToDraw;
    Mat4  mMatModle; //state inside car
    Mat4  mMatTransform; //whole car state
    int mID; /*<! 0 - car body, 1 - front, 2-rear wheels */

};
class Body : public Parts
{
public:
    Body(int nID);
    virtual ~Body();
    bool loadObject(void* data, unsigned int length);

};
class Wheels : public Parts
{
public:
    Wheels(int nID);
    virtual ~Wheels();
    bool loadObject(void* data, unsigned int length);
    void draw(GLuint program, Mat4& pojection );

private:
    Vec3    mVecAxis;
    float   mAngle; /* current wheel rotation angle */
};

class Car {
public:
    Car();
    virtual ~Car();

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

    /* \brief alloc texture buffer
     * \param width width of the image
     * \param height height of the image
     * \param depth bytes per pixel
     * \return pointer to the buffer
     */
    nfPByte allocTextureImage(int width, int height, int depth);

    /* \brief assigned vertex data from memory
     * \param id: 0 - car body, 1 - front, 2-rear wheels
     * \param data: vertex,uv,normal, index arrays
     * \param length: length of data in bytes
     * \return false if format of data is incorrect
     */
    bool loadObject(int id, void* data, unsigned int length);
private:
    bool initVertexData();
    void updateTextureData();

    const EGLContext mEglContext;
    Body    mBody;
    Wheels  mFrontWheels;
    Wheels  mRearWheels;

    Mat4 m_matMvp;


    nfPByte  mpTexImg;   /*<! texture image data */
    int mTexWidth;
    int mTexHeight;
    int mTexDepth;
    /*<! OpenGL ID */
    GLuint mProgramId;          /*<! Shader program ID */
    GLuint mTextureDataId;      /*<! texture buffer ID */
    GLint mTextureUniform;      /*<! ID of texture uniform */
    GLuint mVaoId;              /*<! ID of Vertex array object*/
};


#endif //NAVMES3_CAR_H
