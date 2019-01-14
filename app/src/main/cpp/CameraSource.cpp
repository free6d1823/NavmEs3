#include <stdlib.h>
#include <memory.h>
#define LOG_TAG "CAMERA_SOURCE"
#include "common.h"

#include "CameraManager.h"
#include "./imglab/ImgProcess.h"
#include "CameraSource.h"
#include <linux/videodev2.h>
#include <unistd.h>


#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)


// YUV -> RGB
#define C(Y) ( (Y) - 16  )
#define D(U) ( (U) - 128 )
#define E(V) ( (V) - 128 )

#define YUV2R(Y, U, V) CLIP(( 298 * C(Y)              + 409 * E(V) + 128) >> 8)
#define YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
#define YUV2B(Y, U, V) CLIP(( 298 * C(Y) + 516 * D(U)              + 128) >> 8)

//#define _CHECK_FPS_
#define _FMT_NO_CSC_
void CopyToOutput(unsigned char* pIn, int width, int stride, int height, unsigned char* pRgb, unsigned int outStride)
{
    //VYUY - format
    unsigned char* pY1 = pIn;
    unsigned char* pLine1 = pRgb;
    int bSkip;


    for (int i=0; i<height; i++)
    {
        bSkip = (i%2);
        if(bSkip ==0) {
            memcpy((unsigned char *) pLine1, (unsigned char *) pY1, stride);
            pLine1 += outStride;
        }
        pY1 += stride;

    }
}

void VyuyToRgb32(unsigned char* pYuv, int width, int stride, int height, unsigned char* pRgb, unsigned int outStride)
{
    //VYUY - format
    unsigned char* pY1 = pYuv;
    unsigned char* pLine1 = pRgb;

    unsigned char y1,u,v,y2;
    unsigned char* py;
    unsigned char* pr;
    int  bSkip;

#ifdef _CHECK_FPS_
    return;
#endif
    for (int i=0; i<height; i++)
    {
        py = pY1;
        pr = pLine1;
        bSkip  = (i%2);
        if(bSkip==0) {
            for (int j = 0; j < width; j += 2) {
                v = *py++;
                y1 = *py++;
                u = *py++;
                y2 = *py++;

                *pr++ = YUV2B(y1, u, v);//b
                *pr++ = YUV2G(y1, u, v);//g
                *pr++ = YUV2R(y1, u, v);//r
                *pr++ = 0xff;
                *pr++ = YUV2B(y2, u, v);//b
                *pr++ = YUV2G(y2, u, v);//g
                *pr++ = YUV2R(y2, u, v);//r
                *pr++ = 0xff;
            }
            pLine1 += outStride;
        }
        pY1 += stride;

     }
}

int FramePostProcess(void* pInBuffer, CamProperty* pCp, void* pOut, void* data)
{
    CameraData* pThis = (CameraData*) data;
    /*static int nn = 0;
    if (nn< 8) {
        LOGI("callback %d: id = %d, in=%p, out=%p(%p)", nn, pThis->pCam->GetId(), pInBuffer, pOut, pThis->pBuffer);
        nn++;
    }*/
    LOGV("callback id = %d\n", pThis->pCam->GetId());
#ifdef _FMT_NO_CSC_
    CopyToOutput((unsigned char* )pInBuffer, pCp->width, pCp->width*2,  pCp->height,
                 (unsigned char* )pOut, pThis->stride*2);
#else
    VyuyToRgb32((unsigned char* )pInBuffer, pCp->width, pCp->width*2,  pCp->height,
                (unsigned char* )pOut, pThis->stride);
#endif
    return 0;
}
void FrameCallback(void* pBuffer, CamProperty* pCp, void* data)
{


    CameraData* pThis = (CameraData*) data;
    static int nn = 0;
    if (nn< 8) {
        LOGI("FrameCallback %d: id= %d", nn, pThis->pCam->GetId());
        nn++;
    }

    ((CameraSource*)pThis->pUser)->Lock();
    VyuyToRgb32((unsigned char* )pBuffer, pCp->width, pCp->width*2,  pCp->height,
                pThis->pBuffer, pThis->stride);
    ((CameraSource*)pThis->pUser)->Unlock();

}
CameraSource::CameraSource() :  mpOutBuffer(0)
{
    mFp = NULL;
    mpTemp = NULL;
    mTotalFrames = 0;
    mUseSim = false;
    memset(mCam, 0, sizeof(mCam));
    mState = CS_NONE;

    pthread_mutex_init(&mLock, NULL);
}
void CameraSource::Lock()
{
    pthread_mutex_lock(&mLock);
}
void CameraSource::Unlock()
{
    pthread_mutex_unlock(&mLock);
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
    mWidth = width;
    mHeight = height;
    mVideoFormat = 'ABGR';
    mBytesPerFrameInput = mWidth*depth* mHeight;
    mBytesPerFrameOutput = mWidth*4* mHeight;
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
    mWidth = width;
    mHeight = height;
    mVideoFormat = 'YUYV';
    mBytesPerFrameInput = mWidth*depth* mHeight;
    mBytesPerFrameOutput = mWidth*2* mHeight;
    if(mpOutBuffer) free(mpOutBuffer);
    mpOutBuffer = (unsigned char*) malloc(mBytesPerFrameOutput);
    fseek(mFp, 0, SEEK_END);
    long length = ftell(mFp);
    mTotalFrames = length /mBytesPerFrameInput;
    fseek(mFp, 0, SEEK_SET);
    mUseSim = true;
    LOGE("File length = %ld bytes %d frames", length, mTotalFrames);

}

bool CameraSource::init()
{
    if (mState > CS_NONE)
        return true;
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
    mWidth = IMAGE_WIDTH;
    mHeight = IMAGE_HEIGHT;

    mBytesPerFrameOutput = mWidth*2* mHeight;
    SAFE_FREE(mpOutBuffer);
    mpOutBuffer = (unsigned char*) malloc(mBytesPerFrameOutput);
    mVideoFormat = V4L2_PIX_FMT_UYVY;

    mState = CS_INITED;
    return true;
}
//open and start specified cameras, cam0 = bit0 set
bool CameraSource::open(unsigned int camsBitMask)
{
    if (mState == CS_OPENED)
        close();
    if (mState < CS_INITED) {
        if (!init())
            return false;
    }
    int i;
    pthread_mutex_lock(&mLock);
    for (i=0; i<MAX_CAM; i++) {
         mCam[i].pCam = NULL;

        if ((camsBitMask & (1<<i)) ==0 )
            continue;

        Camera *pCam = GetCameraManager()->GetCameraBySeq(i);
        if (!pCam) {
            pthread_mutex_unlock(&mLock);
            return false;
        }
        CamProperty cp;
        cp.width = IMAGE_WIDTH;
        cp.height = IMAGE_HEIGHT;
        cp.format = V4L2_PIX_FMT_UYVY;//V4L2_PIX_FMT_RGB32;
        cp.field = IMAGE_FIELD_TYPE;
        cp.fps = 30.0;
        bool ret = pCam->Open(&cp);
        if (!ret) {
            LOGE("Open camera /dev/video%d failed!\n", pCam->GetId());
            continue;
        }

        mCam[i].pCam = pCam;
        mCam[i].width = mWidth ;
        mCam[i].height = mHeight ;
        mCam[i].stride = mWidth * 2;
        mCam[i].pUser = this;
        switch (i) {
            case 1: //right
                mCam[i].pBuffer = mpOutBuffer + (mWidth / 2) * 4;
                break;
            case 2: //rear
                mCam[i].pBuffer = mpOutBuffer + mWidth * 4 * mHeight / 2 + mWidth *4 / 2;
                break;
            case 3: //left
                mCam[i].pBuffer = mpOutBuffer + mWidth * 4 * mHeight / 2;
                break;
            default: //0 front
                mCam[i].pBuffer = mpOutBuffer;
                break;
        }
LOGI("Start cam %d", i);
        if(!pCam->Start(FramePostProcess, (mCam + i))){
            mCam[i].pCam = NULL;

        }

    }
    mState = CS_OPENED;
    pthread_mutex_unlock(&mLock);
    return true;
}
//close all cameras
void CameraSource::close()
{
    pthread_mutex_lock(&mLock);
    for (int i=0; i<MAX_CAM; i++) {
        if (mCam[i].pCam) {
            mCam[i].pCam->Stop();
            mCam[i].pCam->Close();
            mCam[i].pCam = NULL;
        }
    }
    mState = CS_INITED;
    pthread_mutex_unlock(&mLock);
}
CameraSource::~CameraSource()
{
    close();
    SAFE_FREE(mpOutBuffer);
        //if use sim file
    if(mFp) {
        fclose(mFp);
        mFp = NULL;
        SAFE_FREE(mpTemp);
    }

    pthread_mutex_destroy(&mLock);
}

unsigned char * CameraSource::GetFrameData()
{
    pthread_mutex_lock(&mLock);
    if (mUseSim && mFp) {
        if (mCurFrame >= mTotalFrames) {
            fseek(mFp, 0, SEEK_SET);
            mCurFrame = 0;
        }
        fread(mpOutBuffer, 1, mBytesPerFrameOutput, mFp);
        usleep(33000);
        mCurFrame++;

    } else if (mpOutBuffer) {

        for (int i = 0; i < MAX_CAM; i++) {
            if (mCam[i].pCam) {
                mCam[i].pCam->GetFrame(mCam[i].pBuffer, mWidth * mHeight);
            }
        }
    }
    pthread_mutex_unlock(&mLock);
    return mpOutBuffer;
}

