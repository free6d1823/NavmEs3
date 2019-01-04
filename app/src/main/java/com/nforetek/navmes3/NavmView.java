/*
 * Copyright 2018 nFore Technology Inc.
 *
 */
package com.nforetek.navmes3;

import android.content.Context;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;

import android.content.res.AssetManager;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import static android.os.Environment.DIRECTORY_DOWNLOADS;

class NavmView extends GLSurfaceView {
    private static final String TAG = "NavmView";
    private static final boolean DEBUG = true;

    public NavmView(Context context) {
        super(context);

        // Pick an EGLConfig with RGB8 color, 16-bit depth, no stencil,
        // supporting OpenGL ES 2.0 or later backwards-compatible versions.
        setEGLConfigChooser(8, 8, 8, 0, 16, 0);
        setEGLContextClientVersion(3);
        setRenderer(new Renderer(context));
        /* find settings file name */
        NavmEs3Lib.init( context.getAssets(), context.getFilesDir().getAbsolutePath());

    }

    private static class Renderer implements GLSurfaceView.Renderer {
        private Context mContext;
        private int width_surface;
        private int height_surface ;
        public Renderer (Context context) {
            mContext = context;
        }
        public void onDrawFrame(GL10 gl) {
            NavmEs3Lib.step();
            if(1==NavmEs3Lib.bSaveTexture())
            {
                NavmEs3Lib.saveTexture(0);

                int w = width_surface ;
                int h = height_surface ;


            }

        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            NavmEs3Lib.resize(width, height);
            width_surface = width;
            height_surface = height;
        }


        final boolean mUseSimFile = true;
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {

            if (mUseSimFile) {
                String path = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath() + "/video_1440x960.rgb";
                Log.e(TAG, "Download is " + path);
                File file = new File(path);
                if (file.exists()) {
                    NavmEs3Lib.start2(path);
                    return;
                }
            }
            //use video input
            NavmEs3Lib.start();


        }
    }
}
