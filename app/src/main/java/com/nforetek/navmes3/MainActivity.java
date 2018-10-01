/*
 * Copyright 2018 nFore Technology Inc.
 *
 */

package com.nforetek.navmes3;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.graphics.Point;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;

public class MainActivity extends Activity {
    private static final String TAG = "MainActivity";
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
/*    TODO runtime permission later
    @Override
    public void onStart() {
        super.onStart();
        if (checkPermissionPass()) {
            startTask();
        }
    }
    //Remember to add these permission in AndroidManifest.xml <uses-permission//
    final String[] PERMISSIONS = {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.CAMERA};
    final int MULTI_PERMISSIONS_ID = 1001;
    private boolean hasPermissions(String... permissions) {
        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && permissions != null) {
            for (String permission : permissions) {
                if (!shouldShowRequestPermissionRationale(permission))
                    return false;
                if (ActivityCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                    if (shouldShowRequestPermissionRationale(permission)) {
                        continue;
                    }
                    return false;
                }
            }
        }
        return true;
    }

    boolean checkPermissionPass(){
        if (hasPermissions(PERMISSIONS))
            return true;
        // if the user has rejected permission, the next time will not show RequestPermission dialog, and directly give
        // PERMISSION_GRANTED. User have to uninstall the APP and give again.
        ActivityCompat.requestPermissions(this,
                PERMISSIONS,
                MULTI_PERMISSIONS_ID);
        return false;
    }
    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult = " + requestCode + " size="+ permissions.length);
        if (requestCode == MULTI_PERMISSIONS_ID)
        {
            int needTotalGranted = PERMISSIONS.length;
            for (int i=0; i< grantResults.length; i++) {
                Log.d(TAG, "onRequestPermissionsResult string = "+permissions[i]+ " grantResults = "+grantResults[i] );
                if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                    for (int j=0; j<PERMISSIONS.length; j++) {
                        if (permissions[i].equals(PERMISSIONS[j]))
                            needTotalGranted--;
                    }
                }
            }
            if (needTotalGranted == 0) {
                startTask();
            } else {
                showDialogOK("This program stop due to not granted necessary permissions. If you want to run again please uninstall the program and run again.",
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                switch (which) {
                                    case DialogInterface.BUTTON_POSITIVE:
                                        //;
                                        break;
                                    case DialogInterface.BUTTON_NEGATIVE:
                                        // proceed with logic by disabling the related features or quit the app.
                                        break;
                                }
                            }
                        });
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }
    private void showDialogOK(String message, DialogInterface.OnClickListener okListener) {
        new AlertDialog.Builder(this)
                .setMessage(message)
                .setPositiveButton("OK", okListener)
                .setNegativeButton("Cancel", okListener)
                .create()
                .show();
    }
    void startTask() {
        //do something
    }
    */
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
