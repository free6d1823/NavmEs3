#include "ImgProcess.h"
#include "Mat.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef ANDROID
#include <android/log.h>
#endif
#define  LOG_TAG    "Floor"
#include "../common.h"
#include "../inifile/inifile.h"



#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)


// YUV -> RGB
#define C(Y) ( (Y) - 16  )
#define D(U) ( (U) - 128 )
#define E(V) ( (V) - 128 )

#define YUV2R(Y, U, V) CLIP(( 298 * C(Y)              + 409 * E(V) + 128) >> 8)
#define YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
#define YUV2B(Y, U, V) CLIP(( 298 * C(Y) + 516 * D(U)              + 128) >> 8)

//////
/// \brief ImgProcess::YuyvToRgb32 YUV420 to RGBA 32
/// \param pYuv input image
/// \param pRgb     output RGB32 buffer, must be allocated by caller before call
/// \param uFirst   true if pYuv is YUYV, false if YVYU
///
void nfYuyvToRgb32(nfImage* pYuv, unsigned char* pRgb, bool uFirst, bool bMirror)
{
    //YVYU - format
    int nBps = pYuv->width*4;//stride in RGB data
    unsigned char* pY1 = pYuv->buffer;

    unsigned char* pV;
    unsigned char* pU;

    int nStride = pYuv->stride;
    if (bMirror) {
        pY1 = pYuv->buffer +(pYuv->height-1)* pYuv->stride;
        nStride = -pYuv->stride;
    }

    if (uFirst) {
        pU = pY1+1; pV = pU+2;
    } else {
        pV = pY1+1; pU = pV+2;
    }


    unsigned char* pLine1 = pRgb;

    unsigned char y1,u,v;
    for (unsigned int i=0; i<pYuv->height; i++)
    {
        for (unsigned int j=0; j<pYuv->width; j++)
        {
            y1 = pY1[2*j];
            u = pU[2*j];
            v = pV[2*j];
            pLine1[j*4] = YUV2B(y1, u, v);//b
            pLine1[j*4+1] = YUV2G(y1, u, v);//g
            pLine1[j*4+2] = YUV2R(y1, u, v);//r
            pLine1[j*4+3] = 0xff;
            j++;
            y1 = pY1[2*j];
            pLine1[j*4] = YUV2B(y1, u, v);//b
            pLine1[j*4+1] = YUV2G(y1, u, v);//g
            pLine1[j*4+2] = YUV2R(y1, u, v);//r
            pLine1[j*4+3] = 0xff;
        }
        pY1 += nStride;
        pV += nStride;
        pU += nStride;
        pLine1 += nBps;

    }
}
/****************************************************************/
nfImage* nfImage::create(unsigned int w, unsigned int h, unsigned int bpp)
{
    nfImage* img =  new nfImage;
    if (!img)
        return img;

    img->width = w;
    img->stride = w*bpp;
    img->height = h;
    img->bRef = false;
    img->buffer = (unsigned char*)malloc( img->stride*h);
    if (!img->buffer ) {
        delete img;
        return NULL;
    }

//    printf ("nfInitImage#%d (%dx%d)\n", img->seq, w, h);
    return img;
}
nfImage* nfImage::ref(unsigned char* data, unsigned int w, unsigned int h, unsigned int stride)
{
    nfImage* img =  new nfImage;
    if (!img)
        return img;

    img->buffer = data;
    img->width = w;
    img->stride = stride;
    img->height = h;
    img->bRef = true;
//    printf ("nfRefImage#%d (%dx%d)\n", img->seq, w, h);
    return img;
}
nfImage* nfImage::clone(nfImage* pSource)
{
    nfImage* img =  new nfImage;
    if (!img)
        return img;
    img->buffer = pSource->buffer;
    img->width = pSource->width;
    img->stride = pSource->stride;
    img->height = pSource->height;
    return img;
}

void nfImage::destroy(nfImage** ppImage)
{
    if(*ppImage) {
        if(!(*ppImage)->bRef)
            SAFE_FREE((*ppImage)->buffer);
        delete (*ppImage);
        *ppImage = NULL;
    }
}
nfPByte nfImage::dettach(nfImage** ppImage)
{
    nfPByte pBuffer = NULL;
    if(*ppImage) {
        pBuffer = (*ppImage)->buffer;
        delete (*ppImage);
        *ppImage = NULL;
    }
    return pBuffer;
}
/****************************************************************/
nfBuffer::nfBuffer(unsigned int /*elements*/)
{
}
nfBuffer* nfBuffer::create(unsigned int counts)
{
    return new nfBuffer(counts);
}

void nfBuffer::destroy(nfBuffer** /*ppBuffer*/)
{
}
nfBuffer::~nfBuffer()
{
}
/****************************************************************/
nfFloatBuffer::nfFloatBuffer(unsigned int elements)
{
    mBpu = sizeof(float);
    mCounts = elements;
    mLength = mBpu* elements;
    mpData = (float*) malloc(mLength);
//    LOGI("nfFloatBuffer new %dx%d=%d mpData=%p", mBpu, elements, mLength, mpData);
}
nfFloatBuffer::~nfFloatBuffer()
{
    if (mpData)
        free (mpData);
}
nfFloatBuffer* nfFloatBuffer::create(unsigned int elements)
{
    return new nfFloatBuffer(elements);
}
void nfFloatBuffer::destroy(nfFloatBuffer** ppBuffer)
{
    if(*ppBuffer) {
        delete (*ppBuffer);
        *ppBuffer = NULL;
    }
}
/****************************************************************/
nfUshortBuffer::nfUshortBuffer(unsigned int elements)
{
    mBpu = sizeof(unsigned int);
    mCounts = elements;
    mLength = mBpu* elements;
    mpData = (unsigned short*) malloc(mLength);
//    LOGI("nfUshortBuffer new %dx%d=%d mpData=%p", mBpu, elements, mLength, mpData);
}
nfUshortBuffer::~nfUshortBuffer()
{
    if (mpData)
        free (mpData);
}
nfUshortBuffer* nfUshortBuffer::create(unsigned int elements)
{
    return new nfUshortBuffer(elements);
}
void nfUshortBuffer::destroy(nfUshortBuffer** ppBuffer)
{
    if(*ppBuffer) {
        delete (*ppBuffer);
        *ppBuffer = NULL;
    }
}


/* Fisheye model: Q =R*theda; P =f*tan(theda)=f*tan(Q/r)
 * boundary condition: R=0.5/wf, f = 0.5/tan(wr)
 * Q = 1/(2* wf) * atan(2* P * tan(wr))  where
 * Q = sqrt(x,y), x,y[-0.5,0.5] is pixel in fisheye,
 * P = sqrt(u,v), u,v[-0.5,0.5] is location at rectified image
 * wf is fisheye FOV/2, assume 90 degree
 * wr is fov/2 on rectified image
*/
#define WF  M_PI_2

void nfDoFec(float u, float v, float &x, float &y, FecParam* pFec)
{
    double  u1, v1;
    double x1,y1;
    //transform uu = M x u
    u1 = u-0.5;
    v1 = v-0.5;
    //////
    /// \brief do PTZ first
    ///
    double u2,v2;
    double z = sin(pFec->pitch)*v1 + cos(pFec->pitch);
    u2 = u1/z;
    v2 = ( cos(pFec->pitch)*v1 - sin(pFec->pitch))/z;
    //yaw
    z = -sin(pFec->yaw)*u2 + cos(pFec->yaw);
    u2 = ( cos(pFec->yaw)*u2  + sin(pFec->yaw))/z;
    v2 = v2/z;

    //spin - z-axis
    u1 = cos(pFec->roll)* u2 - sin(pFec->roll)*v2;
    v1 = sin(pFec->roll)* u2 + cos(pFec->roll)*v2;


    double fr = 2*tan(pFec->fov/2); //reverse of focus length of rectified image
    double rp = sqrt(u1*u1+v1*v1); //radius of point (u1,v1) on rectified image
    double rq;	//fisheye

    double rq1;

    if(1) //fisheye
        rq1 = atan(rp*fr)/(pFec->fov);   // [0.5]  modified by jason huang 20181128
    else //normal lens
        rq1 = rp; //if no fec


    if (rp <= 0) {
        x1 = y1 = 0;
    } else {
        u1  = u1 * rq1/rp;
        v1  = v1 * rq1/rp;
    //LDC
        double rq2 = rq1*rq1;
        rq  = (1+pFec->k1* rq2+ pFec->k2* rq2*rq2);
        x1 = u1/rq;
        y1 = v1/rq;
        //DO intrinsic after LDC   add by jason huang 20181128 ---
        x1 = pFec->a*x1 + pFec->c*y1;		//x-scale and de-skewness
        y1 = pFec->b*y1;                    //y- scale
        // ------------------------------------------------
    }

    x = (x1+ pFec->ptCenter.x);
    y = (y1+ pFec->ptCenter.y);

}
void nfInvFec(float x, float y, float &u, float &v, FecParam* pFec)
{
    float x1,y1;
    x1 = x - pFec->ptCenter.x;
    y1 = y - pFec->ptCenter.y;

    //Do intrinsic before LDC  jason huang 20181228
    y1 = y1 / pFec->b;
    x1 = (x1 - pFec->c*y1)/pFec->a;

    //LDC -- a approcimate invers of LDC
    double rq, rq1, rq2, rp;
    rq2 = x1*x1 + y1*y1;
    double rqt = sqrt(rq2);
    //this approach has error less than 0.1% for small k
    rq  = (1-pFec->k1* rq2- pFec->k2* rq2*rq2);

    double u1,v1,u2,v2;
    u1 = x1 /rq;
    v1 = y1 /rq;
    rq1 = rqt/rq; //rq1 = atan(rp*fr)/(2*WF);		//[0.5]
    double fr = 2*tan(pFec->fov/2); //reverse of focus length of rectified image
    rp = tan(rq1*pFec->fov)/fr; // rp = tan(rq1*2*WF)/fr; jason huang 20181228
    u1 = u1 * rp/ rq1;
    v1 = v1 * rp/rq1;


    //rev spin
    u2 = cos(pFec->roll)* u1 + sin(pFec->roll)*v1;
    v2 = - sin(pFec->roll)* u1 + cos(pFec->roll)*v1;
    //rev
    double z;
    //yaw
    z = 1/(sin(pFec->yaw)*u2 + cos(pFec->yaw));
    u2 = (z*u2 - sin(pFec->yaw))/cos(pFec->yaw);
    v2 = v2*z;
    //rev pitch
    z = 1 /(cos(pFec->pitch) - sin(pFec->pitch)*v2);
    u1 = u2 * z;
    v1 = (v2*z + sin(pFec->pitch) )/cos(pFec->pitch);
    u = u1 + 0.5;
    v = v1 + 0.5;
}

bool nfDoHomoTransform(float s, float t, float &u, float &v, float h[3][3])
{
    u = h[0][0] * s +h[0][1]*t + h[0][2];
    v = h[1][0] * s + h[1][1]*t + h[1][2];
    float scale = h[2][0] * s + h[2][1]*t + 1;
    if (scale != 0) {
        u = u/scale;
        v = v/scale;
//        if(u>=1 || v>=1 ||u<0 ||v <0)
//            return false;
        return true;
    }
    return false;
}

void nfFindHomoMatrix(nfFloat2D s[4], nfFloat2D t[4], float hcoef[3][3])
{
    int i;
    //normalize source FP
    //Ah=b
    Mat A(8,8);
    Mat b(1,8);
    int j=4;
    for (i=0;i<4;i++) {
        A.Set(0,i, t[i].x); A.Set(1, i, t[i].y); A.Set(2,i,1); A.Set(3,i, 0); A.Set(4,i, 0); A.Set(5,i, 0);
        A.Set(6,i, -t[i].x*s[i].x); A.Set(7, i, -t[i].y*s[i].x);
        b.Set(0,i, s[i].x);

        A.Set(0,j, 0); A.Set(1,j, 0); A.Set(2,j, 0);
        A.Set(3,j, t[i].x); A.Set(4, j, t[i].y); A.Set(5,j,1);
        A.Set(6,j, -t[i].x*s[i].y); A.Set(7, j, -t[i].y*s[i].y);
        b.Set(0,j, s[i].y);
        j++;
    }
    Mat h (1,8);
    Mat AI(8,8);
    if(A.FindInverse(AI)) {
        h.Multiply(AI, b);
        for(int i=0;i<8;i++)
            hcoef[i/3][i%3] = 	h.Get(0,i);
        hcoef[2][2] = 1;

    }else {
        for(int i=0;i<8;i++)
            hcoef[i/3][i%3] = -1;
    }
}

/*<! 3-regions feature points indexing , each rect is clock-wised numbering
 *   0 -------- 1 -------- 2 -------- 3
 *   | region 0 | region 1 | region 2 |
 *   4 -------- 5 -------- 6 -------- 7 */

void nfCalculateHomoMatrix3(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo)
{
    nfFloat2D s[3][4] = {{fps[0], fps[1], fps[5], fps[4]}, /*<! source ( uncorrected) region 0 */
                         {fps[1], fps[2], fps[6], fps[5]},
                         {fps[2], fps[3], fps[7], fps[6]}};
    nfFloat2D t[3][4] = {{fpt[0], fpt[1], fpt[5], fpt[4]}, /*<! corrected region 0 */
                         {fpt[1], fpt[2], fpt[6], fpt[5]},
                         {fpt[2], fpt[3], fpt[7], fpt[6]}};

    for(int i=0; i<3; i++) {
        nfFindHomoMatrix(s[i],t[i], homo[i].h);
    }

}
/*<! 3-regions feature points indexing , each rect is clock-wised numbering
 *   0 -------- 1 -------- 2 -------- 3
 *   | region 0 | region 1 | region 2 |
 *   4 -------- 5 -------- 6 -------- 7 */

void nfCalculateHomoMatrix3a(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo)
{
    nfFloat2D s[3][4] = {{fps[1], fps[2], fps[6], fps[5]}, /*<! source ( uncorrected) region 0 */
                         {fps[1], fps[2], fps[6], fps[5]},
                         {fps[1], fps[2], fps[6], fps[5]}};
    nfFloat2D t[3][4] = {{fpt[1], fpt[2], fpt[6], fpt[5]}, /*<! corrected region 0 */
                         {fpt[1], fpt[2], fpt[6], fpt[5]},
                         {fpt[1], fpt[2], fpt[6], fpt[5]}};

    for(int i=0; i<3; i++) {
        nfFindHomoMatrix(s[i],t[i], homo[i].h);
    }

}

/*<! 4-regions feature points indexing , each rect is clock-wised numbering
 *   0 -------- 1 -------- 2 -------- 3 ---------4
 *   | region 0 | region 1 | region 2 | region 3 |
 *   5 -------- 6 -------- 7 -------- 8 ---------9 */

void nfCalculateHomoMatrix4(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo)
{
    nfFloat2D s[4][4] = {{fps[0], fps[1], fps[6], fps[5]}, /*<! source ( uncorrected) region 0 */
                                 {fps[1], fps[2], fps[7], fps[6]},
                                 {fps[2], fps[3], fps[8], fps[7]},
                                 {fps[3], fps[4], fps[9], fps[8]}};
    nfFloat2D t[4][4] = {{fpt[0], fpt[1], fpt[6], fpt[5]}, /*<! corrected region 0 */
                                 {fpt[1], fpt[2], fpt[7], fpt[6]},
                                 {fpt[2], fpt[3], fpt[8], fpt[7]},
                                 {fpt[3], fpt[4], fpt[9], fpt[8]}};

    for(int i=0; i<4; i++) {
        nfFindHomoMatrix(s[i],t[i], homo[i].h);
    }

}
/*<! 10-regions, 18feature points indexing , each rect is clock-wised numbering
 *   0 -------- 1 -------- 2 -------- 3 ---------4 ---------5
 *   | region 0 | region 1 | region 2 | region 3 |    4     |
 *   6 -------- 7 -------- 8 -------- 9 -------- 10 --------11
 *   | region 5 | region 6 | region 7 | region 8 |    9    |
 *   12 ------- 13 -------14 --------15-------- 16-------- 17
 **/
void nfCalculateHomoMatrix10(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo)
{
    int k[10][4] = {
        {0,1,7,6},{1,2,8,7},{2,3,9,8},{3,4,10,9},{4,5,11,10},
        {6,7,13,12},{7,8,14,13}, {8,9,15,14}, {9,10,16,15},{10,11,17,16},
    };

    for(int i=0; i<10; i++) {
        nfFloat2D s[4] = {fps[k[i][0]],fps[k[i][1]], fps[k[i][2]], fps[k[i][3]]};
        nfFloat2D t[4] = {fpt[k[i][0]],fpt[k[i][1]], fpt[k[i][2]], fpt[k[i][3]]};
        nfFindHomoMatrix(s,t, homo[i].h);
    }

}
/*<! 12-regions, 21feature points indexing , each rect is clock-wised numbering
 *   0 -------- 1 -------- 2 -------- 3 ---------4 ---------5 -------- 6
 *   | region 0 | region 1 | region 2 | region 3 |    4     |    5     |
 *   7 -------- 8 -------- 9 -------- 10 --------11 --------12 ------- 13
 *   | region 6 | region 7 | region 8 | region 9 |    10    |    11    |
 *   14 --------15-------- 16-------- 17 --------18 --------19 ------- 20
 **/
void nfCalculateHomoMatrix12(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo)
{
    nfFloat2D s[12][4] = {{fps[0], fps[1], fps[8], fps[7]}, /*<! source ( uncorrected) region 0 */
                          {fps[1], fps[2], fps[9], fps[8]},
                          {fps[2], fps[3], fps[10], fps[9]},
                          {fps[3], fps[4], fps[11], fps[10]},
                          {fps[4], fps[5], fps[12], fps[11]},
                          {fps[5], fps[6], fps[13], fps[12]},
                          {fps[7], fps[8], fps[15], fps[14]},
                          {fps[8], fps[9], fps[16], fps[15]},
                          {fps[9], fps[10], fps[17], fps[16]},
                          {fps[10], fps[11], fps[18], fps[17]},
                          {fps[11], fps[12], fps[19], fps[18]},
                          {fps[12], fps[13], fps[20], fps[19]},
                         };
    nfFloat2D t[12][4] = {{fpt[0], fpt[1], fpt[8], fpt[7]}, /*<! corrected region 0 */
                          {fpt[1], fpt[2], fpt[9], fpt[8]},
                          {fpt[2], fpt[3], fpt[10], fpt[9]},
                          {fpt[3], fpt[4], fpt[11], fpt[10]},
                          {fpt[4], fpt[5], fpt[12], fpt[11]},
                          {fpt[5], fpt[6], fpt[13], fpt[12]},
                          {fpt[7], fpt[8], fpt[15], fpt[14]},
                          {fpt[8], fpt[9], fpt[16], fpt[15]},
                          {fpt[9], fpt[10], fpt[17], fpt[16]},
                          {fpt[10], fpt[11], fpt[18], fpt[17]},
                          {fpt[11], fpt[12], fpt[19], fpt[18]},
                          {fpt[12], fpt[13], fpt[20], fpt[19]},
                         };
    for(int i=0; i<12; i++) {
        nfFindHomoMatrix(s[i],t[i], homo[i].h);
    }

}
/*<! 14-regions, 24  feature points indexing , each rect is clock-wised numbering
 *   0 -------- 1 -------- 2 -------- 3 ---------4 ---5-- 6---7
 *   | region 0 | region 1 | region 2 | region 3 |  4 | 5 | 6 |
 *   8 -------- 9-------- 10-------  11 --------12----13--14--15
 *   | region 7 | region 8 | region 9| region 10| 11 |12 |13 |
 *   16 --------17-------- 18--------19------- 20- --21--22--23
 **/
void nfCalculateHomoMatrix14(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo)
{
    int k[14][4] = {
        {0,1,9,8},{1,2,10,9},{2,3,11,10},{3,4,12,11},{4,5,13,12},{5,6,14,13},{6,7,15,14},
        {8,9,17,16}, {9,10,18,17},{10,11,19,18},{11,12,20,19},{12,13,21,20},{13,14,22,21},{14,15,23,22}
    };

    for(int i=0; i<14; i++) {
        nfFloat2D s[4] = {fps[k[i][0]],fps[k[i][1]], fps[k[i][2]], fps[k[i][3]]};
        nfFloat2D t[4] = {fpt[k[i][0]],fpt[k[i][1]], fpt[k[i][2]], fpt[k[i][3]]};
        nfFindHomoMatrix(s,t, homo[i].h);
    }

}

/*<! 16-regions, 27  feature points indexing , each rect is clock-wised numbering
 *   0 -------- 1 -------- 2 -------- 3 ---------4 ---5-- 6---7---8
 *   | region 0 | region 1 | region 2 | region 3 |  4 | 5 | 6 | 7 |
 *   9 -------- 10-------- 11------- 12 --------13----14--15--16--17
 *   | region 8 | region 9 | region 10| region 11| 12 |13 |14 |15 |
 *   18 --------19-------- 20-------- 21-------22---23---24--25--26
 **/
void nfCalculateHomoMatrix16(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo)
{
    int k[16][4] = {
        {0,1,10,9},{1,2,11,10},{2,3,12,11},{3,4,13,12},{4,5,14,13},{5,6,15,14},{6,7,16,15},{7,8,17,16},
        {9,10,19,18},{10,11,20,19},{11,12,21,20},{12,13,22,21},{13,14,23,22},{14,15,24,23},{15,16,25,24},{16,17,26,25},
    };

    for(int i=0; i<16; i++) {
        nfFloat2D s[4] = {fps[k[i][0]],fps[k[i][1]], fps[k[i][2]], fps[k[i][3]]};
        nfFloat2D t[4] = {fpt[k[i][0]],fpt[k[i][1]], fpt[k[i][2]], fpt[k[i][3]]};
        nfFindHomoMatrix(s,t, homo[i].h);
    }

}

/*<! 20-regions, 33  feature points indexing , each rect is clock-wised numbering
 *   0 - 1 - 2 - 3 --4 --5-- 6---7---8---9--10
 *   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
 *   11--12--13--14--15--16--17--18--19--20--21
 *   | 10| 11| 12| 13| 14| 15| 16| 17| 18| 19|
 *   22--23--24--25--26--27--28--29--30--31--32
 **/
void nfCalculateHomoMatrix20(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo)
{
    int k[20][4] = {
        {0,1,12,11},{1,2,13,12},{2,3,14,13},{3,4,15,14},{4,5,16,15},
            {5,6,17,16},{6,7,18,17},{7,8,19,18},{8,9,20,19},{9,10,21,20},
        {11,12,23,22},{12,13,24,23},{13,14,25,24},{14,15,26,25},{15,16,27,26},
            {16,17,28,27},{17,18,29,28},{18,19,30,29},{19,20,31,30},{20,21,32,31},
    };

    for(int i=0; i<20; i++) {
        nfFloat2D s[4] = {fps[k[i][0]],fps[k[i][1]], fps[k[i][2]], fps[k[i][3]]};
        nfFloat2D t[4] = {fpt[k[i][0]],fpt[k[i][1]], fpt[k[i][2]], fpt[k[i][3]]};
        nfFindHomoMatrix(s,t, homo[i].h);
    }

}
/////////////////////////////////////
/// \brief s_offsetCam each camera position in video frame
/// |-------|------|
/// | front | right|
/// |-------|------|(1,0.5)
/// | left  | rear |
/// |-------|------|(1,1)
///


nfFloat2D TexProcess::s_offsetCam[MAX_CAMERAS]={ {0,0}, {0.5, 0},
                          {0.5, 0.5}, {0, 0.5} };

bool    LoadFecParam(FecParam* pFecParam, int nArea, void* iniFile)
{
    char section[32];
    sprintf(section, "fecparam_%d", nArea);

    if(!GetProfilePointFloat(section, "ptCenter", &pFecParam->ptCenter, iniFile))
    {
        pFecParam->ptCenter.x = 0.5;
        pFecParam->ptCenter.y = 0.5;
    }
    pFecParam->fov = GetProfileFloat(section, "fov", (170.0* M_PI/180.0), iniFile);
    pFecParam->k1 = GetProfileFloat(section, "k1", -8.0, iniFile);
    pFecParam->k2 = GetProfileFloat(section, "k2", 0.0, iniFile);
    pFecParam->a = GetProfileFloat(section, "a", 1.0, iniFile);
    pFecParam->b = GetProfileFloat(section, "b", 1.0, iniFile);
    pFecParam->c = GetProfileFloat(section, "c", 0.0, iniFile);
    pFecParam->pitch = GetProfileFloat(section, "pitch", 0.0, iniFile);
    pFecParam->yaw = GetProfileFloat(section, "yaw", 0.0, iniFile);
    pFecParam->roll = GetProfileFloat(section, "roll", 0.0, iniFile);

    return true;
}
/* please real time calculate homo coefficients by fpf and fpt */
bool    LoadHomoParam(HomoParam* pParam, int nArea, int nFp, void* iniFile)
{

    char section[32];

    sprintf(section,"homoparam_%d_%d", nArea, nFp);
    if(! GetProfileArrayFloat(section, "h0", pParam->h[0], 3, iniFile)){
        pParam->h[0][0]=1;pParam->h[0][1]=0;pParam->h[0][2]=0;
    }
    if(! GetProfileArrayFloat(section, "h1", pParam->h[1], 3, iniFile)){
        pParam->h[1][0]=0;pParam->h[1][1]=1;pParam->h[1][2]=0;
    }
    if(! GetProfileArrayFloat(section, "h2", pParam->h[2], 3, iniFile)){
        pParam->h[2][0]=0;pParam->h[2][1]=0;pParam->h[2][2]=1;
    }

    if(! GetProfileArrayInt(section, "fp_index", pParam->fp_index, 4, iniFile)){
        fprintf(stderr, "Failed to read [%s fp_index !\n", section);
    }
    return true;
}
bool    LoadAreaSettings(AreaSettings* pParam, int nArea, void* iniFile, bool bDeployment)
{

    char section[32];
    sprintf(section, "area_%d", nArea);
    if(!GetProfileRectFloat(section, "range", &pParam->range, iniFile)){
        fprintf(stderr, "[%s] range= not found!\n", section);
    }
    pParam->nFpCounts = GetProfileInt(section, "fp_counts", 4, iniFile);
    if (pParam->nFpCounts > FP_COUNTS){
        fprintf(stderr, "[%s] fp_counts error!\n", section);
        return false;
    }

    char key[32];
    for (int i=0; i< pParam->nFpCounts; i++) {
        sprintf(key, "fpt_%d", i);
        if(!GetProfilePointFloat(section, key, &pParam->fpt[i], iniFile)){
            fprintf(stderr, "%s value not found!\n", key);
        }
        /*  fps for temp use only*/

        sprintf(key, "fpf_%d", i);
        if(!GetProfilePointFloat(section, key, &pParam->fpf[i], iniFile)){
            fprintf(stderr, "[%s] %s value not found!\n", section, key);
        }
    }
    if (bDeployment) {
        for (int i=0; i< pParam->nFpCounts; i++) {
            sprintf(key, "fps_%d", i);
            if(!GetProfilePointFloat(section, key, &pParam->fps[i], iniFile)){
                fprintf(stderr, "[%s] %s value not found!\n", section, key);
            }
        }
    }

    LoadFecParam(&pParam->fec, nArea, iniFile);

    pParam->nFpAreaCounts = GetProfileInt(section, "fp_regions", 4, iniFile);
    if (pParam->nFpAreaCounts > MAX_FP_AREA){
        fprintf(stderr, "[%s] fp_regions error!\n", section);
        return false;
    }
    for (int i=0; i< pParam->nFpAreaCounts; i++) {
        sprintf(key, "region_%d", i);
        if(!GetProfileRectFloat(section, key, &pParam->region[i], iniFile)){

        }
        sprintf(key, "region_type_%d", i);
        pParam->regionType[i] = GetProfileInt(section, key, 0, iniFile);

    }
    if(bDeployment){
        for (int i=0; i< pParam->nFpAreaCounts; i++) {
            LoadHomoParam(&pParam->homo[i], nArea, i, iniFile);
        }
    }
    pParam->state = DATA_STATE_LOADED;
    return true;
}
bool    SaveFecParam(FecParam* pFecParam, int nArea, void* iniFile)
{
    char section[32];
    sprintf(section,"fecparam_%d", nArea);

    if (!WriteProfilePointFloat(section, "ptCenter", &pFecParam->ptCenter, iniFile))
        return false;
    if(!WriteProfileFloat(section, "fov", pFecParam->fov, iniFile))
        return false;
    if(!WriteProfileFloat(section, "k1", pFecParam->k1, iniFile))
        return false;
    if(!WriteProfileFloat(section, "k2", pFecParam->k2, iniFile))
        return false;
    if(!WriteProfileFloat(section, "a", pFecParam->a, iniFile))
        return false;
    if(!WriteProfileFloat(section, "b", pFecParam->b, iniFile))
        return false;
    if(!WriteProfileFloat(section, "c", pFecParam->c, iniFile))
        return false;
    if(!WriteProfileFloat(section, "pitch", pFecParam->pitch, iniFile))
        return false;
    if(!WriteProfileFloat(section, "yaw", pFecParam->yaw, iniFile))
        return false;
    if(!WriteProfileFloat(section, "roll", pFecParam->roll, iniFile))
        return false;
    return true;
}
bool    SaveHomoParam(HomoParam* pParam, int nArea, int nFp, void* iniFile)
{
    char section[32];

    sprintf(section,"homoparam_%d_%d", nArea, nFp);
    if(! WriteProfileArrayFloat(section, "h0", pParam->h[0], 3, iniFile))
        return false;
    if(! WriteProfileArrayFloat(section, "h1", pParam->h[1], 3, iniFile))
        return false;
    if(! WriteProfileArrayFloat(section, "h2", pParam->h[2], 3, iniFile))
        return false;

    if(! WriteProfileArrayInt(section, "fp_index", pParam->fp_index, 4, iniFile))
        return false;

    return true;
}
bool    SaveAreaSettings(AreaSettings* pParam, int nArea, void* iniFile)
{
    char section[32];
    sprintf(section, "area_%d", nArea);
    if(!WriteProfileRectFloat(section, "range", &pParam->range, iniFile))
        return false;
    if(!WriteProfileInt(section, "fp_counts", pParam->nFpCounts, iniFile))
        return false;
    char key[32];
    for (int i=0; i< pParam->nFpCounts; i++) {
        sprintf(key, "fpt_%d", i);
        if(!WriteProfilePointFloat(section, key, &pParam->fpt[i], iniFile))
            return false;
    }
    for (int i=0; i< pParam->nFpCounts; i++) {

        sprintf(key, "fps_%d", i);
        if(!WriteProfilePointFloat(section, key, &pParam->fps[i], iniFile))
            return false;
    }
    for (int i=0; i< pParam->nFpCounts; i++) {
        sprintf(key, "fpf_%d", i);
        if(!WriteProfilePointFloat(section, key, &pParam->fpf[i], iniFile))
            return false;
    }

    if(!SaveFecParam(&pParam->fec, nArea, iniFile))
        return false;
    if(!WriteProfileInt(section, "fp_regions", pParam->nFpAreaCounts, iniFile))
        return false;
    for (int i=0; i< pParam->nFpAreaCounts; i++) {
        sprintf(key, "region_%d", i);
        if(!WriteProfileRectFloat(section, key, &pParam->region[i], iniFile)){
        }
        sprintf(key, "region_type_%d", i);
        WriteProfileInt(section, key, pParam->regionType[i], iniFile);

        SaveHomoParam(&pParam->homo[i], nArea, i, iniFile);
    }
    return true;
}
nfImage* TexProcess::getSourceImage()
{
     if(!mpSourceImageName)
         return NULL;

     char value[32];
     const char* p1 = strchr(mpSourceImageName, '_');
     const char* p2 = strchr(mpSourceImageName, '.');
     int length = p2-p1-1;
     if (p1 == NULL || p2 == NULL || length <3 || length>= (int) sizeof(value)-1){
         LOGE("Wrong file format");
         return NULL;
     }

     memcpy(value, p1+1, length);
     value[length] = 0;
     p1 = strchr(value, 'x');
     if (!p1) {
         LOGE("File name must be in the form of abc_widthxheight.yuv!");
         return NULL;
     }
     *(char*) p1 = 0;

     int width = atoi(value);
     int height = atoi(p1+1);
     FILE* pFile = fopen(mpSourceImageName, "r");
     if (!pFile) {
         LOGE("Failed to open image %s!", mpSourceImageName);
         return NULL;
     }
     nfImage* pYuv = nfImage::create(width, height, 2);
     if (! pYuv){
         LOGE("YUYV: Out of memory!\n");
         fclose(pFile);
         return NULL;
     }
     //read file to pYuv
     if( fread(pYuv->buffer, 1, width*height*2, pFile)<= 0) {
         LOGE("Failed to load image file %s!\n", mpSourceImageName);
         nfImage::destroy(&pYuv);
         fclose(pFile);
         return NULL;
     }
     fclose(pFile);

     nfImage* pRgb = nfImage::create(width, height, 4);
     if (! pRgb){
         LOGE("Out of memory!\n");
         nfImage::destroy(&pYuv);
         return NULL;
     }

     nfYuyvToRgb32(pYuv, pRgb->buffer, true, false);
     nfImage::destroy(&pYuv);
    return pRgb;
}

/* used for calibration */
bool TexProcess::loadIniFile(const char* filename)
{
    int i ;
    bool bOK;
    char sourceImg[256];

    if(mpSourceImageName){
        free (mpSourceImageName);
        mpSourceImageName = NULL;
    }
    void* handle  = openIniFile(filename, true);
    if(!handle)
        return false;

    if (GetProfileString("system", "image_file", sourceImg,
                     sizeof(sourceImg), "", handle)) {
        mpSourceImageName = strdup(sourceImg);
    }
    memset(mAreaSettings, 0, sizeof(mAreaSettings));
    for (i=0; i< MAX_CAMERAS; i++) {
        bOK = LoadAreaSettings(&mAreaSettings[i], i, handle, false);
        if (bOK != true)
            break;
    }
    closeIniFile(handle);
    return bOK;
}
bool TexProcess::loadIniFile2(const char* filename)
{
    int i ;
    bool bOK;

    if(mpSourceImageName){
        free (mpSourceImageName);
        mpSourceImageName = NULL;
    }
    void* handle  = openIniFile(filename, true);
    if(!handle)
        return false;

    memset(mAreaSettings, 0, sizeof(mAreaSettings));
    for (i=0; i< MAX_CAMERAS; i++) {
        bOK = LoadAreaSettings(&mAreaSettings[i], i, handle, true);
        if (bOK != true)
            break;
    }
    closeIniFile(handle);
    return bOK;
}
bool TexProcess::saveIniFile(const char* filename)
{
    int i ;
    bool bOK;
    void* handle  = openIniFile(filename, false);
    if(!handle)
        return false;

    if(mpSourceImageName) {
        WriteProfileString("system", "image_file", mpSourceImageName, handle);
    }
    for (i=0; i< MAX_CAMERAS; i++) {
        bOK = SaveAreaSettings(&mAreaSettings[i], i, handle);
        if (bOK != true)
            break;
    }
    closeIniFile(handle);
    return bOK;
}
void TexProcess::normalizeFpf(int nAreaId)
{
    if (mAreaSettings[nAreaId].fpf[3].x > 2 || mAreaSettings[nAreaId].fpf[3].y > 2)
    { //normalize to [0,1]
        for (int i=0; i<mAreaSettings[nAreaId].nFpCounts; i++){
            mAreaSettings[nAreaId].fpf[i].x /= (float)(IMAGE_WIDTH/2);
            mAreaSettings[nAreaId].fpf[i].y /= (float)(IMAGE_HEIGHT/2);
        }
    }
    mAreaSettings[nAreaId].state = DATA_STATE_FPF;
}
void TexProcess::calculateFps(int nAreaId)
{
    for (int i=0; i<mAreaSettings[nAreaId].nFpCounts; i++){
        nfInvFec(mAreaSettings[nAreaId].fpf[i].x, mAreaSettings[nAreaId].fpf[i].y,
                 mAreaSettings[nAreaId].fps[i].x, mAreaSettings[nAreaId].fps[i].y,
                 &mAreaSettings[nAreaId].fec);
    }
    mAreaSettings[nAreaId].state = DATA_STATE_FPS;
}
void TexProcess::calculateHomo(int nAreaId)
{
    if(mAreaSettings[nAreaId].nFpAreaCounts == 3)
        nfCalculateHomoMatrix3(mAreaSettings[nAreaId].fps, mAreaSettings[nAreaId].fpt, mAreaSettings[nAreaId].homo);
    else if(mAreaSettings[nAreaId].nFpAreaCounts == 14)
        nfCalculateHomoMatrix14(mAreaSettings[nAreaId].fps, mAreaSettings[nAreaId].fpt, mAreaSettings[nAreaId].homo);
    else if(mAreaSettings[nAreaId].nFpAreaCounts == 10)
        nfCalculateHomoMatrix10(mAreaSettings[nAreaId].fps, mAreaSettings[nAreaId].fpt, mAreaSettings[nAreaId].homo);

    else if(mAreaSettings[nAreaId].nFpAreaCounts == 20)
        nfCalculateHomoMatrix20(mAreaSettings[nAreaId].fps, mAreaSettings[nAreaId].fpt, mAreaSettings[nAreaId].homo);
    else if(mAreaSettings[nAreaId].nFpAreaCounts == 16)
        nfCalculateHomoMatrix16(mAreaSettings[nAreaId].fps, mAreaSettings[nAreaId].fpt, mAreaSettings[nAreaId].homo);
    else if (mAreaSettings[nAreaId].nFpAreaCounts == 12)
        nfCalculateHomoMatrix12(mAreaSettings[nAreaId].fps, mAreaSettings[nAreaId].fpt, mAreaSettings[nAreaId].homo);
    else
        nfCalculateHomoMatrix4(mAreaSettings[nAreaId].fps, mAreaSettings[nAreaId].fpt, mAreaSettings[nAreaId].homo);

    mAreaSettings[nAreaId].state = DATA_STATE_HOMO;

}
TexProcess::TexProcess():mpSourceImageName(NULL)
{
    for (int m=0; m< MAX_CAMERAS; m++) {
        for(int k=0; k<MAX_FP_AREA; k++)
            m_RegionMap[m][k] = 1;
    }

/*
    m_RegionMap[0][6] = 1;
    m_RegionMap[0][11] = 1;
    m_RegionMap[2][6] = 1;
    m_RegionMap[2][11] = 1;
    m_RegionMap[1][0] = 0;
    m_RegionMap[3][0] = 0;
    m_RegionMap[1][7] = 0;
    m_RegionMap[3][7] = 0;
*/
}
/////////////
/// \brief TexProcess::update call update if AreaSettings has changed
/// \param xIntv horizontal intervals numbers
/// \param yIntv vertical interval numbers
/// \return false if any error
///
bool TexProcess::update()
{
    /*
    for (int m=0; m< MAX_CAMERAS; m++) {
        if (mAreaSettings[m].nFpAreaCounts == 16){
            nfCalculateHomoMatrix16(mAreaSettings[m].fps, mAreaSettings[m].fpt, mAreaSettings[m].homo);
        }
        else if(mAreaSettings[m].nFpAreaCounts == 12){
            nfCalculateHomoMatrix12(mAreaSettings[m].fps, mAreaSettings[m].fpt, mAreaSettings[m].homo);
        }else {
            nfCalculateHomoMatrix4(mAreaSettings[m].fps, mAreaSettings[m].fpt, mAreaSettings[m].homo);
        }

    }
     */

    return true;
}

TexProcess::~TexProcess()
{
    if (mpSourceImageName)
        free(mpSourceImageName);
}
///////
/// \brief Texture position in 3D world coordinates, the x-z plan
#define TX_SCALEUP    20
#define TX_CENTER      10
#define TZ_SCALEUP    20
#define TZ_CENTER      10

#define X_INTV  16
#define Y_INTV  16
/*<! returns numbers of vertex created in this region*/
int TexProcess::initVertices(vector<nfFloat3D> & vert, nfRectF region)
{
    int i,j;
    float s,t;
    nfFloat3D v;
    int c = 0;
    for (i=0; i<= Y_INTV; i++) {
        t = (region.b - region.t)*(float)i/(float)Y_INTV + region.t;
        for (j=0; j<=X_INTV; j++) {
            s = (region.r - region.l)*(float)j/(float)X_INTV + region.l;
            v.y = 0;
            v.z = TZ_CENTER-t*TZ_SCALEUP;
#ifdef HORZ_MIRROR
            v.x = s*TX_SCALEUP-TX_CENTER;
#else
            v.x = (1-s)*TX_SCALEUP-TX_CENTER;
#endif
            vert.push_back(v);
            c++;
        }
    }
    return c;
}
/*
 * type 1    *----*
 *            *   *
 *              * *
 *                *
 */
int TexProcess::initVertices_type1(vector<nfFloat3D> & vert, nfRectF region)
{
    int i,j;
    float s,t;
    nfFloat3D v;
    int c = 0;
    float dx = (region.r - region.l)/(float)X_INTV;
    float dy = (region.b - region.t)/(float)Y_INTV;
    for (i=0; i<= Y_INTV; i++) {
        t = dy*(float)i + region.t;
        for (j=i; j<=X_INTV; j++) {
            s = dx *(float)j + region.l;
            v.y = 0;
            v.z = TZ_CENTER-t*TZ_SCALEUP;
#ifdef HORZ_MIRROR
            v.x = s*TX_SCALEUP-TX_CENTER;
#else
            v.x = (1-s)*TX_SCALEUP-TX_CENTER;
#endif
            vert.push_back(v);
            c++;
        }
    }
    return c;
}
/*
 * type 2      *
 *           * *
 *          *  *
 *         *---*
 */
int TexProcess::initVertices_type2(vector<nfFloat3D> & vert, nfRectF region)
{
    int i,j;
    float s,t;
    nfFloat3D v;
    int c = 0;
    for (i=0; i<= Y_INTV; i++) {
        t = (region.b - region.t)*(float)i/(float)Y_INTV + region.t;
        for (j=X_INTV-i; j<=X_INTV; j++) {
            s = (region.r - region.l)*(float)j/(float)X_INTV + region.l;
            v.y = 0;
            v.z = TZ_CENTER-t*TZ_SCALEUP;
#ifdef HORZ_MIRROR
            v.x = s*TX_SCALEUP-TX_CENTER;
#else
            v.x = (1-s)*TX_SCALEUP-TX_CENTER;
#endif
            vert.push_back(v);
            c++;
        }
    }
    return c;
}
/*
 * type 3  *
 *         * *
 *         *  *
 *         *---*
 */
int TexProcess::initVertices_type3(vector<nfFloat3D> & vert, nfRectF region)
{
    int i,j;
    float s,t;
    nfFloat3D v;
    int c = 0;
    for (i=0; i<= Y_INTV; i++) {
        t = (region.b - region.t)*(float)i/(float)Y_INTV + region.t;
        for (j=0; j<=i; j++) {
            s = (region.r - region.l)*(float)j/(float)X_INTV + region.l;
            v.y = 0;
            v.z = TZ_CENTER-t*TZ_SCALEUP;
#ifdef HORZ_MIRROR
            v.x = s*TX_SCALEUP-TX_CENTER;
#else
            v.x = (1-s)*TX_SCALEUP-TX_CENTER;
#endif
            vert.push_back(v);
            c++;
        }
    }
    return c;
}
/*
 * type 4  *---*
 *         *  *
 *         * *
 *         *
 */
int TexProcess::initVertices_type4(vector<nfFloat3D> & vert, nfRectF region)
{
    int i,j;
    float s,t;
    nfFloat3D v;
    int c = 0;
    for (i=0; i<= Y_INTV; i++) {
        t = (region.b - region.t)*(float)i/(float)Y_INTV + region.t;
        for (j=0; j<=X_INTV-i; j++) {
            s = (region.r - region.l)*(float)j/(float)X_INTV + region.l;
            v.y = 0;
            v.z = TZ_CENTER-t*TZ_SCALEUP;
#ifdef HORZ_MIRROR
            v.x = s*TX_SCALEUP-TX_CENTER;
#else
            v.x = (1-s)*TX_SCALEUP-TX_CENTER;
#endif
            vert.push_back(v);
            c++;
        }
    }
    return c;
}

int TexProcess::reloadIndices(vector<unsigned short>& indices)
{
    int m,k;
    indices.clear();
//simulate indices changed
/*
    for (m=0; m< MAX_CAMERAS; m++) {
        for(k=0; k<MAX_FP_AREA; k++) {
            if(k==0)
                m_RegionMap[m][k] = 1-g_show;
            else if(k==MAX_FP_AREA-1)
                m_RegionMap[m][k] = g_show;
        }
    }
    g_show = 1-g_show;
*/
//end of simulation
    int c = 0;
    for(m=0; m< MAX_CAMERAS; m++){
        for (k=0; k<MAX_FP_AREA; k++){
            switch(mAreaSettings[m].regionType[k]) {
                case 1:
                    updateIndices_type1(indices, m, c);
                    break;
                case 2:
                    updateIndices_type2(indices, m, c);
                    break;
                case 3:
                    updateIndices_type3(indices, m, c);
                    break;

                case 4:
                    updateIndices_type4(indices, m, c);
                    break;
                default:
                    updateIndices(indices, m, c);
                    break;

            }
            c += mAreaSettings[m].nVertCounts[k];
        }
    }
    return 0;
}
/*
 * type 0    (k)----(k+1)--------------(k+X)
 *           *     *   *     *   *
 *           *    *    *    *    *
 *           *   *     *   *     *
 *           (k+X+1)--(k+X+2)----*-----(k+2X+1)
 */
void TexProcess::updateIndices(vector<unsigned short>& indices, int nCam, int nRegion)
{
    int i, j,k;
    int startIndex = nRegion;//(nCam*MAX_FP_AREA + nRegion)*(Y_INTV+1)*(X_INTV+1);
    for (i=0; i< Y_INTV; i++) {
        k = startIndex + i*(X_INTV+1);
        for(j=0; j<X_INTV; j++) {
#ifdef HORZ_MIRROR
            indices.push_back(k);
            indices.push_back(k+1);
            indices.push_back(k+X_INTV+1);
            indices.push_back(k+X_INTV+1);
            indices.push_back(k+1);
            indices.push_back(k+X_INTV+2);
#else
            indices.push_back(k);
            indices.push_back(k+X_INTV+1);
            indices.push_back(k+1);
            indices.push_back(k+1);
            indices.push_back(k+X_INTV+1);
            indices.push_back(k+X_INTV+2);
#endif
            k++;
        }
    }

}
/*
 * type 1   layer i      (k0)-----(k0+1)   k0 = (2W+3-i)*i/2 = i*W+ (3i-i*i)/2
 *                          *     *        counts = W+1-i
 *                           *    *
 *                             *  *
 *          layer i+1           (k1)       k1= (2W+2-i)*(i+1)/2 = (i+1)*w+(i-i*i)/2 + 1
 */
void TexProcess::updateIndices_type1(vector<unsigned short>& indices, int nCam, int nRegion)
{
    int i, j,k0,k1;
    int startIndex = nRegion; //(nCam*MAX_FP_AREA + nRegion)*(Y_INTV+1)*(X_INTV+1);
    for (i=0; i< Y_INTV; i++) {
        k0 = startIndex + i*X_INTV+ (3*i-i*i)/2;
        k1 = k0 + X_INTV+1-i;
#ifdef HORZ_MIRROR //CW
        indices.push_back(k0);
        indices.push_back(k1+1);
        indices.push_back(k1);
#else //CCW
        indices.push_back(k0);
        indices.push_back(k1);
        indices.push_back(k0+1);
#endif
        k0++;
        for(j=1; j<X_INTV-i; j++) {
#ifdef HORZ_MIRROR //CW
            indices.push_back(k0);
            indices.push_back(k0+1);
            indices.push_back(k1);
            indices.push_back(k1);
            indices.push_back(k0+1);
            indices.push_back(k1+1);
#else //CCW
            indices.push_back(k0);
            indices.push_back(k1);
            indices.push_back(k0+1);
            indices.push_back(k0+1);
            indices.push_back(k1);
            indices.push_back(k1+1);
#endif
            k0++;
            k1++;
        }
    }
}
/*
 * type 2   layer i            (k)   k0 = (1+i)*i/2
 *                           *   *
 *                          *    *
 *                         *     *   k1= (i+2)*(i+1)/2
 *          layer i+1  (k+X+1)--(k+X+2)----*-----(k+2X+1)
 */
void TexProcess::updateIndices_type2(vector<unsigned short>& indices, int nCam, int nRegion)
{
    int i, j,k0,k1;
    int startIndex = nRegion;//(nCam*MAX_FP_AREA + nRegion)*(Y_INTV+1)*(X_INTV+1);
    for (i=0; i< Y_INTV; i++) {
        k0 = startIndex + (i+1)*i/2;
        k1 = k0 + (i+1);
#ifdef HORZ_MIRROR //CW
        indices.push_back(k0);
        indices.push_back(k1+1);
        indices.push_back(k1);
#else //CCW
        indices.push_back(k0);
        indices.push_back(k1);
        indices.push_back(k1+1);
#endif
        k1++;
        for(j=0; j<i; j++) {
#ifdef HORZ_MIRROR //CW
            indices.push_back(k0);
            indices.push_back(k0+1);
            indices.push_back(k1);
            indices.push_back(k1);
            indices.push_back(k0+1);
            indices.push_back(k1+1);
#else //CCW
            indices.push_back(k0);
            indices.push_back(k1);
            indices.push_back(k0+1);
            indices.push_back(k0+1);
            indices.push_back(k1);
            indices.push_back(k1+1);
#endif
            k0++;
            k1++;
        }
    }
}
/*
 * type 3   layer i       (k)        k0 = (1+i)*i/2
 *                         *   *
 *                         *    *
 *                         *     *   k1= k0 + (i+1)
 *          layer i+1  (k+X+1)--(k+X+2)----*-----(k+2X+1)
 */
void TexProcess::updateIndices_type3(vector<unsigned short>& indices, int nCam, int nRegion)
{
    int i, j,k0,k1;
    int startIndex = nRegion;//(nCam*MAX_FP_AREA + nRegion)*(Y_INTV+1)*(X_INTV+1);
    for (i=0; i< Y_INTV; i++) {
        k0 = startIndex + (i+1)*i/2;
        k1 = k0 + (i+1);
#ifdef HORZ_MIRROR //CW
        indices.push_back(k0);
        indices.push_back(k1+1);
        indices.push_back(k1);
#else //CCW
        indices.push_back(k0);
        indices.push_back(k1);
        indices.push_back(k1+1);
#endif
        k1++;
        for(j=0; j<i; j++) {
#ifdef HORZ_MIRROR //CW
            indices.push_back(k0);
            indices.push_back(k0+1);
            indices.push_back(k1);
            indices.push_back(k1);
            indices.push_back(k0+1);
            indices.push_back(k1+1);
#else //CCW
            indices.push_back(k0);
            indices.push_back(k1);
            indices.push_back(k0+1);
            indices.push_back(k0+1);
            indices.push_back(k1);
            indices.push_back(k1+1);
#endif
            k0++;
            k1++;
        }
    }
}

/*
 * type 4   layer i      (k0)-----(k0+1)   k0 = (2W+3-i)*i/2 = i*W+ (3i-i*i)/2
 *                         *     *        counts = W+1-i
 *                         *    *
 *                         *  *
 *          layer i+1     (k1)       k1= (2W+2-i)*(i+1)/2 = (i+1)*w+(i-i*i)/2 + 1
 */
void TexProcess::updateIndices_type4(vector<unsigned short>& indices, int nCam, int nRegion)
{
    int i, j,k0,k1;
    int startIndex = nRegion; //(nCam*MAX_FP_AREA + nRegion)*(Y_INTV+1)*(X_INTV+1);
    for (i=0; i< Y_INTV; i++) {
        k0 = startIndex + i*X_INTV+ (3*i-i*i)/2;
        k1 = k0 + X_INTV+1-i;
#ifdef HORZ_MIRROR //CW
        indices.push_back(k0);
        indices.push_back(k1+1);
        indices.push_back(k1);
#else //CCW
        indices.push_back(k0);
        indices.push_back(k1);
        indices.push_back(k0+1);
#endif
        k0++;
        for(j=1; j<X_INTV-i; j++) {
#ifdef HORZ_MIRROR //CW
            indices.push_back(k0);
            indices.push_back(k0+1);
            indices.push_back(k1);
            indices.push_back(k1);
            indices.push_back(k0+1);
            indices.push_back(k1+1);
#else //CCW
            indices.push_back(k0);
            indices.push_back(k1);
            indices.push_back(k0+1);
            indices.push_back(k0+1);
            indices.push_back(k1);
            indices.push_back(k1+1);
#endif
            k0++;
            k1++;
        }
    }
}
int TexProcess::createVertices(vector<nfFloat3D> & vert, vector<unsigned short>& indices)
{
    int m,k;
    vert.clear();
    indices.clear();
    int c = 0;//start vertex index of the region
    for(m=0; m< MAX_CAMERAS; m++){
        for (k=0; k<MAX_FP_AREA; k++){
            /* remember vertix counts in eanch region */
            switch(mAreaSettings[m].regionType[k]) {
                case 1:
                    mAreaSettings[m].nVertCounts[k] = initVertices_type1(vert, mAreaSettings[m].region[k]);
                    updateIndices_type1(indices, m, c);
                    break;
                case 2:
                    mAreaSettings[m].nVertCounts[k] = initVertices_type2(vert, mAreaSettings[m].region[k]);
                    updateIndices_type2(indices, m, c);
                    break;
                case 3:
                    mAreaSettings[m].nVertCounts[k] = initVertices_type3(vert, mAreaSettings[m].region[k]);
                    updateIndices_type3(indices, m, c);
                    break;
                case 4:
                    mAreaSettings[m].nVertCounts[k] = initVertices_type4(vert, mAreaSettings[m].region[k]);
                    updateIndices_type4(indices, m, c);
                    break;
                case 0:
                default:
                    mAreaSettings[m].nVertCounts[k] = initVertices(vert, mAreaSettings[m].region[k]);
                    updateIndices(indices, m, c);
                    break;

            }
            c += mAreaSettings[m].nVertCounts[k];//next start index
        }
    }
    return 0;
}
int TexProcess::updateUv(vector<nfFloat3D> vert, vector <nfFloat2D> &uv)
{
    int m,k,i;
    uv.clear();
    nfFloat2D value;
    int c = 0;
    for(m=0; m< MAX_CAMERAS; m++) {
        for (k = 0; k < MAX_FP_AREA; k++) {
            for (i=0; i< mAreaSettings[m].nVertCounts[k]; i++) {
                float s, t, u, v, x,y;
                s = 1- (vert[c].x + TX_CENTER)/TX_SCALEUP;
                t = (TZ_CENTER - vert[c].z)/TZ_SCALEUP;

                if (nfDoHomoTransform(s, t, u, v, mAreaSettings[m].homo[k].h)) {
                    nfDoFec(u, v, x, y, &mAreaSettings[m].fec);
                    value.x = (x*0.5f+ s_offsetCam[m].x);
                    value.y = y*0.5f+ s_offsetCam[m].y;
                }else {
                    value.x=value.y=0.5;
                }
                c++;
                uv.push_back(value);
            }
        }
    }

    return 0;
}

int TexProcess::updateUvNoFisheye(vector <nfFloat2D> &uv)
{
    int m,k,i,j;
    uv.clear();
    nfFloat2D value;
    float rota[4] = {1,0,-1,0};
    float rotb[4] = {0,1,0,-1};
    float rotc[4] = {0,1,0,-1};
    float rotd[4] = {-1,0,1,0};
    float tx[4] = {0, 0, 1, 1};
    float ty[4] = {1, 0, 0, 1};

    for(m=0; m< MAX_CAMERAS; m++){
        nfRectF area = mAreaSettings[m].range;

        for (k=0; k<MAX_FP_AREA; k++){
            ////---- update UV in the region  gAreaSettings[m].region[k]);
            float s,t, u,v,x,y;
            nfRectF region = mAreaSettings[m].region[k];
            for (i=0; i<= Y_INTV; i++) {
                t = (region.b - region.t)*(float)i/(float)Y_INTV + region.t;
                for (j=0; j<=X_INTV; j++) {
                    s = (region.r - region.l)*(float)j/(float)X_INTV+region.l;

                    u = (s - area.l )/ (area.r - area.l);
                    v = (t- area.t)/(area.b - area.t);
                    x = rota[m]*u + rotb[m]*v + tx[m];
                    y = rotc[m]*u + rotd[m]*v + ty[m];
                    value.x = (x*0.5f+ s_offsetCam[m].x);
                    value.y = y*0.4f*0.5f+ s_offsetCam[m].y+0.3f; //take 0.2 to 0.6 of height in camera


                    //
                    uv.push_back(value);
                }
            }
            ////---- end of one region
        }
    }

    return 0;
}


