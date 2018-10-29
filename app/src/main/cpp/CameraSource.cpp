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
    unsigned char* pY1 = pYuv;
    unsigned char* pV = pYuv+3;
    unsigned char* pU = pYuv + 1;

    unsigned char* pLine1 = pRgb;

    unsigned char y1,u,v;
    for (int i=0; i<height; i++)
    {
        for (int j=0; j<width; j++)
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
        pY1 += stride;
        pV += stride;
        pU += stride;
        pLine1 += nBps;

    }
}
void VyuyToRgb32(unsigned char* pYuv, int width, int stride, int height, unsigned char* pRgb)
{
    //VYUY - format
    int nBps = width*4;
    unsigned char* pY1 = pYuv+1;
    unsigned char* pV = pYuv;
    unsigned char* pU = pYuv + 2;

    unsigned char* pLine1 = pRgb;

    unsigned char y1,u,v;
    for (int i=0; i<height; i++)
    {
        for (int j=0; j<width; j++)
        {
            y1 = pY1[2*j];
            u = pU[4*j];
            v = pV[4*j];
            pLine1[j*4] = YUV2B(y1, u, v);//b
            pLine1[j*4+1] = YUV2G(y1, u, v);//g
            pLine1[j*4+2] = YUV2R(y1, u, v);//r
            pLine1[j*4+3] = 0xff;
            j++;
            y1 = pY1[2*j];//pY1[2*j];
            pLine1[j*4] = YUV2B(y1, u, v);//b
            pLine1[j*4+1] = YUV2G(y1, u, v);//g
            pLine1[j*4+2] = YUV2R(y1, u, v);//r
            pLine1[j*4+3] = 0xff;
        }
        pY1 += stride;
        pV += stride;
        pU += stride;
        pLine1 += nBps;

    }
}

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
//    YuyvToRgb32((unsigned char* )pInBuffer, width, stride, height, (unsigned char* )pOut);

    VyuyToRgb32((unsigned char* )pInBuffer, width, stride, height, (unsigned char* )pOut);

    return width*height*4;
}

CameraSource::CameraSource()
{
    mpOutBuffer = NULL;
    mFp = NULL;
    mpTemp = NULL;
    mTotalFrames = 0;
    mUseSim = false;
}
/* call this before init() if use SimFile */
void CameraSource::setSimFileRgb32(int width, int height, int depth, const char* szFile)
{
    SAFE_FREE(mpOutBuffer);
    SAFE_FREE(mpTemp);
    if (mFp ) {
        fclose(mFp);
    }
    mFp = fopen(szFile, "r");
    if (!mFp) {
        LOGE("Failed to open video simulation file %s!!", szFile);
        return;
    }
    m_nWidth = width;
    m_nHeight = height;
    m_VideoFormat = 'ABGR';
    mBytesPerFrameInput = m_nWidth*depth* m_nHeight;
    mBytesPerFrameOutput = m_nWidth*4* m_nHeight;
    if(mpOutBuffer) free(mpOutBuffer);
    mpOutBuffer = (unsigned char*) malloc(mBytesPerFrameOutput);
    fseek(mFp, 0, SEEK_END);
    long length = ftell(mFp);
     mTotalFrames = length /mBytesPerFrameOutput;
    fseek(mFp, 0, SEEK_SET);
    mUseSim = true;
}
void CameraSource::setSimFileYuv(int width, int height, int depth, const char* szFile)
{
    SAFE_FREE(mpOutBuffer);
    SAFE_FREE(mpTemp);
    if (mFp ) {
        fclose(mFp);
    }
    mFp = fopen(szFile, "r");
    if (!mFp) {
        LOGE("Failed to open video simulation file %s!!", szFile);
        return;
    }
    m_nWidth = width;
    m_nHeight = height;
    m_VideoFormat = 'YUYV';
    mBytesPerFrameInput = m_nWidth*depth* m_nHeight;
    mBytesPerFrameOutput = m_nWidth*4* m_nHeight;
    if(mpOutBuffer) free(mpOutBuffer);
    mpOutBuffer = (unsigned char*) malloc(mBytesPerFrameOutput);
    mpTemp = (unsigned char*) malloc(mBytesPerFrameInput); //use conversion
    fseek(mFp, 0, SEEK_END);
    long length = ftell(mFp);
    mTotalFrames = length /mBytesPerFrameInput;
    fseek(mFp, 0, SEEK_SET);
    mUseSim = true;
    LOGE("File length = %ld bytes %d frames", length, mTotalFrames);

}

bool CameraSource::init()
{
    if(mFp && mUseSim){
        return true;
    }
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
            if (pCp[i].width > 1900)//1920x1080,1280x720, 2592x1944, 1024x768
                break;
            nMaxWidth = pCp[i].width;
            candidate = i;
        }
        LOGI("Camera 0 width=%dx %d, format=0x%0X fps = %f", pCp[i].width, pCp[i].height, pCp[i].format, pCp[i].fps);
    }
    candidate = 1;
    if (candidate < 0) {
        LOGE("No camera found");
        return false;
    }
    m_nWidth = pCp[candidate].width;
    m_nHeight = pCp[candidate].height;
    m_VideoFormat = pCp[candidate].format;
    mBytesPerFrameOutput = m_nWidth*4* m_nHeight;
    SAFE_FREE(mpOutBuffer);
    mpOutBuffer = (unsigned char*) malloc(mBytesPerFrameOutput);

    bool ret = pCam->Open(& pCp[candidate]);
    if (!ret){
        LOGE("Open camera /dev/video%d failed!\n", pCam->GetId());
        return false;
    }
    LOGI("Use camera %dx%d %0X fps=%f",m_nWidth, m_nHeight,  m_VideoFormat, pCp[candidate].fps);
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

    SAFE_FREE(mpOutBuffer);

    //if use sim file
    if(mFp) {
        fclose(mFp);
        mFp = NULL;
        SAFE_FREE(mpTemp);
    }

}
unsigned char * CameraSource::GetFrameData()
{
    if (mUseSim && mFp ) {
        if (mCurFrame >= mTotalFrames) {
            fseek(mFp, 0, SEEK_SET);
            mCurFrame = 0;
        }

        if(mpTemp) {//input is YUYV
            fread(mpTemp, 1, mBytesPerFrameInput, mFp);
            YuyvToRgb32(mpTemp, m_nWidth, m_nWidth*2, m_nHeight, (unsigned char* )mpOutBuffer);
        }
        else //input is RGB32
            fread(mpOutBuffer, 1,mBytesPerFrameOutput, mFp);


    }else if(mpOutBuffer){

        int length = m_pCam->GetFrame(mpOutBuffer, mBytesPerFrameOutput);
        if (length != mBytesPerFrameOutput) {
            perror("Data cropt!!\n");
            return NULL;
        }
    }
    mCurFrame++;
    return mpOutBuffer;
}
