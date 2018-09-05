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
#include "Cube.h"
#define LOG_TAG "GLES3JNI"
#include "common.h"
#include "imglab/ImgProcess.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <cassert>

Cube  mCube;
Floor mFloor;
static void printGlString(const char* name, GLenum s) {
    const char* v = (const char*)glGetString(s);
    LOGI("GL %s: %s\n", name, v);
}

MainJni::MainJni() : mMode(0), mZoom(1), mAngle(0)
{
    changeViewMode(0);
}
MainJni:: ~MainJni() {
}
void MainJni::resize(int w, int h)
{
    glViewport(0, 0, w, h);
}

void MainJni::step()
{
    rotate(0.1);

    Mat4 proj;

    if(mMode == 0){
        Vec3 posCam = mDirCam;
        posCam.rotateY(mAngle);
        Mat4 view = Mat4::LookAt(mPosCam, mPosCenter, posCam);
        proj = mMatProj* view;
    }else {
        Vec3 posCam = mPosCam;
        posCam.rotateY(mAngle);
        Mat4 view = Mat4::LookAt(posCam, mPosCenter, mDirCam);
        proj = mMatProj* view;

    }

    mCube.update(proj);
    mFloor.update(proj);
}
void MainJni::render()
{

    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mFloor.draw();
//    mCube.draw();

    checkGlError("Renderer::render");
}

void MainJni::changeViewMode(int mode)
{
    mMode = mode;
    if (mMode == 0)//bird view
    {
        mPosCam = Vec3(0,7,0);
        mPosCenter = Vec3(0,0,0);
        mDirCam = Vec3(0,0,1);
        mMatProj= Mat4::Perspective(4,4,1,100);
    }else { //back view
        mPosCam = Vec3(0,0,-5);
        mPosCenter = Vec3(0,0,0);
        mDirCam = Vec3(0,1,0);
        mMatProj= Mat4::Perspective(1,1,1,100);
    }
}
void MainJni::zoom(float factor)
{

}
void MainJni::rotate(float degree)
{
    mAngle += degree * M_PI/ 180;
}

static MainJni* g_main = NULL;
// ----------------------------------------------------------------------------

extern "C" {
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_init(JNIEnv* env, jobject obj);
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_init2(JNIEnv* env, jobject obj, jobject assetManager);

    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_resize(JNIEnv* env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_nforetek_navmes3_NavmEs3Lib_step(JNIEnv* env, jobject obj);
};

JNIEXPORT void JNICALL
Java_com_nforetek_navmes3_NavmEs3Lib_init(JNIEnv* env, jobject obj) {
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
    mCube.init();


}

void nfYuyvToRgb32(nfImage* pYuv, unsigned char* pRgb, bool uFirst);
JNIEXPORT void JNICALL
Java_com_nforetek_navmes3_NavmEs3Lib_init2(JNIEnv* env, jobject obj, jobject assetManager) {
    // use asset manager to open asset by filename
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    assert(NULL != mgr);

    AAsset* testAsset = AAssetManager_open(mgr, "camera1800x1440.yuv", AASSET_MODE_UNKNOWN);
    if (testAsset)
    {
        assert(testAsset);

        size_t assetLength = AAsset_getLength(testAsset);

        LOGI("Native Audio Asset file size: %d\n", assetLength);
        nfImage* pSrc = nfImage::create(1800, 1440, assetLength/(1800*1440));
        nfPByte dest = mFloor.allocTextureImage(1800, 1440, 4);
        if (pSrc && dest) {
            AAsset_read(testAsset, pSrc->buffer, assetLength);
            nfYuyvToRgb32(pSrc, dest, true);
        }
        AAsset_close(testAsset);
        nfImage::destroy(&pSrc);
    }
    else
    {
        LOGE("Cannot open file in assets!");
    }

}
JNIEXPORT void JNICALL
Java_com_nforetek_navmes3_NavmEs3Lib_resize(JNIEnv* env, jobject obj, jint width, jint height) {
    if (g_main) {
        g_main->resize(width, height);
    }
    LOGD("resize %dx%d", width, height);

}

JNIEXPORT void JNICALL
Java_com_nforetek_navmes3_NavmEs3Lib_step(JNIEnv* env, jobject obj) {
    if (g_main) {
        g_main->step();
        g_main->render();
    }
}
