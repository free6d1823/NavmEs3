#include <stdlib.h>
#include <memory.h>
#define LOG_TAG "CAMERA_SOURCE"
#include "common.h"

#include "CameraManager.h"
#include "./imglab/ImgProcess.h"
#include "CameraSource.h"
#include <linux/videodev2.h>



#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)


// YUV -> RGB
#define C(Y) ( (Y) - 16  )
#define D(U) ( (U) - 128 )
#define E(V) ( (V) - 128 )

#define YUV2R(Y, U, V) CLIP(( 298 * C(Y)              + 409 * E(V) + 128) >> 8)
#define YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
#define YUV2B(Y, U, V) CLIP(( 298 * C(Y) + 516 * D(U)              + 128) >> 8)


void YuyvToRgb32(unsigned char* pYuv, int width, int stride, int height, unsigned char* pRgb)
{
    //VYUY - format
    int nBps = width*4;
    unsigned char* pY1 = pYuv+1;
    unsigned char* pV = pYuv;
    unsigned char* pU = pYuv + 3;

    unsigned char* pLine1 = pRgb;

    unsigned char y1,u,v;
    for (int i=0; i<height; i++)
    {
        for (int j=0; j<width; j+=2)
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
        pY1 += stride;
        pV += stride;
        pU += stride;
        pLine1 += nBps;

    }
}
//plan mode
void YuyvToRgb32_planmode(unsigned char* pYuv, int width, int height, unsigned char* pRgb)
{
    int nBps = width*4;
    unsigned char* pY1 = pYuv;
    unsigned char* pV = pYuv + width*height;
    unsigned char* pU = pV + width*height/4;
//    unsigned char* pVU = pYuv + width*height;
//    unsigned char* pV = pU + width*height/4;

    unsigned char* pLine1 = pRgb;

    unsigned char y1,u,v;
    for (int i=0; i<height; i++)
    {
        for (int j=0; j<width; j+=2)
        {
            y1 = pY1[j];
            u = pU[j/2];
            v = pV[j/2];
            pLine1[j*4] = YUV2B(y1, u, v);//b
            pLine1[j*4+1] = YUV2G(y1, u, v);//g
            pLine1[j*4+2] = YUV2R(y1, u, v);//r
            pLine1[j*4+3] = 0xff;

            y1 = pY1[j+1];
            pLine1[j*4+4] = YUV2B(y1, u, v);//b
            pLine1[j*4+5] = YUV2G(y1, u, v);//g
            pLine1[j*4+6] = YUV2R(y1, u, v);//r
            pLine1[j*4+7] = 0xff;
        }
        pY1 += width;
        pU += width/2;
        pV += width/2;
        pLine1 += nBps;

    }
}
static int m_nBytesPerFrame; //yuv frame
Camera* m_pCam = NULL;
int FramePostProcess(void* pInBuffer, CamProperty* pCp, void* pOut, void* data)
{
    CameraSource* pThis = (CameraSource*) data;
    //pCp->format == YUYV, stride is width*2
    return pThis->DoFramePostProcess(pInBuffer, pCp->width, pCp->height, pCp->width*2, pOut);
}

int CameraSource::DoFramePostProcess(void* pInBuffer, int width, int height, int stride, void* pOut)
{
    //YUV to RGB
 //   if(m_VideoFormat == V4L2_PIX_FMT_YUYV)//YUYV, PC
 //       nfYuyvToRgb32((unsigned char* )pInBuffer, width, stride, height, (unsigned char* )pOut, false);
 //   else //if (V4L2_PIX_FMT_VYUY == m_VideoFormat)
        YuyvToRgb32((unsigned char* )pInBuffer, width, stride, height, (unsigned char* )pOut);

    return m_nBytesPerFrame;
}

CameraSource::CameraSource()
{
    m_pData = NULL;
    init();
}

bool CameraSource::init()
{
    //find the max resolution for our experiment
    int nMaxCam = GetCameraManager()->MaxCamera();
    if (nMaxCam <= 0) {
        LOGE("No camera available. Exit!\n");
        return false;
    }
    LOGI("Found %d cameras\n", nMaxCam);
    unsigned int nMaxWidth = 0;
    int candidate = -1;

    //use first camera
    Camera* pCam = GetCameraManager()->GetCameraBySeq(0);
    if(! pCam)
        return false;
    int n=0;
    CamProperty* pCp = pCam->GetSupportedProperty(n);
    for(int i=0; i < n; i++) {
        if (pCp[i].width > nMaxWidth) {
//            if (pCp[i].width > 1800)//1920x1080,1280x720
//                break;
            nMaxWidth = pCp[i].width;
            candidate = i;
        }
        LOGI("Camera 0 width=%dx %d, format=0x0X fps = %d", pCp[i].width, pCp[i].height, pCp[i].format, pCp[i].fps);
    }

    if (candidate < 0) {
        LOGE("No camera found");
        return false;
    }
    m_nWidth = pCp[candidate].width;
    m_nHeight = pCp[candidate].height;
    m_VideoFormat = pCp[candidate].format;
    m_nBytesPerFrame = m_nWidth*4* m_nHeight;
    if(m_pData) free(m_pData);
    m_pData = (unsigned char*) malloc(m_nBytesPerFrame);

    bool ret = pCam->Open(& pCp[candidate]);
    if (!ret){
        LOGE("Open camera /dev/video%d failed!\n", pCam->GetId());
        return false;
    }
    m_pCam = pCam;
    m_pCam->Start(FramePostProcess, this);
    return true;
}

CameraSource::~CameraSource()
{
    if (m_pCam ) {
        m_pCam->Stop();
        m_pCam->Close();
        m_pCam = NULL; //no need to delete it, CameraManager do it.
    }

    if(m_pData) free(m_pData);
}
unsigned char * CameraSource::GetFrameData()
{
   int length = m_pCam->GetFrame(m_pData, m_nBytesPerFrame);
   if (length != m_nBytesPerFrame){
       perror("Data cropt!!\n");
       return NULL;
   }

    return m_pData;
}
