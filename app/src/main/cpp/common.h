#ifndef NAVMES3_COMMON_H_
#define NAVMES3_COMMON_H_
#ifdef ANDROID
#include <android/log.h>
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else
#include <stdio.h>
#define  LOGI(...)  fprintf(stdout, __VA_ARGS__)
#define  LOGE(...)  fprintf(stderr, __VA_ARGS__)

#endif

#define MAX_CAMERAS 4   /*<! numbers of cameras, or number of separated area, in AVM */
#define MAX_FP_AREA 16   /*<! number of feature points region in a camera area */
/*!<numbersof total FP in a camera area*/
#define FP_COUNTS			30


/*<! basic data type re-definition */
#ifndef _NFORE_DATA_TYPE___
#define _NFORE_DATA_TYPE___

typedef unsigned char nfByte, *nfPByte;

typedef struct _nfFloat2D{
    float x;
    float y;
}nfFloat2D;

typedef struct _nfFloat3D{
    float x;
    float y;
    float z;
}nfFloat3D;

typedef struct _nfRectF {
    float l;
    float t;
    float r;
    float b;
}nfRectF;
#endif //_NFORE_DATA_TYPE___
/*!<Fisheye Correction parametters*/
typedef struct _FecParam{
    nfFloat2D ptCenter;		/*!<symmetry center of image relative to top-left of rcInput, in pixels*/
    float	pitch;			/*!<pitch angle in radiun */
    float	yaw;			/*!<rotate with respect to y-axis */
    float	roll;			/*!<rotate with respect to z-axis */
    float	fov;			/*!<hoizontal FOV factor, 0 to PI*/
    float	k1;             /*!<radical coefficient 1 */
    float	k2;             /*!<radical coefficient 2 */
    float a;				/*!<intrinsic arameters, x- scale factor */
    float b;				/*!<intrinsic arameters, y- scale factor */
    float c; 				/*!<intrinsic arameters, skew  */
}FecParam;
/*!<Homograph Transformation parametters*/
typedef struct _HomoParam{
    int fp_index[4];    /*!<feature points indices of FP array, in the homo region*/
    float h[3][3];     /*!<Homography Matrix coef. of the region */
}HomoParam;

typedef struct _AreaSettings {
    nfRectF  range;                  /*!<the coordinates on final normalized image, 1.0x1.0 of this area  */
    FecParam fec;                   /*!<FEC applied to all area */
    int nFpCounts;                  /*!<numbers of feature points in this camera, le. than  FP_COUNTS*/
    nfFloat2D fpt[FP_COUNTS];         /*!<feature points at final image*/
    nfFloat2D fps[FP_COUNTS];         /*!<feature points at rectified image*/
    nfFloat2D fpf[FP_COUNTS];         /*!<feature points at fisheye image*/
    int nFpAreaCounts;                /*!<numbers of FP areas in this camera, l.e. than MAX_FP_AREA*/
    nfRectF	region[MAX_FP_AREA];      /*!<the normalized coordinates of homo_region on final image*/
    HomoParam homo[MAX_FP_AREA];    /*!<homo apply to selected region */
}AreaSettings;

bool    LoadFecParam(FecParam* pFecParam, int nArea);
bool    LoadAreaSettings(AreaSettings* pParam, int nArea);
bool    LoadHomoParam(HomoParam* pParam, int nArea, int nFp);
bool    SaveFecParam(FecParam* pFecParam, int nArea);
bool    SaveHomoParam(HomoParam* pParam, int nArea, int nFp);
bool    SaveAreaSettings(AreaSettings* pParam, int nArea);

#endif //NAVMES3_COMMON_H_

