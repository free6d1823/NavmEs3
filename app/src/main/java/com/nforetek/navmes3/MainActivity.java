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
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;

import android.os.Handler;
import android.widget.Toast;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;

public class MainActivity extends Activity {
    private static final String TAG = "MainActivity";
    private GestureDetector mDetector;

    NavmView mView;
    Handler m_handler ;
    int m_mode=0;
    float mRotateThresholdMin = 500; //area to rotate scene
    float mRotateThresholdMax = 500; //area to rotate scene

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

//        mView = new NavmView(getApplication());
//        setContentView(mView);

        mDetector = new GestureDetector(this, new MyGestureListener());


        Point size = new Point();
        getWindowManager().getDefaultDisplay().getSize(size);
        mRotateThresholdMin = size.y *0.4f;
        mRotateThresholdMax = size.y *0.6f;

        //m_handler = new Handler();
        //startRepeatingTask();

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
    @Override
     protected void onDestroy() {
        super.onDestroy();

    }

    boolean mInited = false;
    @Override protected void onPause() {
        super.onPause();
        if (mInited)
            mView.onPause();

    }

    @Override protected void onResume() {
        super.onResume();
        if (mInited)
            mView.onResume();

    }

    /*For SOUND SK82 platform, no need to request CAMERA permission //
    final String[] PERMISSIONS = {Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.CAMERA};
    */
    final String[] PERMISSIONS = {Manifest.permission.READ_EXTERNAL_STORAGE,};
    final int MULTI_PERMISSIONS_ID = 1001;
    @Override
    public void onStart() {
        super.onStart();
        if (hasPermissions(PERMISSIONS)) {
            startTask();
        } else {
        }
    }

    private boolean hasPermissions(String... permissions) {
        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && permissions != null) {
            for (String permission : permissions) {

                if (ActivityCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                    if (shouldShowRequestPermissionRationale(permission)) {
                        ActivityCompat.requestPermissions(this,
                                PERMISSIONS,
                                MULTI_PERMISSIONS_ID);
                    }
                    return false;
                }
            }
        }
        mInited = true;
        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        //first time we got grantResults.length=0
        if (requestCode == MULTI_PERMISSIONS_ID && grantResults.length>0)
        {
            int needTotalGranted = PERMISSIONS.length;
            for (int i=0; i< grantResults.length; i++) {
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
        mView = new NavmView(getApplication());
        setContentView(mView);
        mView.setOnTouchListener(touchListener);
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
            //NavmEs3Lib.setAutoRun(0);


          /*  Calendar cal = Calendar.getInstance();
            DateFormat dateFormat = new SimpleDateFormat("MMddHHmmss");
            String texFile = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath() +
                    "/Cam-Floor"+dateFormat.format(cal.getTime())+"_1280x960.rgb";
            Log.i("TAG", "saveTexture: "+ texFile);

            if (0!= NavmEs3Lib.saveTexture(1)){
                Toast.makeText(getBaseContext(),"Failed to save texture to file " + texFile,
                        Toast.LENGTH_LONG).show();
            } else {
                Toast.makeText(getBaseContext(),"Save texture to file " + texFile + "successfully.",
                        Toast.LENGTH_LONG).show();
            }*/
        }

        int mOption = 0;
        @Override
        public boolean onDoubleTap(MotionEvent e) {
            //Log.i("TAG", "onDoubleTap: ");
            //NavmEs3Lib.setAutoRun(1);
            mOption ++;
            if (mOption > 1) mOption = 0;
            NavmEs3Lib.setOption(mOption);
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
            if (velocityX > 4000) { //to right
                 mode --;
                 if(mode <0)
                     mode = 2;
                 NavmEs3Lib.setMode(mode);
            } else if (velocityX < -4000) {
                mode++;
                if(mode > 2)
                    mode =0;
                NavmEs3Lib.setMode(mode);
            }

            return true;
        }
    }
}
