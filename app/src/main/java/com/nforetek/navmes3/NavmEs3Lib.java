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

     public static native void init(AssetManager assetManager, String iniFile);
     public static native void start();
     public static native void resize(int width, int height);
     public static native void step();

     public static native void rotate(float degree);
     public static native void zoom(float farther);
     public static native void setMode(int mode);
     public static native  int getMode();
     public static native void setAutoRun(int autorun);

}
