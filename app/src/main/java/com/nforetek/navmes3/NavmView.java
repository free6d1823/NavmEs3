/*
 * Copyright 2018 nFore Technology Inc.
 *
 */
package com.nforetek.navmes3;

import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

class NavmView extends GLSurfaceView {
    private static final String TAG = "NavmView";
    private static final boolean DEBUG = true;

    public NavmView(Context context) {
        super(context);
        // Pick an EGLConfig with RGB8 color, 16-bit depth, no stencil,
        // supporting OpenGL ES 2.0 or later backwards-compatible versions.
        setEGLConfigChooser(8, 8, 8, 0, 16, 0);
        setEGLContextClientVersion(3);
        setRenderer(new Renderer());
    }

    private static class Renderer implements GLSurfaceView.Renderer {
        public void onDrawFrame(GL10 gl) {
            NavmEs3Lib.step();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            NavmEs3Lib.resize(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            NavmEs3Lib.init();
        }
    }
}