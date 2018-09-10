//
// Created by cj on 2018/9/3.
//
#include <GLES3/gl31.h>
#include <EGL/egl.h>
#include <cstdlib>
#include <cstring>

#include "GlHelper.h"
#include "Car.h"


#define LOG_TAG "CAR"
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
Parts::Parts(): mNumToDraw(0)
{
    memset(&mVertexBufId, 0, sizeof(mVertexBufId));
    mMatModle = Mat4::Identity(); //internal state matrix related to car
    mMatTransform = Mat4::Identity(); //car state related to world
}
Parts::~Parts()
{
    if(mVertexBufId[0]) {
        glDeleteBuffers(TOTAL_BUF, mVertexBufId);
    }

}
void Parts::draw(GLuint program, Mat4& pojection )
{
    GLuint mVertexAttrib = (GLuint) glGetAttribLocation(program, "vertexPosition");
    glEnableVertexAttribArray(mVertexAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[VERT_BUF]);
    glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(nfFloat3D), (const GLvoid*)0);

    GLuint mUvAttrib =  (GLuint)glGetAttribLocation(program, "vertexUv");
    glEnableVertexAttribArray(mUvAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[TEXT_BUF]);
    glVertexAttribPointer(mUvAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(nfFloat2D), (const GLvoid*)0);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexBufId[INDE_BUF]);


    Mat4 view = pojection * mMatTransform * mMatModle;

    GLint mMvpMatrixUniform = glGetUniformLocation(program, "mvp_matrix");
    glUniformMatrix4fv(mMvpMatrixUniform , 1, GL_FALSE, view.Ptr());

    glDrawElements(GL_TRIANGLES, mNumToDraw, GL_UNSIGNED_SHORT, 0 );

    glDisableVertexAttribArray(mVertexAttrib);
    glDisableVertexAttribArray(mUvAttrib);
}

/* \brief assigned vertex data from memory
 * \param data: vertex,uv,normal, index arrays
 * \param length: length of data in bytes
 * \return false if format of data is incorrect
 */
bool Parts::loadObject(void* data, unsigned int length)
{
    unsigned int na,nb,nc,nd;
    bool bOK = false;
    nfFloatBuffer* pVertexBuf = NULL;         /*<! vertex coordinates, 3D */
    nfFloatBuffer* pUvBuf = NULL;             /*<! texture UV coordinates, 2D */
    nfFloatBuffer* pNormBuf = NULL;             /*<! normal coordinates, 3D */
    nfUshortBuffer * pIndexBuf = NULL;/*<! index array */

    do {
        char* p = (char*) data;
        if ( memcmp("OBJ3", p, 4)!= 0){
            printf("wrong file tag !\n");
            break;
        }
        p+=4;
        na = *(unsigned int*)p; p+=sizeof(int);
        nb = *(unsigned int*)p; p+=sizeof(int);
        nc = *(unsigned int*)p; p+=sizeof(int);
        nd = *(unsigned int*)p; p+=sizeof(int);
        //LOGD("Size V:%d U:%d N:%d, i:%d\n", na,nb,nc,nd);
        if (na == 0 || nb != na ||nc != nb||nd ==0) {
            LOGE("verctor length field is not correct!!\n");
            break;
        }

        if ( memcmp("VERT", p, 4)!= 0){
            LOGE("not VERT tag !\n");
            break;
        }
        p+= 4;
        pVertexBuf = nfFloatBuffer::create(na*3);
        memcpy(pVertexBuf->data(), p, pVertexBuf->length()); p+= pVertexBuf->length();

        if ( memcmp("TEXT", p, 4)!= 0){
            LOGE("not TEXT tag !\n");
            break;
        }
        p+= 4;
        pUvBuf = nfFloatBuffer::create(nb*2);
        memcpy(pUvBuf->data(), p, pUvBuf->length()); p+= pUvBuf->length();

        if ( memcmp("NORM", p, 4)!= 0){
            LOGE("not NORM tag !\n");
            break;
        }
        p+=4;
        pNormBuf = nfFloatBuffer::create(nc*3);
        memcpy(pNormBuf->data(), p, pNormBuf->length()); p+= pNormBuf->length();

        if ( memcmp("INDE", p, 4)!= 0){
            LOGE("not INDE tag !\n");
            break;
        }
        p+=4;
        pIndexBuf =  nfUshortBuffer::create(nd);
        memcpy(pIndexBuf->data(), p, pIndexBuf->length()); p+= pIndexBuf->length();

        mNumToDraw = nd;
        bOK = true;
    }while (0);


    if (!bOK)
        return bOK;
    glGenBuffers(TOTAL_BUF, mVertexBufId);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[VERT_BUF]);
    glBufferData(GL_ARRAY_BUFFER, pVertexBuf->length(), pVertexBuf->data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[TEXT_BUF]);
    glBufferData(GL_ARRAY_BUFFER, pUvBuf->length(), pUvBuf->data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[NORM_BUF]);
    glBufferData(GL_ARRAY_BUFFER,  pNormBuf->length(), pNormBuf->data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexBufId[INDE_BUF]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, pIndexBuf->length(), pIndexBuf->data(), GL_STATIC_DRAW);

    nfFloatBuffer::destroy(&pVertexBuf) ;         /*<! vertex coordinates, 3D */
    nfFloatBuffer::destroy(&pUvBuf);             /*<! texture UV coordinates, 2D */
    nfFloatBuffer::destroy(&pNormBuf);
    nfUshortBuffer::destroy(&pIndexBuf);/*<! index array */

    return bOK;

}

Body::Body(int id) {
    mID = id;
}
Body::~Body() {
}
bool Body::loadObject(void* data, unsigned int length)
{
    return Parts::loadObject(data, length);
};
Wheels::Wheels(int id) :mAngle(0){
    mID = id;

}
Wheels::~Wheels() {
}
bool Wheels::loadObject(void* data, unsigned int length)
{
    if(mID == 1)
        mVecAxis = Vec3(-0.000818, 0.327574, -1.203252);
    else //rear
        mVecAxis = Vec3(0.001061, 0.327574, 1.156935);

    return Parts::loadObject(data, length);
};
void Wheels::draw(GLuint program, Mat4& pojection )
{
    GLuint mVertexAttrib = (GLuint) glGetAttribLocation(program, "vertexPosition");
    glEnableVertexAttribArray(mVertexAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[VERT_BUF]);
    glVertexAttribPointer(mVertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(nfFloat3D), (const GLvoid*)0);

    GLuint mUvAttrib =  (GLuint)glGetAttribLocation(program, "vertexUv");
    glEnableVertexAttribArray(mUvAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufId[TEXT_BUF]);
    glVertexAttribPointer(mUvAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(nfFloat2D), (const GLvoid*)0);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVertexBufId[INDE_BUF]);


    GLint mMvpMatrixUniform = glGetUniformLocation(program, "mvp_matrix");
    Mat4 obj = Mat4::Translation(0, mVecAxis.getY(), mVecAxis.getZ())* Mat4::RotationX(mAngle)* Mat4::Translation(0, -mVecAxis.getY(), -mVecAxis.getZ())* mMatModle;
    Mat4 view = pojection * mMatTransform * obj;
    mAngle += M_PI/90;
    if (mAngle > M_2_PI) mAngle -= M_2_PI;

    glUniformMatrix4fv(mMvpMatrixUniform , 1, GL_FALSE, view.Ptr());

    glDrawElements(GL_TRIANGLES, mNumToDraw, GL_UNSIGNED_SHORT, 0 );

    glDisableVertexAttribArray(mVertexAttrib);
    glDisableVertexAttribArray(mUvAttrib);
}
/**************************************************************************************************/
Car::Car()
        :   mEglContext(eglGetCurrentContext()),
            mBody(0),
            mFrontWheels(1),
            mRearWheels(2),
            mpTexImg(0),   /*<! texture image data */
            mProgramId(-1), /*<! Shader program ID */
            mTextureDataId(-1),
            mTextureUniform(-1),      /*<! ID of texture uniform */
            mVaoId(-1) /*<! ID of Vertex array object*/

{
}
Car ::~Car()
{
    cleanup();
}
void Car ::cleanup()
{
    SAFE_FREE(mpTexImg);

    if (mVaoId) {
        glDeleteVertexArrays(1, &mVaoId);
    }

    if(mProgramId != -1)                   /*<! Shader program ID */
        glDeleteProgram(mProgramId);
    if(mTextureDataId != -1)               /*<! ID of texture data */
        glDeleteTextures(1, &mTextureDataId);
}

/* \brief assigned vertex data from memory
     * \param id: 0 - car body, 1 - front, 2-rear wheels
     * \param data: vertex,uv,normal, index arrays
     * \param length: length of data in bytes
     * \return false if format of data is incorrect
     */
bool Car::loadObject(int id, void* data, unsigned int length)
{
    if (id == 1)
        return mFrontWheels.loadObject(data, length);
    else if (id == 2)
        return mRearWheels.loadObject(data, length);
    return mBody.loadObject(data, length);
}

bool Car ::init()
{
     mProgramId = CreateProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    if (!mProgramId)
        return false;
    mTextureUniform = glGetUniformLocation(mProgramId, "texture");

    glGenTextures(1, &mTextureDataId);
    updateTextureData();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glGenVertexArrays(1, &mVaoId);
    glBindVertexArray(mVaoId);

    Mat4 state = Mat4::Translation(0,0.605885,0);//initial car position related to world
    mFrontWheels.setState(state);
    mRearWheels.setState(state);
    mBody.setState(state); //Mat4::RotationY(M_PI);

    return true;
}
//called before init
nfPByte Car::allocTextureImage(int width, int height, int depth)
{
    SAFE_FREE(mpTexImg);
    mTexWidth = width;
    mTexHeight = height;
    mTexDepth = depth;
    mpTexImg = (nfPByte) malloc(mTexWidth*mTexHeight*mTexDepth);
    return mpTexImg;
}

void Car ::updateTextureData()
{

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureDataId);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, mpTexImg);

}
/* \brief render floor image
 * \param bReload set true to reload texture
 *
 */
void Car ::draw(bool bReload)
{
    glUseProgram(mProgramId);
    checkGlError("RendererES2--::glUseProgram");
    updateTextureData();
    glUniform1i(mTextureUniform, 0);//"texture" to use first texture data
    glBindBuffer(GL_ARRAY_BUFFER, mTextureDataId);
    mBody.draw(mProgramId, m_matMvp);
    mFrontWheels.draw(mProgramId, m_matMvp);
    mRearWheels.draw(mProgramId, m_matMvp);

}

/* \brief update basic matrics
 * \param pojection

 */
void Car ::update(Mat4& pojection )
{
    m_matMvp = pojection*Mat4::RotationY(M_PI) ;

}
