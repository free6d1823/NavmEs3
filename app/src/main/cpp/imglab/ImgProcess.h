#ifndef ImgProcess_H
#define ImgProcess_H

#include <cstdlib>
#include <vector>
#include "../common.h"

#if 0
#define IMAGE_PATH  ":/camera1800x1440.yuv"
#define IMAGE_WIDTH 1800
#define IMAGE_HEIGHT    1440
#define IMAGE_AREA_WIDTH    900
#define IMAGE_AREA_HEIGHT   720
#endif
#define IMAGE_PATH  "camera1440x960.yuv"
#define IMAGE_WIDTH 1440
#define IMAGE_HEIGHT    960
#define IMAGE_AREA_WIDTH    720
#define IMAGE_AREA_HEIGHT   480



using namespace std;

#define SAFE_FREE(p) if(p) { free(p); p=NULL;}
#define SAFE_ALLOC(p, n) { if(p) free(p); p=(typeof(p)) malloc(n);}

class nfImage {
public:
static nfImage* create(unsigned int w, unsigned int h, unsigned int bpp);
static nfImage* ref(unsigned char* data, unsigned int w, unsigned int h, unsigned int bpp);
static void destroy(nfImage** ppImage);
    unsigned char* buffer;   /*<! image data */
    unsigned int width;  /*<! image width in pixel */
    unsigned int stride; /*<! bytes per scan line */
    unsigned int height; /*<! image height in lines */
    bool bRef; /*!<true: buffer is referenced, don't free */
static    int seq;

};

class nfBuffer{
public:
	nfBuffer(){};
	nfBuffer(unsigned int elements);
	virtual ~nfBuffer();
	void set(unsigned int offset, char value){};
	void* data(){return NULL;}

	virtual	unsigned int length(){return mLength;}
	virtual	unsigned int size(){ return mCounts;}
static nfBuffer* create(unsigned int counts);
static void destroy(nfBuffer** pBuffer);
private:

protected:
    unsigned int mBpu; /*<! bytes of per data unit */
    unsigned int mCounts; /*<! counts of data unit */
    unsigned int mLength;  /*<! length in bytes of the data */
};

class nfFloatBuffer : public nfBuffer {
public:
	nfFloatBuffer(unsigned int elements);
    virtual ~nfFloatBuffer();
	float* data(){ return mpData;}
	void set(unsigned int offset, float value){ mpData[offset] = value;};
static nfFloatBuffer* create(unsigned int counts);
static void destroy(nfFloatBuffer** ppBuffer);
private:
    float* mpData;
};

class nfUshortBuffer : public nfBuffer {
public:
	nfUshortBuffer(unsigned int elements);
    virtual ~nfUshortBuffer();
	void set(unsigned int offset, unsigned short value){ mpData[offset] = value;};
	unsigned short* data(){ return mpData;}
static nfUshortBuffer* create(unsigned int counts);
static void destroy(nfUshortBuffer** ppBuffer);
private:
    unsigned short* mpData;
};

void nfYuyvToRgb32(nfImage* pYuv, unsigned char* pRgb, bool uFirst, bool bMirror);
//void doFec(double u, double v, double &x, double &y, FecParam* m_pFec);
//void calculateHomoMatrix(dbPOINT* fps, dbPOINT* fpt, HomoParam* homo);
//bool doHomoTransform(double s, double t, double &u, double &v, double h[3][3]);


#if 0
IMAGE* loadImage();
IMAGE* loadImageArea(int idArea, FecParam* pFec);
void ApplyFec(unsigned char* pSrc, int width, int inStride,  int height, unsigned char* pTar, int outStride, FecParam* pFec);

    /*!<S= HT, S=source, T=target on stitched view */
static void findHomoMatreix(dbPOINT s[4], dbPOINT t[4], double hcoef[3][3]);


#endif
class TexProcess
{
public:
	TexProcess();
	~TexProcess();
	bool update();
	int createVertices(vector<nfFloat3D> & vert, vector<unsigned short>& indices);
	int updateUv(vector <nfFloat2D> &uv);
    int updateUvNoFisheye(vector <nfFloat2D> &uv);

        int reloadIndices(vector<unsigned short>& indices);
public:
	void LoadAllAreaSettings();
	AreaSettings mAreaSettings[MAX_CAMERAS];

private:
    bool mIsSettingsUpdated;
	void initVertices(vector<nfFloat3D> & vert, nfRectF region);
	void updateIndices(vector<unsigned short>& indices, int nCam, int nRegion);
	static   nfFloat2D s_offsetCam[MAX_CAMERAS];
	///
	/// \brief m_RegionMap indicates the region should be shown or hiden
	///        0 for hidden
	int m_RegionMap[MAX_CAMERAS][MAX_FP_AREA];
};

#endif // ImgProcess_H
