#ifndef VIDEOSOURCE_H
#define VIDEOSOURCE_H
#include "stdio.h"

class CameraSource
{
public:
    CameraSource();
    ~CameraSource();

    int Width(){return m_nWidth;}
    int Height() {return m_nHeight;}
    unsigned char * GetFrameData();
    int DoFramePostProcess(void* pInBuffer, int width, int height, int stride, void* pOut);
protected:
    bool init();
    int m_nWidth;
    int m_nHeight;
    unsigned int m_VideoFormat; //camera output format

    //ping-pong buffers
    unsigned char* m_pData;//RGB888
};

#endif // VIDEOSOURCE_H
