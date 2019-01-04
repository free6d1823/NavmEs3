#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/stat.h>
#include "CameraManager.h"

#define LOG_TAG "CAMERA_SOURCE"
#include "common.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define MAX_BUFFER  4
static vector <Camera*>    m_listCam;
static CameraManager theCameraManager;
CameraManager* GetCameraManager(){return &theCameraManager;}


//Android
/*
#include <camera/NdkCameraManager.h>
ACameraManager* gpCameraManager = NULL;
*/
/******************************************************************
 *  Global functions
 * ****************************************************************/
static int xioctl(int fd, int request, void* arg)
{

    int r;

    do {
        r = ioctl(fd, request, arg);
    }while(r == -1 &&  EINTR == errno);

    return r;
    /*
  for (int i = 0; i < 100; i++) {
    int r = ioctl(fd, request, arg);
    if (r != -1 || errno != EINTR) return r;
  }
  return -1;*/
}

/******************************************************************
 *  \class CameraManager
 * ****************************************************************/
 /*<! current max camera after previous reflesh */
int CameraManager::MaxCamera()
{
    return m_listCam.size();
}
void CameraManager::Clean()
{
    Camera* pCam = m_listCam.back();
    while (pCam){
        delete pCam;
        m_listCam.pop_back();
        pCam = m_listCam.back();
    }
    Reflesh();
}
CameraManager::CameraManager()
{

/*    gpCameraManager = NULL;
    gpCameraManager = ACameraManager_create();
LOGE("---- enter new CameraManager");
    if (gpCameraManager) {
        ACameraIdList* pList;
        camera_status_t ret = ACameraManager_getCameraIdList(gpCameraManager,&pList);
        if (ret == ACAMERA_OK ){
            m_listCam.clear();
            for (int i=0; i< pList->numCameras; i++){
LOGE("%d -- cam=%s", i, pList->cameraIds[i]);
            }
            ACameraManager_deleteCameraIdList(pList);
        }
    }
*/
    m_listCam.clear();
    Reflesh();
}
CameraManager::~CameraManager()
{

}
int EnumFpsByFrameSize(int fd, v4l2_frmivalenum& fit, CamProperty* pCp, int& n)
{
    int ret = xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS,&fit);
    if ( 0 != ret)
        return n;
    if (fit.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
        do {
            pCp[n].width = fit.width;
            pCp[n].height = fit.height;
            pCp[n].format = fit.pixel_format;
            pCp[n].fps = fit.discrete.denominator /fit.discrete.numerator;
            fit.index ++;
            ret = xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS,&fit);
            n++;
        }while (ret == 0);
    } else {
        //V4L2_FRMIVAL_TYPE_CONTINUOUS ||V4L2_FRMIVAL_TYPE_STEPWISE
        float min = (float) fit.stepwise.min.numerator/(float)fit.stepwise.min.denominator;
        float max = (float) fit.stepwise.max.numerator/(float)fit.stepwise.max.denominator;
        float dd =  (float) fit.stepwise.step.numerator/(float)fit.stepwise.step.denominator;

        for (float ff=min; ff <=max; ff+= dd){
            pCp[n].width = fit.width;
            pCp[n].height = fit.height;
            pCp[n].format = fit.pixel_format;
            pCp[n].fps = 1.0/ff;
            n++;
        }
    }
    return n;
}

int EnumFrameSizeByPixelFormat(int fd, v4l2_frmsizeenum& fse, CamProperty* pCp, int& n)
{
    v4l2_frmivalenum fit;

    int ret = xioctl(fd, VIDIOC_ENUM_FRAMESIZES,&fse);
    //get resolution
    if ( 0 != ret)
       return n;
    CLEAR(fit);
    fit.pixel_format = fse.pixel_format;
    if (fse.type == V4L2_FRMSIZE_TYPE_DISCRETE || fse.type == 0){//
        do {

            fit.width = fse.discrete.width;
            fit.height = fse.discrete.height;
            fit.index = 0;
            EnumFpsByFrameSize(fd, fit, pCp, n);
            fse.index ++;
            ret = xioctl(fd, VIDIOC_ENUM_FRAMESIZES,&fse);
        }while (ret == 0);

    } else {//V4L2_FRMIVAL_TYPE_CONTINUOUS(2) |V4L2_FRMIVAL_TYPE_STEPWISE(3)
        fit.width = fse.stepwise.min_width;
        fit.height = fse.stepwise.min_height;
        fit.index = 0;
        EnumFpsByFrameSize(fd, fit, pCp, n);
    }
    return n;
}
/*<! re-init the camera, return the maximumal camera numbers */
int CameraManager::Reflesh()
{
    char device[32];
    int i;
LOGI("--- CameraManager::Reflesh()");

    for (i=0;i<18; i++){

        struct stat st;


        sprintf(device, "/dev/video%d", i);

        if (-1 == stat(device, &st))
            continue;

        if (!S_ISCHR(st.st_mode))
            continue;

LOGI("-- open cam %s", device);
        mfd[i] = open(device, O_RDWR | O_NONBLOCK, 0);

        if (mfd[i]  < 0)
            continue;
LOGI("--found device");

        struct v4l2_capability cap;
        //find capture and streaming device
        if ( (xioctl(mfd[i] , VIDIOC_QUERYCAP, &cap) == -1) || !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
             || !(cap.capabilities & V4L2_CAP_STREAMING)) {
            close(mfd[i] );
            continue;
        }
        if(cap.capabilities & V4L2_CAP_READWRITE){
            LOGI("Cam%d Support V4L2_CAP_READWRITE", i);
        }
        if(cap.capabilities & V4L2_CAP_STREAMING){
            LOGI("Cam%d Support V4L2_CAP_STREAMING", i);
        }
LOGI(" -- VIDIOC_QUERYCAP V4L2_CAP_VIDEO_CAPTURE V4L2_CAP_STREAMING OK");
        //CamProperty cp[100];
        int n = 0; //number of property set
        struct v4l2_fmtdesc fmt;
        CLEAR(fmt);
        fmt.index = 0;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //get pixel format
        //To enumerate image formats applications initialize the type and index field of struct v4l2_fmtdesc
        //and call the ioctl VIDIOC_ENUM_FMT ioctl with a pointer to this structure. Drivers fill the rest
        //of the structure or return an EINVAL error code. All formats are enumerable by beginning at
        //index zero and incrementing by one until EINVAL is returned.
        uint32_t pixelformat = 0;
        while (0 == xioctl(mfd[i] , VIDIOC_ENUM_FMT,&fmt)){
            //some driver never end, so we check the fmt, break if the same as before
            if (pixelformat == fmt.pixelformat)
                break;
            pixelformat = fmt.pixelformat;

            v4l2_frmsizeenum fse;
            CLEAR(fse);
            fse.index = 0;
            fse.pixel_format = fmt.pixelformat;
            EnumFrameSizeByPixelFormat(mfd[i] , fse, m_cp, n);

            if (n>= 80) { //this is a rough check; n could overfloat within above functions.
                break;
            }
            fmt.index ++;
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        }
        close(mfd[i]);

        m_pCam[i] = new Camera(i);

        m_pCam[i]->m_nMaxProperties = n;
        m_pCam[i]->m_pPropList = (CamProperty*) malloc(sizeof(CamProperty)*n);
        for (int k=0; k<n; k++){
            //pCam->m_pPropList[k] = cp[k];
            m_pCam[i]->m_pPropList[k] = m_cp[k];
        }

        m_listCam.push_back( m_pCam[i]);
        //close(mfd[i]);
        LOGI("Add cam %d",  m_pCam[i]->m_id);
        //try next dev nodes

#if 0
        Camera* pCam = new Camera(i);
        pCam->m_nMaxProperties = n;
        pCam->m_pPropList = (CamProperty*) malloc(sizeof(CamProperty)*n);
        for (int k=0; k<n; k++){
            pCam->m_pPropList[k] = cp[k];
        }
        m_listCam.push_back(pCam);
        //close(fd);
        //try next dev nodes
#endif
    }

    return (int) m_listCam.size();
}
/*<! open a camera object */
Camera* CameraManager::GetCamera(int id)
{
    vector<Camera*>::iterator it;
    for(it=m_listCam.begin(); it!=m_listCam.end(); ++it) {
        if ( (*it)->m_id == id)
            return ((*it));
    }
    return NULL;
}
/*<! get camera object by sequence number */
Camera* CameraManager::GetCameraBySeq(int seq)
{
    if (seq < (int) m_listCam.size())
        return m_listCam[seq];
    return NULL;
}

#define ReturnError(...) \
{                           \
    close(m_fd);           \
    m_fd = -1;                  \
    fprintf(stderr, ##__VA_ARGS__);    \
    return false;               \
}
/******************************************************************
 *  \class Camera
 * ****************************************************************/
CamProperty* Camera::GetSupportedProperty(int& count)
{
    count = m_nMaxProperties;
    return m_pPropList;
}
bool Camera::GetCurrentProperty(CamProperty* pCp)
{
    if (!m_fd){
        perror("Device not opened~\n");
        return false;
    }
    v4l2_format fmt;
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ( 0 != xioctl(m_fd, VIDIOC_G_FMT,&fmt)) {
        ReturnError("VIDIOC_G_FMT error!\n");
    }
    pCp->width = fmt.fmt.pix.width;
    pCp->height = fmt.fmt.pix.height;
    pCp->format = fmt.fmt.pix.pixelformat;

    /*VIDIOC_G_PARM no support, no message shown
    v4l2_streamparm smp;
    CLEAR(smp);
    smp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ( 0 != xioctl(m_fd, VIDIOC_G_PARM,&fmt) || smp.parm.capture.timeperframe.numerator==0) {
        perror("VIDIOC_G_PARM error!\n");
        pCp->fps = 1; //not support
    }else {
        printf("ps = %d/%d\n", smp.parm.capture.timeperframe.denominator, smp.parm.capture.timeperframe.numerator);
    pCp->fps = (float)smp.parm.capture.timeperframe.denominator /(float) smp.parm.capture.timeperframe.numerator;
    }
    */
    return true;
}
Camera::Camera(int id, bool useCallback)
{
    m_id = id;
    m_pPropList = NULL;
    m_fd = -1;
    m_nMaxBuffer = 0;   //real allocated buffer counts
    m_pBuf = NULL;
    m_pfnOnFrame = NULL;
    m_pOnFrameData = NULL;
    m_pfnFramePostProcess = NULL;
    m_nStopThread = 0;
}
Camera::~Camera()
{
    if (m_pPropList)
        free (m_pPropList);
    if (m_fd >= 0)
        close(m_fd);
    if(m_pBuf)
        free(m_pBuf);
}
bool Camera::Open(CamProperty* pCp)
{
    if (m_fd >=0) {
        printf("Camera has been opened before.\n");
        return true;
    }
    char device[32];
    sprintf(device, "/dev/video%d", m_id);
    m_fd = open(device, O_RDWR | O_NONBLOCK, 0);
    //int fd = open(device, O_RDWR | O_NONBLOCK, 0);
    if (m_fd < 0) {
        fprintf(stderr, "Open device %s failed!!\n", device);
        return false;
    }
    //

    //m_fd = fd;
    v4l2_format fmt;
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ( 0 != xioctl(m_fd, VIDIOC_G_FMT,&fmt)) {
        ReturnError("VIDIOC_G_FMT error!\n");
    }
#ifdef _SET_FORMAT_
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = pCp->width; //replace
    fmt.fmt.pix.height      = pCp->height; //replace
    fmt.fmt.pix.pixelformat = pCp->format; //replace
    fmt.fmt.pix.field = pCp->field;//V4L2_FIELD_ANY;
    printf("intent setting is %dx%d format %08X \n", fmt.fmt.pix.width, fmt.fmt.pix.height,
           fmt.fmt.pix.pixelformat);

    if ( 0 != xioctl(m_fd, VIDIOC_S_FMT,&fmt)) {
        ReturnError("VIDIOC_S_FMT error!\n");
    }
    /*Wandboard no support, returns "mxc_v4l2_s_param: vidioc_int_s_parm returned an error -1 "
    v4l2_streamparm smp;
    CLEAR(smp);
    smp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    smp.parm.capture.timeperframe.numerator = 1;
    smp.parm.capture.timeperframe.denominator = pCp->fps;
    if ( 0 != xioctl(fd, VIDIOC_S_PARM,&fmt)) {
        perror("VIDIOC_S_PARM error!\n");
    }*/
#endif
    struct v4l2_requestbuffers  req;
    CLEAR(req);
    req.count = MAX_BUFFER;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (xioctl(m_fd, VIDIOC_REQBUFS, &req) == -1) {
        if (EINVAL == errno) {
             ReturnError( "%s does not support memory mapping\n", device);
        }
        ReturnError( "VIDIOC_REQBUFS failed!\n");
    }
    if (req.count < 2) {
      ReturnError("Insufficient buffer memory on %s\n", device);
    }
    if (m_pBuf)free(m_pBuf);
    m_nMaxBuffer = req.count;
    m_pBuf = (VideoBuffer*) calloc(req.count, sizeof(VideoBuffer));
    for (size_t i = 0; i < m_nMaxBuffer; ++i) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;
        if (-1 == xioctl(m_fd, VIDIOC_QUERYBUF, &buf))
            ReturnError("VIDIOC_QUERYBUF error\n");

        m_pBuf[i].length = buf.length;
        m_pBuf[i].start =
                mmap(NULL /* start anywhere */,
                        buf.length,
                        PROT_READ | PROT_WRITE /* required */,
                        MAP_SHARED /* recommended */,
                     m_fd, buf.m.offset);
        if (MAP_FAILED == m_pBuf[i].start)
            ReturnError("mmap error \n");
    }

    for (size_t i = 0; i < m_nMaxBuffer; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == xioctl(m_fd, VIDIOC_QBUF, &buf))
                    ReturnError("VIDIOC_QBUF error\n");
    }
    m_curCamProperty = *pCp;

    return true;
}

void *DoThread(void *argument)
{
    Camera* THIS = (Camera*)argument;
    THIS->DoFrameProcess();
    return (void*) 0;
}
void Camera::DoFrameProcess()
{
    while(m_nStopThread == 0)
    {
        struct v4l2_buffer buf;
        CLEAR(buf);
        if(!m_pBuf) {
            perror("Device not open yet\n");
            break;
        }
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (-1 == xioctl(m_fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
                case EAGAIN:
                    usleep(500);
                    continue;
                default:
                    fprintf(stderr, "VIDIOC_DQBUF error!\n");
                break;
            }
        }
        void* pBuf = m_pBuf[buf.index].start;
        if (m_pfnOnFrame) {
            m_pfnOnFrame(pBuf, &m_curCamProperty, m_pOnFrameData);
        }
        if (-1 == xioctl(m_fd, VIDIOC_QBUF, &buf)){
            fprintf(stderr, "VIDIOC_QBUF error\n");
            break;
        }
    }
}
bool Camera::Start(OnFrameCallback func, void* data)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (func == NULL) {
        ReturnError("Must provide OnFrameCallback \n");
    }
    if (-1 == xioctl(m_fd, VIDIOC_STREAMON, &type)){
        ReturnError("VIDIOC_STREAMON error\n");
    }
    m_pfnOnFrame = func;
    m_pOnFrameData = data;
   //use callback method
    m_nStopThread = 0;
    pthread_create(&m_threadFrame, NULL, DoThread, (void *) this);
    return true;
}
bool Camera::Start(OnFramePostProcess func, void* data)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(m_fd, VIDIOC_STREAMON, &type)){
        LOGE( "%d (mfd=%d)VIDIOC_STREAMON(%d) error(%d):%s", GetId(), m_fd, type, errno, strerror(errno));
        //ReturnError("VIDIOC_STREAMON error\n");
    }
    m_pfnOnFrame = NULL;
    m_pfnFramePostProcess = func;
    m_pOnFrameData = data;

    return true;
}
bool Camera::Stop()
{
    if(m_nStopThread == 0){
        m_nStopThread = 1;
        pthread_join(m_threadFrame, NULL);
    }
    m_nStopThread = 0;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(m_fd, VIDIOC_STREAMOFF, &type)){
        ReturnError("VIDIOC_STREAMON error\n");
    }
    return true;
}
bool Camera::Close()
{
    if (m_fd >= 0){
        if (m_pBuf) {
            for (size_t i=0; i< m_nMaxBuffer;i ++){
                if (-1 == munmap(m_pBuf[i].start, m_pBuf[i].length))
                    ReturnError("munmap error\n");
            }
            free(m_pBuf);
            m_pBuf = NULL;
        }
        close(m_fd);
    }
    m_fd = -1;
    return true;
}
/* \func get video frame
 * \param [in] buffer buffer to receive data
 * \param length allocated bytes
 * \return length copied. negative if error;
 */
int Camera::GetFrame(void* buffer, int length)
{
    if(m_pfnOnFrame){
        fprintf(stderr, "GetFrame cannot be called in callback mode!!\n");
        return -1;
    }
    struct v4l2_buffer buf;
    CLEAR(buf);
    if(!m_pBuf) {
        perror("Device not open yet\n");
        return -1;
    }
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    while (-1 == xioctl(m_fd, VIDIOC_DQBUF, &buf)) {
        if (errno == EAGAIN) {
            usleep(500);
            continue;
        }
        fprintf(stderr, "VIDIOC_DQBUF error!\n");
        return -1;
    }

    /* ============= user provided post-process function ===========*/
    if(m_pfnFramePostProcess) {
        //buf.bytesused == m_curCamProperty.width*height*2
        length = m_pfnFramePostProcess(m_pBuf[buf.index].start, &m_curCamProperty,
                buffer, m_pOnFrameData);
    } else { //no
        if ( length < (int) buf.bytesused){
            fprintf(stderr, "Buffer too short, need %d bytes, you allocated %d bytes\n",  buf.bytesused, length);
            return -2;
        };
        length = buf.bytesused;
        memcpy(buffer, m_pBuf[buf.index].start,length);
    }
    /* ======================== return the buffer back ========================*/
    if (-1 == xioctl(m_fd, VIDIOC_QBUF, &buf)){
        fprintf(stderr, "VIDIOC_QBUF error\n");
    }

    return (int) length;
}
