#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H
#include <stdint.h>
#include <vector>
#include <pthread.h>

using namespace std;

typedef struct VideoBuffer {
    void   *start;
    size_t  length;
} VideoBuffer;

typedef struct {
    uint32_t    width;
    uint32_t    height;
    uint32_t    format; //RGB, YUV
    float         fps;
}CamProperty;
typedef void (*OnFrameCallback)(void* pBuffer, CamProperty* pCp, void* data);
typedef int (*OnFramePostProcess)(void* pInBuffer, CamProperty* pCp, void* pOut, void* data);

class Camera {
    friend class CameraManager;
public:
    Camera(int id, bool useCallback = false);
    ~Camera();
    int GetId(){return m_id;}
    bool Open(CamProperty* pCp);
    bool Close();
    bool Start(OnFrameCallback func, void* data);   //used in callback only
    bool Start(OnFramePostProcess func, void* data); //used in GetFrame only
    bool Stop();
    CamProperty* GetSupportedProperty(int& count);
    bool GetCurrentProperty(CamProperty* pCp);
    bool IsOpened() { return (m_fd >=0);}
    int GetFrame(void* buffer, int length);
    void DoFrameProcess();
private:
    int m_id; //dev/video##
    int m_nMaxProperties;
    CamProperty* m_pPropList;
    CamProperty m_curCamProperty;
    uint32_t m_nMaxBuffer;
    VideoBuffer*    m_pBuf;
    OnFrameCallback m_pfnOnFrame;
    OnFramePostProcess  m_pfnFramePostProcess;
    void*           m_pOnFrameData;
protected:
    int m_fd; //g.e. zero if device is opened
    pthread_t   m_threadFrame;
    int         m_nStopThread;
};
class CameraManager
{
public:
    CameraManager();
    ~CameraManager();
    int Reflesh();  /*<! re-init the camera, return the maximumal camera numbers */
    int MaxCamera();/*<! current max camera after previous reflesh */
    Camera* GetCamera(int id);
    Camera* GetCameraBySeq(int seq);/*<! get camera object by sequence number */

private:
    void Clean();
};
CameraManager* GetCameraManager();
#endif // CAMERAMANAGER_H
