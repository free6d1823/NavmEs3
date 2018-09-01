/*
 * Copyright 2018 nFore Technology Inc.
 *
 */

package com.nforetek.navmes3;

// Wrapper for native library

public class NavmEs3Lib {

     static {
          System.loadLibrary("navmes3");
     }

     public static native void init();
     public static native void resize(int width, int height);
     public static native void step();
}
