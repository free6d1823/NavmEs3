#include "ImgProcess.h"
#include "Mat.h"
#include <math.h>
#include <android/log.h>

#define  LOG_TAG    "Floor"
#include "../common.h"

#define IMAGE_PATH  ":/camera1800x1440.yuv"
#define IMAGE_WIDTH 1800
#define IMAGE_HEIGHT    1440
#define IMAGE_AREA_WIDTH    900
#define IMAGE_AREA_HEIGHT   720



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
    for (int i=0; i<pYuv->height; i++)
    {
        for (int j=0; j<pYuv->width; j+=2)
        {
            y1 = pY1[2*j];
            u = pU[2*j];
            v = pV[2*j];
            pLine1[j*4] = YUV2B(y1, u, v);//b
            pLine1[j*4+1] = YUV2G(y1, u, v);//g
            pLine1[j*4+2] = YUV2R(y1, u, v);//r
            pLine1[j*4+3] = 0xff;

            y1 = pY1[2*j+2];
            pLine1[j*4+4] = YUV2B(y1, u, v);//b
            pLine1[j*4+5] = YUV2G(y1, u, v);//g
            pLine1[j*4+6] = YUV2R(y1, u, v);//r
            pLine1[j*4+7] = 0xff;
        }
        pY1 += nStride;
        pV += nStride;
        pU += nStride;
        pLine1 += nBps;

    }
}
/****************************************************************/
int nfImage::seq=0;
nfImage* nfImage::create(unsigned int w, unsigned int h, unsigned int bpp)
{
    nfImage* img =  new nfImage;
    if (!img)
        return img;
    img->seq = ++seq;
    img->width = w;
    img->stride = w*bpp;
    img->height = h;
    img->bRef = false;
    img->buffer = (unsigned char*)malloc( img->stride*h);
    if (!img->buffer ) {
        delete img;
        return NULL;
    }

    printf ("nfInitImage#%d (%dx%d)\n", img->seq, w, h);
    return img;
}
nfImage* nfImage::ref(unsigned char* data, unsigned int w, unsigned int h, unsigned int bpp)
{
    nfImage* img =  new nfImage;
    if (!img)
        return img;
    img->seq = ++seq;
    img->buffer = data;
    img->width = w;
    img->stride = w*bpp;
    img->height = h;
    img->bRef = true;
    printf ("nfRefImage#%d (%dx%d)\n", img->seq, w, h);
    return img;
}

void nfImage::destroy(nfImage** ppImage)
{
    if(*ppImage) {
printf("nfFreeImage,#%d - %dx%d\n", (*ppImage)->seq, (*ppImage)->width,(*ppImage)->height);
        if(!(*ppImage)->bRef)
            SAFE_FREE((*ppImage)->buffer);
        delete (*ppImage);
        *ppImage = NULL;
    }
}
/****************************************************************/
nfBuffer::nfBuffer(unsigned int elements)
{
}
nfBuffer* nfBuffer::create(unsigned int counts)
{
    return new nfBuffer(counts);
}

void nfBuffer::destroy(nfBuffer** ppBuffer)
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
    LOGI("nfFloatBuffer new %dx%d=%d mpData=%p", mBpu, elements, mLength, mpData);
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
    LOGI("nfUshortBuffer new %dx%d=%d mpData=%p", mBpu, elements, mLength, mpData);
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

#if 0
IMAGE* ImgProcess::loadImage()
{
    QFile fp(IMAGE_PATH);
    if(!fp.open(QIODevice::ReadOnly))
            return NULL;

    IMAGE* pImg = initImage(IMAGE_WIDTH, IMAGE_HEIGHT);
    if (!pImg){
       fp.close();
       return pImg;
    }
    do {
        unsigned char* pSrc = (unsigned char*) malloc(IMAGE_WIDTH*2*IMAGE_HEIGHT);
        if (!pSrc)
            break;
        fp.read((char* )pSrc, IMAGE_WIDTH*2*IMAGE_HEIGHT);
        YuyvToRgb32(pSrc, IMAGE_WIDTH, IMAGE_WIDTH*2, IMAGE_HEIGHT, pImg->buffer, true);
        free(pSrc);
    }while(0);
   fp.close();
   return pImg;
}
IMAGE* ImgProcess::loadImageArea(int idArea, FecParam* pFec)
{
    IMAGE* pSrc = loadImage();
    if (!pSrc)
        return NULL;
    IMAGE* pOut = initImage(IMAGE_AREA_WIDTH, IMAGE_AREA_HEIGHT);
    unsigned char* pIn;
    switch (idArea) {
    case 0:
        pIn = pSrc->buffer;
        break;
    case 1:
        pIn = pSrc->buffer+ IMAGE_AREA_WIDTH*4;
        break;
    case 2:
        pIn = pSrc->buffer + IMAGE_WIDTH*4*IMAGE_AREA_HEIGHT;
        break;
    case 3:
    default:
        pIn = pSrc->buffer + IMAGE_WIDTH*4*IMAGE_AREA_HEIGHT+ IMAGE_AREA_WIDTH*4;
        break;
    }

    ApplyFec(pIn, pOut->width, pSrc->stride, pOut->height,
             pOut->buffer, pOut->stride, pFec);
    ImgProcess::freeImage(pSrc);
    return pOut;
}
void ImgProcess::ApplyFec(unsigned char* pSrc, int width, int inStride,  int height,
                        unsigned char* pTar, int outStride, FecParam* pFec)
{
    double x,y,u,v;
    int nX, nY;
    for (int i=0; i< height; i++) {
        v = (double)i/(double)height;
        for (int j=0; j<width; j++) {
            u = (double)j/(double)width;
            ImgProcess::doFec(u,v,x,y, pFec);

            if ( x>=0 && x<1 && y>=0 && y< 1){
                nX = (int) (x * (double) width+0.5);
                nY = (int) (y * (double) height+0.5);

                pTar[i*outStride + j*4  ] = pSrc[nY*inStride+nX*4  ];//B
                pTar[i*outStride + j*4+1] = pSrc[nY*inStride+nX*4+1];//G
                pTar[i*outStride + j*4+2] = pSrc[nY*inStride+nX*4+2];//R
                pTar[i*outStride + j*4+3] = pSrc[nY*inStride+nX*4+3];//A
            }
            else {
                pTar[i*outStride + j*4  ] = 0;
                pTar[i*outStride + j*4+1] = 0;
                pTar[i*outStride + j*4+2] = 0;
                pTar[i*outStride + j*4+3] = 0xff;
            }
        }
    }
}

void ImgProcess::doFec(double u, double v, double &x, double &y, FecParam* m_pFec)
{
    double  u1, v1;
    double x1,y1;
    //transform uu = M x u
    u1 = u-0.5;
    v1 = v-0.5;
    double fr = 2*tan(m_pFec->fov/2); //focus length of rectified image

    double rp = sqrt(u1*u1+v1*v1); //radius of point (u1,v1) on rectified image
    double rq;	//fisheye

    double rq1;

    if(1) //fisheye
        rq1 = atan(rp*fr)/PI_2;		//0~1
    else //normal lens
        rq1 = rp; //if no fec

    //LDC
    if (rp <= 0) {
        x1 = y1 = 0;
    } else {
        u1  = u1 * rq1/rp;
        v1  = v1 * rq1/rp;

        double rq2 = rq1*rq1;
        rq  = (1+m_pFec->k1* rq2+ m_pFec->k2* rq2*rq2);
        x1 = u1/rq;
        //t
        y1 = v1/rq+m_pFec->c*rq2;
    }
    double x2,y2;
    //pitch
    double phi = atan(y1*2);//assume r=0.5
    double sy = 1+tan(m_pFec->pitch)*tan(phi);
    x2 = x1/sy;
    y2 = (y1/sy)/cos(m_pFec->pitch);
    //yaw
    double theda = atan(x2*2);
    double sx = 1+tan(m_pFec->yaw)*tan(theda);
    x2 = (x2/sx)/cos(m_pFec->yaw);
    y2 = y2/sx;

    //spin - z-axis
    x1 = cos(m_pFec->roll)* x2 - sin(m_pFec->roll)*y2;
    y1 = sin(m_pFec->roll)* x2 + cos(m_pFec->roll)*y2;
    //intrinsic parameters calibration
    //t 	x1 = param.a*x1 + param.c*y1;		//x-scale and de-skewness
    x1 = m_pFec->a*x1;		//x-scale and de-skewness
    y1 = m_pFec->b*y1;								//y- scale
    x = (x1+ m_pFec->ptCenter.x);
    y = (y1+ m_pFec->ptCenter.y);

}

#define IsInRect(x,y, rc) (x>=rc.l && x<rc.r && y>= rc.t && y< rc.b)

void ImgProcess::findHomoMatreix(dbPOINT s[4], dbPOINT t[4], double hcoef[3][3])
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
bool ImgProcess::doHomoTransform(double s, double t, double &u, double &v, double h[3][3])
{
    u = h[0][0] * s +h[0][1]*t + h[0][2];
    v = h[1][0] * s + h[1][1]*t + h[1][2];
    double scale = h[2][0] * s + h[2][1]*t + 1;
    if (scale != 0) {
            u = u/scale;
            v = v/scale;
            if(u>=1 || v>=1 ||u<0 ||v <0) return false;
            return true;
    }
    return false;
}
void ImgProcess::calculateHomoMatrix(dbPOINT* fps, dbPOINT* fpt, HomoParam* homo)
{
    dbPOINT s[MAX_FP_AREA][4] = {{fps[0], fps[1], fps[6], fps[5]},
                     {fps[1], fps[2], fps[7], fps[6]},
                     {fps[2], fps[3], fps[8], fps[7]},
                     {fps[3], fps[4], fps[9], fps[8]}};
    dbPOINT t[MAX_FP_AREA][4] = {{fpt[0], fpt[1], fpt[6], fpt[5]},
                     {fpt[1], fpt[2], fpt[7], fpt[6]},
                     {fpt[2], fpt[3], fpt[8], fpt[7]},
                     {fpt[3], fpt[4], fpt[9], fpt[8]}};

    for(int i=0; i<MAX_FP_AREA; i++) {

        ImgProcess::findHomoMatreix(s[i],t[i], homo[i].h);
    }

}
#endif

/////////////////////////////////////
/// \brief s_offsetCam each camera position in video frame
/// |-------|------|
/// | front | right|
/// |-------|------|
/// | rear  | left |
/// |-------|------|
///

#if 0
QPointF TexProcess::s_offsetCam[MAX_CAMERAS]={QPointF(0,0), QPointF(0.5, 0),
                          QPointF(0, 0.5), QPointF(0.5, 0.5)};

TexProcess::TexProcess()

{
}
/////////////
/// \brief TexProcess::init call init if AreaSettings has changed
/// \param xIntv horizontal intervals numbers
/// \param yIntv vertical interval numbers
/// \return false if any error
///
bool TexProcess::init()
{
    for (int m=0; m< MAX_CAMERAS; m++) {
       LoadAreaSettings(&m_as[m], m);
       ImgProcess::calculateHomoMatrix(m_as[m].fps, m_as[m].fpt, m_as[m].homo);
       for(int k=0; k<MAX_FP_AREA; k++) {
           if(k==0)
               m_RegionMap[m][k] = 0;
           else
               m_RegionMap[m][k] = 1;
       }
    }


    return true;
}

TexProcess::~TexProcess()
{

}
///////
/// \brief Texture position in 3D world coordinates, the x-z plan
#define TX_SCALEUP    20
#define TX_CENTER      10
#define TZ_SCALEUP    20
#define TZ_CENTER      10

#define X_INTV  16
#define Y_INTV  16

void TexProcess::initVertices(vector<QVector3D> & vert, dbRECT region)
{
    int i,j;
    float s,t;
    for (i=0; i<= Y_INTV; i++) {
        t = (region.b - region.t)*(float)i/(float)Y_INTV + region.t;
        for (j=0; j<=X_INTV; j++) {
            s = (region.r - region.l)*(float)j/(float)X_INTV + region.l;
#ifdef HORZ_MIRROR
            vert.push_back(QVector3D(s*TX_SCALEUP-TX_CENTER, 0, TZ_CENTER-t*TZ_SCALEUP));
#else
            vert.push_back(QVector3D((1-s)*TX_SCALEUP-TX_CENTER, 0, TZ_CENTER-t*TZ_SCALEUP));
#endif
        }
    }
}
int g_show = 0;
int TexProcess::reloadIndices(vector<unsigned short>& indices)
{
    int m,k;
    indices.clear();
//simulate indices changed
    for (m=0; m< MAX_CAMERAS; m++) {
        for(k=0; k<MAX_FP_AREA; k++) {
            if(k==0)
                m_RegionMap[m][k] = 1-g_show;
            else if(k==MAX_FP_AREA-1)
                m_RegionMap[m][k] = g_show;


        }
    }
    g_show = 1-g_show;
//end of simulation

    for(m=0; m< MAX_CAMERAS; m++){
        for (k=0; k<MAX_FP_AREA; k++){
            if(m_RegionMap[m][k] != 0)
                updateIndices(indices, m, k);
        }
    }
    return 0;
}

void TexProcess::updateIndices(vector<unsigned short>& indices, int nCam, int nRegion)
{
    int i, j,k;
    int startIndex = (nCam*MAX_FP_AREA + nRegion)*(Y_INTV+1)*(X_INTV+1);
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

int TexProcess::createVertices(vector<QVector3D> & vert, vector<unsigned short>& indices)
{
    int m,k;
    vert.clear();
    indices.clear();
    for(m=0; m< MAX_CAMERAS; m++){
        for (k=0; k<MAX_FP_AREA; k++){
            initVertices(vert, m_as[m].region[k]);
            if(m_RegionMap[m][k] != 0)
                updateIndices(indices, m, k);
        }
    }
    return 0;
}
int TexProcess::updateUv(vector <QVector2D> &uv)
{
    int m,k,i,j;
    uv.clear();
    for(m=0; m< MAX_CAMERAS; m++){
        for (k=0; k<MAX_FP_AREA; k++){
            ////---- update UV in the region  m_as[m].region[k]);
            double s,t,u,v,x,y;
            dbRECT region = m_as[m].region[k];
            for (i=0; i<= Y_INTV; i++) {
                t = (region.b - region.t)*(double)i/(double)Y_INTV + region.t;
                for (j=0; j<=X_INTV; j++) {
                    s = (region.r - region.l)*(double)j/(double)X_INTV + region.l;
                    if (ImgProcess::doHomoTransform(s,t,u,v, m_as[m].homo[k].h)) {
                        ImgProcess::doFec(u,v,x,y, &m_as[m].fec);
                        x = (x*0.5+ s_offsetCam[m].x());
                        y = y*0.5+ s_offsetCam[m].y();

                        //
                    }else{
                        x=y=0.5;
                    }
                    //
                    uv.push_back(QVector2D(x, y));
                }
            }
            ////---- end of one region
        }
    }

    return 0;
}

#endif
