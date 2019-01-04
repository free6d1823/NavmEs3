#ifndef VIDEOSOURCE_H
#define VIDEOSOURCE_H
#include "stdio.h"
#include<pthread.h>

#include "CameraManager.h"
#define MAX_CAM 4
/* For SOUND SK82 project, use 640*2*480*2 as total image size */
#define IMAGE_FIELD_TYPE     V4L2_FIELD_INTERLACED

typedef struct _CameraData{
    Camera* pCam;
    unsigned char* pBuffer;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    void* pUser;
}CameraData;
class CameraSource
{
public:
    CameraSource();
    ~CameraSource();
    void setSimFileRgb32(int width, int height, int depth, const char* szFile);
    void setSimFileYuv(int width, int height, int depth, const char* szFile);
    bool init();
    //open and start specified cameras
    bool open(unsigned int camsBitMask);
    //close all cameras
    void close();

    int Width(){return mWidth;}
    int Height() {return mHeight;}
    unsigned char * GetFrameData();
    int DoFramePostProcess(void* pInBuffer, int width, int height, int stride, void* pOut);
    void Lock();
    void Unlock();
protected:
    int mWidth;
    int mHeight;
    unsigned int mVideoFormat; //camera input format

    //ping-pong buffers
    unsigned char* mpOutBuffer;//RGB888 output
    int mBytesPerFrameInput;    //w*h*depth
    int mBytesPerFrameOutput; //w*h*4


    bool mUseSim; /*<! true to use file to simulate scene*/
    int mCurFrame;
    int mTotalFrames;
    /*<! if mFp != NULL, use sim file
     * if mFp==NULL && mpCs != NULL, use camera
     * if mFp&mpCs == NULL, load image failed, no floor image */
    FILE* mFp; /*<! video file FILE pointer */
    unsigned char* mpTemp;   /*<! source buffer if colorconversion is needed*/

    CameraData mCam[MAX_CAM];
    pthread_mutex_t mLock;
    enum CAM_STATE {
        CS_NONE = 0,    //cameras first state
        CS_INITED = 1,  //resource inited
        CS_OPENED = 2  //camera opened and started
    };
    CAM_STATE   mState;
};

#endif // VIDEOSOURCE_H
