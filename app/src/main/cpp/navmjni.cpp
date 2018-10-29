/*
 * Copyright 2018 nFore Technology Inc.
 *
 */

#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "navmjni.h"
#include "GlHelper.h"
#include "Floor.h"
#include "Car.h"
#include "Plat.h"
#define LOG_TAG "GLES3JNI"
#include "common.h"
#include "imglab/ImgProcess.h"
#include "inifile/inifile.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <cassert>
#include "./imglab/ImgProcess.h"

Car  mCar;
Floor mFloor;
Plat    mPlat1;
TexProcess gTexProcess;

#define MODE_BIRDVIEW   0
#define MODE_REARVIEW   1
#define MODE_FRONTVIEW  2
#define MODE_BACKVIEW  3
#define MODE_MASK       0x03
#define MODE_CRUISING   8

static void printGlString(const char* name, GLenum s) {
    const char* v = (const char*)glGetString(s);
    LOGI("GL %s: %s\n", name, v);
}

MainJni::MainJni() : mMode(0), mAngle(0)
{
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    // Enable back face culling
    glEnable(GL_CULL_FACE);
//    glCullFace(GL_BACK);//default
//    glCullFace(GL_FRONT);

    mAspectRatio  = 1;
    changeViewMode(MODE_BIRDVIEW);
}
MainJni:: ~MainJni() {
}
void MainJni::resize(int w, int h)
{
    glViewport(w-h, 0, h, h);
    mDisplayWidth = w;
    mDisplayHeight = h;
    mAspectRatio = 1;//(float)w/(float)h;
}

void MainJni::render()
{

    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(mDisplayWidth-mDisplayHeight, 0, mDisplayHeight, mDisplayHeight);

    mFloor.draw();
    mCar.draw();

    glViewport(0, 0, mDisplayWidth-mDisplayHeight, mDisplayHeight);
    mPlat1.draw();



    checkGlError("Renderer::render");
}
int MainJni::getViewMode()
{

    return (mMode & MODE_MASK);
}
void MainJni::setAutoRun(int value)
{
    if (value == 1)
        mMode |= MODE_CRUISING;
    else
        mMode &= (~MODE_CRUISING);
}

void MainJni::changeViewMode(int mode)
{
    if (mode >= 3) //I don't support mode 3
        mode =0;
    mMode = (mMode & ~MODE_MASK) | mode;

    LOGE(" ----- New mode is %d", mode);
    switch(mode) {
        /* no back view
        case MODE_BACKVIEW:
            mPosCam = Vec3(0,5,0);
            mPosCenter = Vec3(0,0,-10);
            mDirCam = Vec3(0,1,0);
            mMatProj= Mat4::Perspective(M_PI*60/180,mAspectRatio,1,20);
            break;*/
        case MODE_REARVIEW:
            mCamAngle = 45*M_PI/180;
            mCamDist = -10;
            mPosCam = Vec3(0,abs(mCamDist*sin(mCamAngle)),mCamDist*cos(mCamAngle));
            mPosCenter = Vec3(0,0,mCamDist*cos(mCamAngle)/2);
            mDirCam = Vec3(0,1,0);
            mMatProj= Mat4::Perspective(M_PI*45/180,mAspectRatio,1,20);

            break;
        case MODE_FRONTVIEW:
            mCamAngle = 45*M_PI/180;
            mCamDist = 10;
            mPosCam = Vec3(0,abs(mCamDist*sin(mCamAngle)),mCamDist*cos(mCamAngle));
            mPosCenter = Vec3(0,0,mCamDist*cos(mCamAngle)/2);
            mDirCam = Vec3(0,1,0);
            mMatProj= Mat4::Perspective(M_PI*45/180,mAspectRatio,1,20);
            break;

        case MODE_BIRDVIEW:
        default:
            mPosCam = Vec3(0,17,0);
            mPosCenter = Vec3(0,0,0);
            mDirCam = Vec3(0,0,1);
            mMatProj= Mat4::Perspective(M_PI*60/180,mAspectRatio,1,20);
            break;
    }
}
#define MAX_BIRDVIEW_ZOOM    18
#define MIN_BIRDVIEW_ZOOM    5
#define MAX_ZOOM    (50*M_PI/180)
#define MIN_ZOOM    (10*M_PI/180)
#define MAX_BACKVIEW_ZOOM    -5
#define MIN_BACKVIEW_ZOOM    -3
void MainJni::zoom(float factor)
{
    float newValue;
    switch(mMode & MODE_MASK)
    {
        /*
        case MODE_BACKVIEW://camera in car
            newValue = mPosCenter.getZ() - factor;//farther if more negative
            if (newValue < MAX_BACKVIEW_ZOOM)newValue = MAX_BACKVIEW_ZOOM;
            if (newValue > MIN_BACKVIEW_ZOOM)newValue = MIN_BACKVIEW_ZOOM;
            mPosCenter.setZ( newValue);
            break;
*/
        case MODE_REARVIEW: //camera out side
        case MODE_FRONTVIEW:
            mCamAngle += (factor/10);

            if (mCamAngle > MAX_ZOOM)mCamAngle = MAX_ZOOM;
            if (mCamAngle < MIN_ZOOM)mCamAngle = MIN_ZOOM;
            mPosCam = Vec3(0,abs(mCamDist*sin(mCamAngle)),mCamDist*cos(mCamAngle));
            mPosCenter = Vec3(0,0,mCamDist*cos(mCamAngle)/2);
            break;

        case MODE_BIRDVIEW:
        default:
            newValue = mPosCam.getY() + factor;
            if (newValue > MAX_BIRDVIEW_ZOOM)newValue = MAX_BIRDVIEW_ZOOM;
            if (newValue < MIN_BIRDVIEW_ZOOM)newValue = MIN_BIRDVIEW_ZOOM;
            mPosCam.setY( newValue);
            break;
    }
}
void MainJni::rotate(float degree)
{
    mAngle += degree * M_PI/ 180;
}

void MainJni::step()
{

    if(mMode & MODE_CRUISING)
        rotate(0.02);

    Mat4 proj;
    switch (mMode & MODE_MASK)
    {
        /*
        case MODE_BACKVIEW: {
            Vec3 pos = mPosCenter;
            pos.rotateY(-mAngle);
            Mat4 view = Mat4::LookAt(mPosCam, pos, mDirCam);
            proj = mMatProj * view;

        }
        break;*/
        case MODE_FRONTVIEW:
        case MODE_REARVIEW: {
            Vec3 posCam = mPosCam;
            posCam.rotateY(mAngle);
            Vec3 posCenter = mPosCenter;
            posCenter.rotateY(mAngle);
            Mat4 view = Mat4::LookAt(posCam, posCenter, mDirCam);
            proj = mMatProj * view;
        }
            break;
        case MODE_BIRDVIEW:
        default:{
            Vec3 posCam = mDirCam;
            posCam.rotateY(mAngle);
            Mat4 view = Mat4::LookAt(mPosCam, mPosCenter, posCam);
            proj = mMatProj * view;
        }

            break;
    }
    mCar.update(proj);
    mFloor.update(proj);
}
/* other ini helper functions *******************************************************************/
extern AAssetManager* gAmgr;

static bool CreateDefaultIniFile(const char* iniFile)
{
    time_t T= time(NULL);
//    struct  tm tm = *localtime(&T);

    FILE* fp = fopen(iniFile, "w");
    if (!fp)
        return false;
LOGI("copy default.ini to %s", iniFile);
    AAsset* testAsset = AAssetManager_open(gAmgr, "default.ini", AASSET_MODE_UNKNOWN);
    if (testAsset)
    {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);

        LOGI("default.ini  file size: %lu\n", assetLength);
        char* pSrc = (char*) malloc(assetLength);
        if (pSrc) {
            AAsset_read(testAsset, pSrc, assetLength);
            fwrite(pSrc, 1, assetLength, fp);
        }
        AAsset_close(testAsset);
        free(pSrc);
    }
    else
    {
        LOGE("Cannot open default ini file in assets!");
    }

    fclose(fp);
    return true;
}

static MainJni* g_main = NULL;
// ----------------------------------------------------------------------------

extern "C" {
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_init(JNIEnv* env, jobject  obj, jobject assetManager, jstring appFolder);
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_start2(JNIEnv* env, jobject  obj, jstring simVideoFile);
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_start(JNIEnv* env, jobject obj );
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_resize(JNIEnv* env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_step(JNIEnv* env, jobject obj );
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_rotate(JNIEnv* env, jobject obj , jfloat degree);
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_zoom(JNIEnv* env, jobject obj, jfloat farther);
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_setMode(JNIEnv* env, jobject obj, jint mode);
    JNIEXPORT jint  JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_getMode(JNIEnv* env, jobject obj);
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_setAutoRun(JNIEnv* env, jobject obj, jint autorun);

};

AAssetManager* gAmgr = NULL;

JNIEXPORT void JNICALL
Java_com_nforetek_navmes3_NavmEs3Lib_start2(JNIEnv* env, jobject obj, jstring simVideoFile) {
    if (g_main) {
        delete g_main;
        g_main = NULL;
    }
    g_main = new MainJni;
    if (gAmgr == NULL) {
        LOGE("ini() must be called first!");
        return;
    }
    const char* szFile = env->GetStringUTFChars(simVideoFile, 0);
    /*To use sim file, setSimVideoFileRgb, call before init() */
    mFloor.setSimVideoFileRgb(IMAGE_WIDTH, IMAGE_HEIGHT, 4, szFile);
    mFloor.init();

    AAsset* testAsset = AAssetManager_open(gAmgr, "redskin_100x100.yuv", AASSET_MODE_UNKNOWN);
    if (testAsset)
    {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);

        LOGI("Native Asset skin file size: %lu\n", assetLength);
        nfImage* pSrc = nfImage::create(100, 100, 2);
        nfPByte dest = mCar.allocTextureImage(100, 100, 4);
        if (pSrc && dest) {
            AAsset_read(testAsset, pSrc->buffer, assetLength);
            nfYuyvToRgb32(pSrc, dest, true, true);
        }
        AAsset_close(testAsset);
        nfImage::destroy(&pSrc);
    }
    else
    {
        LOGE("Cannot open skin image in assets!");
    }
    mCar.init();
    mPlat1.init();
    //
    testAsset = AAssetManager_open(gAmgr, "porch_body.bin", AASSET_MODE_UNKNOWN);
    if (testAsset)
    {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);
        void * pBuffer = malloc(assetLength);
        if (pBuffer) {
            AAsset_read(testAsset, pBuffer, assetLength);
            if(!mCar.loadObject(0, pBuffer, assetLength)){
                LOGE("Load 3D object 0 error!");
            }
            free(pBuffer);
        }
        AAsset_close(testAsset);

    }
    else
    {
        LOGE("Cannot open skin image in assets!");
    }
    testAsset = AAssetManager_open(gAmgr, "frontwheels.bin", AASSET_MODE_UNKNOWN);
    if (testAsset)
    {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);

        void * pBuffer = malloc(assetLength);
        if (pBuffer) {
            AAsset_read(testAsset, pBuffer, assetLength);
            if(!mCar.loadObject(1, pBuffer, assetLength)){
                LOGE("Load 3D object 1 error!");
            }
            free(pBuffer);
        }
        AAsset_close(testAsset);
    }
    else
    {
        LOGE("Cannot open skin image in assets!");
    }
    testAsset = AAssetManager_open(gAmgr, "rearwheels.bin", AASSET_MODE_UNKNOWN);
    if (testAsset)
    {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);

        void * pBuffer = malloc(assetLength);
        if (pBuffer) {
            AAsset_read(testAsset, pBuffer, assetLength);
            if(!mCar.loadObject(2, pBuffer, assetLength)){
                LOGE("Load 3D object 2 error!");
            }
            free(pBuffer);
        }
        AAsset_close(testAsset);
    }
    else
    {
        LOGE("Cannot open skin image in assets!");
    }
    ////
    LOGE("exit Java_com_nforetek_navmes3_NavmEs3Lib_start2" );
}


JNIEXPORT void JNICALL
Java_com_nforetek_navmes3_NavmEs3Lib_start(JNIEnv* env, jobject  obj ) {

    (void) obj;
    LOGE("Java_com_nforetek_navmes3_NavmEs3Lib_start" );


    if (g_main) {
        delete g_main;
        g_main = NULL;
    }
    printGlString("Version", GL_VERSION);
    printGlString("Vendor", GL_VENDOR);
    printGlString("Renderer", GL_RENDERER);
    printGlString("Extensions", GL_EXTENSIONS);

    g_main = new MainJni;

    mFloor.init();

    AAsset* testAsset;
    testAsset = AAssetManager_open(gAmgr, "redskin_100x100.yuv", AASSET_MODE_UNKNOWN);
    if (testAsset)
    {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);

        LOGI("Native Asset skin file size: %lu\n", assetLength);
        nfImage* pSrc = nfImage::create(100, 100, 2);
        nfPByte dest = mCar.allocTextureImage(100, 100, 4);
        if (pSrc && dest) {
            AAsset_read(testAsset, pSrc->buffer, assetLength);
            nfYuyvToRgb32(pSrc, dest, true, true);
        }
        AAsset_close(testAsset);
        nfImage::destroy(&pSrc);
    }
    else
    {
        LOGE("Cannot open skin image in assets!");
    }
    mCar.init();
    mPlat1.init();
    //
    testAsset = AAssetManager_open(gAmgr, "porch_body.bin", AASSET_MODE_UNKNOWN);
    if (testAsset)
    {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);
        void * pBuffer = malloc(assetLength);
        if (pBuffer) {
            AAsset_read(testAsset, pBuffer, assetLength);
            if(!mCar.loadObject(0, pBuffer, assetLength)){
                LOGE("Load 3D object 0 error!");
            }
            free(pBuffer);
        }
        AAsset_close(testAsset);

    }
    else
    {
        LOGE("Cannot open skin image in assets!");
    }
    testAsset = AAssetManager_open(gAmgr, "frontwheels.bin", AASSET_MODE_UNKNOWN);
    if (testAsset)
    {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);

        void * pBuffer = malloc(assetLength);
        if (pBuffer) {
            AAsset_read(testAsset, pBuffer, assetLength);
            if(!mCar.loadObject(1, pBuffer, assetLength)){
                LOGE("Load 3D object 1 error!");
            }
            free(pBuffer);
        }
        AAsset_close(testAsset);
    }
    else
    {
        LOGE("Cannot open skin image in assets!");
    }
    testAsset = AAssetManager_open(gAmgr, "rearwheels.bin", AASSET_MODE_UNKNOWN);
    if (testAsset)
    {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);

        void * pBuffer = malloc(assetLength);
        if (pBuffer) {
            AAsset_read(testAsset, pBuffer, assetLength);
            if(!mCar.loadObject(2, pBuffer, assetLength)){
                LOGE("Load 3D object 2 error!");
            }
            free(pBuffer);
        }
        AAsset_close(testAsset);
    }
    else
    {
        LOGE("Cannot open skin image in assets!");
    }
    ////
    LOGE("exit Java_com_nforetek_navmes3_NavmEs3Lib_start" );
}

void RunServer(int serverPort, const char* workFolder);

static int isInited = 0;
#define DEFAULT_SETTING_FILE "/navmsettings.ini"
void nfYuyvToRgb32(nfImage* pYuv, unsigned char* pRgb, bool uFirst);
JNIEXPORT void JNICALL
Java_com_nforetek_navmes3_NavmEs3Lib_init(JNIEnv* env, jobject obj, jobject assetManager, jstring appFolder) {

    if (isInited)
    {
        return;
    }
    LOGE("Java_com_nforetek_navmes3_NavmEs3Lib_init" );
    char szIniFile[256];
    // use asset manager to open asset by filename
    gAmgr = AAssetManager_fromJava(env, assetManager);
    assert(NULL != gAmgr);

    const char* szRootDir = env->GetStringUTFChars(appFolder, 0);
    strncpy(szIniFile, szRootDir, sizeof(szIniFile));
    strncat(szIniFile, DEFAULT_SETTING_FILE, sizeof (szIniFile));
    FILE* fp = fopen(szIniFile,"r");
    //FILE* fp = NULL; //force overwrite inifile
    if(!fp) {
        LOGE("Setting file %s not found, copy default settings.", szIniFile);
        CreateDefaultIniFile(szIniFile);
    } else
        fclose(fp);

    LOGI("SloadIniFile %s .", szIniFile);
    gTexProcess.loadIniFile2(szIniFile);
    LOGI("gTexProcess update ");
    gTexProcess.update(); //takes 10 secs
    LOGI("SloadIniFile %s .", szIniFile);

    isInited = 1;
    RunServer(9000, szRootDir);

    LOGE("ini file is %s", szIniFile);
}



JNIEXPORT void JNICALL
Java_com_nforetek_navmes3_NavmEs3Lib_resize(JNIEnv* env, jobject obj, jint width, jint height) {
    if (g_main) {
        g_main->resize(width, height);
    }
    LOGI("resize %dx%d", width, height);

}

JNIEXPORT void JNICALL
Java_com_nforetek_navmes3_NavmEs3Lib_step(JNIEnv* env, jobject obj) {
    if (g_main) {
        g_main->step();
        g_main->render();
    }
}
JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_rotate(JNIEnv* env, jobject obj, jfloat degree){
    if (g_main)
        g_main->rotate(degree);
}
JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_zoom(JNIEnv* env, jobject obj, jfloat farther)
{
    if (g_main)
        g_main->zoom( farther);

}
JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_setMode(JNIEnv* env, jobject obj, jint mode)
{
    if (g_main)
        g_main->changeViewMode(mode);
}
JNIEXPORT jint  JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_getMode(JNIEnv* env, jobject obj)
{
    if (g_main) return g_main->getViewMode();
    else return -1;
}
JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_setAutoRun(JNIEnv* env, jobject obj, jint autorun)
{
    if (g_main)
        g_main->setAutoRun(autorun);
}
