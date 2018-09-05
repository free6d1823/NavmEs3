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

     public static native void init(AssetManager assetManager);
     public static native void start();
     public static native void resize(int width, int height);
     public static native void step();
}
