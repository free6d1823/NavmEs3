/*
 * Copyright 2018 nFore Technology Inc.
 *
 */

package com.nforetek.navmes3;

// Wrapper for native library

import android.content.res.AssetManager;

public class NavmEs3Lib {

     static {
          System.loadLibrary("navmes3");
     }

     public static native void init(AssetManager assetManager, String appFolder);
     /*<! call satrt2 instead of start, to use sim video file */
     public static native void start2(String simVideoFile);
     public static native void start();
     public static native void resize(int width, int height);
     public static native void step();

     public static native void rotate(float degree);
     public static native void zoom(float farther);
     public static native void setMode(int mode);
     public static native  int getMode();
     public static native void setAutoRun(int autorun);
     public static native void setOption(int nOption);
     public static native int saveTexture(int flag);
     public static native int bSaveTexture();
}
