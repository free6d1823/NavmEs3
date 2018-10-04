/*
 * Copyright 2018 nFore Technology Inc.
 *
 */


#ifndef NAVMJNI_H
#define NAVMJNI_H 1

#include <android/log.h>
#include <math.h>

#if DYNAMIC_ES3
#include "gl3stub.h"
#else
// Include the latest possible header file( GL version header )
#if __ANDROID_API__ >= 24
#include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21
#include <GLES3/gl31.h>
#else
#include <GLES3/gl3.h>
#endif

#endif


#include "./imglab/vecmath.h"

// returns true if a GL error occurred
extern bool checkGlError(const char* funcName);

class MainJni {
public:
    virtual ~MainJni();
    void resize(int w, int h);
    void step();
    void render();

    void changeViewMode(int mode);
    int getViewMode();
    void setAutoRun(int value);

    void zoom(float factor);
    void rotate(float degree);
    MainJni();
private:
    float mDisplayWidth;
    float mDisplayHeight;
    float mAspectRatio;
    Vec3    mPosCam;
    Vec3    mPosCenter;
    Vec3    mDirCam;
    Mat4    mMatProj;
    int mMode;
    float mAngle; /*<! in radian */
    float mCamAngle; /*<!camera position pitch angle, used in rear/front view */
    float mCamDist; /*<!camera distance, used in rear/front view */
};
#endif // NAVMJNI_H
