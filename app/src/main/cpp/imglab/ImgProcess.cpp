#include "ImgProcess.h"
#include "Mat.h"
#include <math.h>
#include <android/log.h>

#define  LOG_TAG    "Floor"
#include "../common.h"
#include "../inifile/inifile.h"

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

//    printf ("nfInitImage#%d (%dx%d)\n", img->seq, w, h);
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
//    printf ("nfRefImage#%d (%dx%d)\n", img->seq, w, h);
    return img;
}

void nfImage::destroy(nfImage** ppImage)
{
    if(*ppImage) {
//printf("nfFreeImage,#%d - %dx%d\n", (*ppImage)->seq, (*ppImage)->width,(*ppImage)->height);
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

#define IsInRect(x,y, rc) (x>=rc.l && x<rc.r && y>= rc.t && y< rc.b)



#endif

void nfDoFec(float u, float v, float &x, float &y, FecParam* m_pFec)
{
    float  u1, v1;
    float x1,y1;
    //transform uu = M x u
    u1 = u-0.5;
    v1 = v-0.5;
    float fr = (float) 2*tan(m_pFec->fov/2); //focus length of rectified image

    float rp = (float) sqrt(u1*u1+v1*v1); //radius of point (u1,v1) on rectified image
    float rq;	//fisheye

    float rq1;

    if(1) //fisheye
        rq1 = (float) atan(rp*fr)/M_PI_2;		//0~1
    else //normal lens
        rq1 = rp; //if no fec

    //LDC
    if (rp <= 0) {
        x1 = y1 = 0;
    } else {
        u1  = u1 * rq1/rp;
        v1  = v1 * rq1/rp;

        float rq2 = rq1*rq1;
        rq  = (1+m_pFec->k1* rq2+ m_pFec->k2* rq2*rq2);
        x1 = u1/rq;
        //t
        y1 = v1/rq+m_pFec->c*rq2;
    }
    float x2,y2;
    //pitch
    float phi = atan(y1*2);//assume r=0.5
    float sy = 1+tan(m_pFec->pitch)*tan(phi);
    x2 = x1/sy;
    y2 = (y1/sy)/cos(m_pFec->pitch);
    //yaw
    float theda = atan(x2*2);
    float sx = 1+tan(m_pFec->yaw)*tan(theda);
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

bool nfDoHomoTransform(float s, float t, float &u, float &v, float h[3][3])
{
    u = h[0][0] * s +h[0][1]*t + h[0][2];
    v = h[1][0] * s + h[1][1]*t + h[1][2];
    float scale = h[2][0] * s + h[2][1]*t + 1;
    if (scale != 0) {
        u = u/scale;
        v = v/scale;
        if(u>=1 || v>=1 ||u<0 ||v <0)
            return false;
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

/*<! feature points indexing , each rect is clock-wised numbering
 *   0 -------- 1 -------- 2 -------- 3 ---------4
 *   | region 0 | region 1 | region 2 | region 3 |
 *   5 -------- 6 -------- 7 -------- 8 ---------9 */

void nfCalculateHomoMatrix(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo)
{
    nfFloat2D s[MAX_FP_AREA][4] = {{fps[0], fps[1], fps[6], fps[5]}, /*<! source ( uncorrected) region 0 */
                                 {fps[1], fps[2], fps[7], fps[6]},
                                 {fps[2], fps[3], fps[8], fps[7]},
                                 {fps[3], fps[4], fps[9], fps[8]}};
    nfFloat2D t[MAX_FP_AREA][4] = {{fpt[0], fpt[1], fpt[6], fpt[5]}, /*<! corrected region 0 */
                                 {fpt[1], fpt[2], fpt[7], fpt[6]},
                                 {fpt[2], fpt[3], fpt[8], fpt[7]},
                                 {fpt[3], fpt[4], fpt[9], fpt[8]}};

    for(int i=0; i<MAX_FP_AREA; i++) {
        nfFindHomoMatrix(s[i],t[i], homo[i].h);
    }

}
/////////////////////////////////////
/// \brief s_offsetCam each camera position in video frame
/// |-------|------|
/// | front | right|
/// |-------|------|(1,0.5)
/// | rear  | left |
/// |-------|------|(1,1)
///


nfFloat2D TexProcess::s_offsetCam[MAX_CAMERAS]={ {0,0}, {0.5, 0},
                          {0, 0.5}, {0.5, 0.5}};

AreaSettings TexProcess::gAreaSettings[MAX_CAMERAS];
bool mIsUpdated = false;
extern char gIniFile[256];

bool    LoadFecParam(FecParam* pFecParam, int nArea)
{
    char section[32];
    sprintf(section, "fecparam_%d", nArea);

    if(!GetProfilePointFloat(section, "ptCenter", &pFecParam->ptCenter, gIniFile))
    {
        pFecParam->ptCenter.x = 0.5;
        pFecParam->ptCenter.y = 0.5;
    }
    pFecParam->fov = GetProfileFloat(section, "fov", (170.0* M_PI/180.0), gIniFile);
    pFecParam->k1 = GetProfileFloat(section, "k1", -8.0, gIniFile);
    pFecParam->k2 = GetProfileFloat(section, "k2", 0.0, gIniFile);
    pFecParam->a = GetProfileFloat(section, "a", 1.0, gIniFile);
    pFecParam->b = GetProfileFloat(section, "b", 1.0, gIniFile);
    pFecParam->c = GetProfileFloat(section, "c", 0.0, gIniFile);
    pFecParam->pitch = GetProfileFloat(section, "pitch", 0.0, gIniFile);
    pFecParam->yaw = GetProfileFloat(section, "yaw", 0.0, gIniFile);
    pFecParam->roll = GetProfileFloat(section, "roll", 0.0, gIniFile);

    return true;
}
bool    LoadHomoParam(HomoParam* pParam, int nArea, int nFp)
{
    char section[32];

    sprintf(section,"homoparam_%d_%d", nArea, nFp);
    if(! GetProfileArrayFloat(section, "h0", pParam->h[0], 3, gIniFile)){
        pParam->h[0][0]=1;pParam->h[0][1]=0;pParam->h[0][2]=0;
    }
    if(! GetProfileArrayFloat(section, "h1", pParam->h[1], 3, gIniFile)){
        pParam->h[1][0]=0;pParam->h[1][1]=1;pParam->h[1][2]=0;
    }
    if(! GetProfileArrayFloat(section, "h2", pParam->h[2], 3, gIniFile)){
        pParam->h[2][0]=0;pParam->h[2][1]=0;pParam->h[2][2]=1;
    }

    if(! GetProfileArrayInt(section, "fp_index", pParam->fp_index, 4, gIniFile)){
        fprintf(stderr, "Failed to read [%s fp_index !\n", section);
    }
    return true;
}
bool    LoadAreaSettings(AreaSettings* pParam, int nArea)
{

    char section[32];
    sprintf(section, "area_%d", nArea);
    if(!GetProfileRectFloat(section, "range", &pParam->range, gIniFile)){
        fprintf(stderr, "[%s] range= not found!\n", section);
    }
    char key[32];
    for (int i=0; i< FP_COUNTS; i++) {
        sprintf(key, "fpt_%d", i);
        if(!GetProfilePointFloat(section, key, &pParam->fpt[i], gIniFile)){
            fprintf(stderr, "%s value not found!\n", key);
        }
        sprintf(key, "fps_%d", i);
        if(!GetProfilePointFloat(section, key, &pParam->fps[i], gIniFile)){
            fprintf(stderr, "[%s] %s value not found!\n", section, key);
        }
    }
    LoadFecParam(&pParam->fec, nArea);
    for (int i=0; i< MAX_FP_AREA; i++) {
        sprintf(key, "region_%d", i);
        if(!GetProfileRectFloat(section, key, &pParam->region[i], gIniFile)){

        }

        LoadHomoParam(&pParam->homo[i], nArea, i);
    }
    return true;
}
bool    SaveFecParam(FecParam* pFecParam, int nArea)
{
    char section[32];
    sprintf(section,"fecparam_%d", nArea);

    if (!WriteProfilePointFloat(section, "ptCenter", &pFecParam->ptCenter, gIniFile))
        return false;
    if(!WriteProfileFloat(section, "fov", pFecParam->fov, gIniFile))
        return false;
    if(!WriteProfileFloat(section, "k1", pFecParam->k1, gIniFile))
        return false;
    if(!WriteProfileFloat(section, "k2", pFecParam->k2, gIniFile))
        return false;
    if(!WriteProfileFloat(section, "a", pFecParam->a, gIniFile))
        return false;
    if(!WriteProfileFloat(section, "b", pFecParam->b, gIniFile))
        return false;
    if(!WriteProfileFloat(section, "c", pFecParam->c, gIniFile))
        return false;
    if(!WriteProfileFloat(section, "pitch", pFecParam->pitch, gIniFile))
        return false;
    if(!WriteProfileFloat(section, "yaw", pFecParam->yaw, gIniFile))
        return false;
    if(!WriteProfileFloat(section, "roll", pFecParam->roll, gIniFile))
        return false;
    return true;
}
bool    SaveHomoParam(HomoParam* pParam, int nArea, int nFp)
{
    char section[32];

    sprintf(section,"homoparam_%d_%d", nArea, nFp);
    if(! WriteProfileArrayFloat(section, "h0", pParam->h[0], 3, gIniFile))
        return false;
    if(! WriteProfileArrayFloat(section, "h1", pParam->h[1], 3, gIniFile))
        return false;
    if(! WriteProfileArrayFloat(section, "h2", pParam->h[2], 3, gIniFile))
        return false;

    if(! WriteProfileArrayInt(section, "fp_index", pParam->fp_index, 4, gIniFile))
        return false;

    return true;
}

bool    SaveAreaSettings(AreaSettings* pParam, int nArea)
{
    char section[32];
    sprintf(section, "area_%d", nArea);
    if(!WriteProfileRectFloat(section, "range", &pParam->range, gIniFile))
        return false;
    char key[32];
    for (int i=0; i< FP_COUNTS; i++) {
        sprintf(key, "fpt_%d", i);
        if(!WriteProfilePointFloat(section, key, &pParam->fpt[i], gIniFile))
            return false;
        sprintf(key, "fps_%d", i);
        if(!WriteProfilePointFloat(section, key, &pParam->fps[i], gIniFile))
            return false;
    }

    if(!SaveFecParam(&pParam->fec, nArea))
        return false;

    for (int i=0; i< MAX_FP_AREA; i++) {
        sprintf(key, "region_%d", i);
        if(!WriteProfileRectFloat(section, key, &pParam->region[i], gIniFile)){
        }

        SaveHomoParam(&pParam->homo[i], nArea, i);
    }
    return true;
}
void TexProcess::LoadAllAreaSettings()
{
    if (mIsUpdated)
        return;
    int i ;
    for (i=0; i< MAX_CAMERAS; i++) {
        LoadAreaSettings(&gAreaSettings[i], i);
    }
    mIsUpdated = true;
}
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
    LoadAllAreaSettings();
    for (int m=0; m< MAX_CAMERAS; m++) {
       nfCalculateHomoMatrix(gAreaSettings[m].fps, gAreaSettings[m].fpt, gAreaSettings[m].homo);
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

void TexProcess::initVertices(vector<nfFloat3D> & vert, nfRectF region)
{
    int i,j;
    float s,t;
    nfFloat3D v;
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

int TexProcess::createVertices(vector<nfFloat3D> & vert, vector<unsigned short>& indices)
{
    int m,k;
    vert.clear();
    indices.clear();
    for(m=0; m< MAX_CAMERAS; m++){
        for (k=0; k<MAX_FP_AREA; k++){
            initVertices(vert, gAreaSettings[m].region[k]);
            if(m_RegionMap[m][k] != 0)
                updateIndices(indices, m, k);
        }
    }
    return 0;
}
int TexProcess::updateUv(vector <nfFloat2D> &uv)
{
    int m,k,i,j;
    uv.clear();
    nfFloat2D value;
    for(m=0; m< MAX_CAMERAS; m++){
        for (k=0; k<MAX_FP_AREA; k++){
            ////---- update UV in the region  gAreaSettings[m].region[k]);
            float s,t,u,v,x,y;
            nfRectF region = gAreaSettings[m].region[k];
            for (i=0; i<= Y_INTV; i++) {
                t = (region.b - region.t)*(double)i/(double)Y_INTV + region.t;
                for (j=0; j<=X_INTV; j++) {
                    s = (region.r - region.l)*(double)j/(double)X_INTV + region.l;
                    if (nfDoHomoTransform(s,t,u,v, gAreaSettings[m].homo[k].h)) {
                        nfDoFec(u,v,x,y, &gAreaSettings[m].fec);
                        value.x = (x*0.5f+ s_offsetCam[m].x);
                        value.y = y*0.5f+ s_offsetCam[m].y;

                        //
                    }else{
                        value.x=value.y=0.5;
                    }
                    //
                    uv.push_back(value);
                }
            }
            ////---- end of one region
        }
    }

    return 0;
}


