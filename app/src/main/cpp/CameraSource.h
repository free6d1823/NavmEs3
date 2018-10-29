#ifndef VIDEOSOURCE_H
#define VIDEOSOURCE_H
#include "stdio.h"

class CameraSource
{
public:
    CameraSource();
    ~CameraSource();
    void setSimFileRgb32(int width, int height, int depth, const char* szFile);
    void setSimFileYuv(int width, int height, int depth, const char* szFile);
    bool init();
    int Width(){return m_nWidth;}
    int Height() {return m_nHeight;}
    unsigned char * GetFrameData();
    int DoFramePostProcess(void* pInBuffer, int width, int height, int stride, void* pOut);
protected:
    int m_nWidth;
    int m_nHeight;
    unsigned int m_VideoFormat; //camera input format

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

};

#endif // VIDEOSOURCE_H
