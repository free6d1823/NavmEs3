#ifndef NAVMES3_COMMON_H_
#define NAVMES3_COMMON_H_
#include <android/log.h>

#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)


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

#endif //NAVMES3_COMMON_H_

