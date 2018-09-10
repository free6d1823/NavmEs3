/*
 * Copyright 2018 nFore Technology Inc.
 *
 */

package com.nforetek.navmes3;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.graphics.Point;
import android.os.Bundle;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import java.io.File;

public class MainActivity extends Activity {
    private GestureDetector mDetector;

    NavmView mView;
    float mRotateThresholdMin = 500; //area to rotate scene
    float mRotateThresholdMax = 500; //area to rotate scene

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        mView = new NavmView(getApplication());
        setContentView(mView);

        mDetector = new GestureDetector(this, new MyGestureListener());
        mView.setOnTouchListener(touchListener);

        Point size = new Point();
        getWindowManager().getDefaultDisplay().getSize(size);
        mRotateThresholdMin = size.y *0.4f;
        mRotateThresholdMax = size.y *0.6f;

    }
    View.OnTouchListener touchListener = new View.OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            // pass the events to the gesture detector
            // a return value of true means the detector is handling it
            // a return value of false means the detector didn't
            // recognize the event
            return mDetector.onTouchEvent(event);

        }
    };
    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }

    class MyGestureListener extends GestureDetector.SimpleOnGestureListener {

        @Override
        public boolean onDown(MotionEvent event) {
           // Log.d("TAG","onDown: ");

            // don't return false here or else none of the other
            // gestures will work
            return true;
        }

        @Override
        public boolean onSingleTapConfirmed(MotionEvent e) {
            //Log.i("TAG", "onSingleTapConfirmed: ");
            return true;
        }

        @Override
        public void onLongPress(MotionEvent e) {
//            Log.i("TAG", "onLongPress: ");
            NavmEs3Lib.setAutoRun(0);
        }

        @Override
        public boolean onDoubleTap(MotionEvent e) {
            //Log.i("TAG", "onDoubleTap: ");
            NavmEs3Lib.setAutoRun(1);
            return false;
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2,
                                float distanceX, float distanceY) {
          //  Log.i("TAG", "onScroll: x=" + distanceX + " y= "+ distanceY+ " e2= "+e2.getY() );
            distanceX /= 10;
            distanceY /= 100;
            if (Math.abs(distanceX) > Math.abs(distanceY)) {
                if (distanceX > 1 || distanceX < -1) {
                    if (e2.getY() > mRotateThresholdMax)
                        NavmEs3Lib.rotate(distanceX);
                    else if (e2.getY() < mRotateThresholdMin)
                        NavmEs3Lib.rotate(-distanceX);
                    return false;
                }
            }else {
                if (distanceY > 0.1 || distanceY < -0.1) {
                    NavmEs3Lib.zoom(distanceY);
                    return false;
                }
            }
            return true;
        }

        @Override
        public boolean onFling(MotionEvent event1, MotionEvent event2,
                               float velocityX, float velocityY) {
            //Log.i("TAG", "onFling: velocityX="+velocityX + " velocityY= "+ velocityY);
            int mode = NavmEs3Lib.getMode();
            if (velocityX > 8000) { //to right
                 mode --;
                 NavmEs3Lib.setMode(mode);
            } else if (velocityX < -8000) {
                mode++;
                NavmEs3Lib.setMode(mode);
            }

            return true;
        }
    }
}
