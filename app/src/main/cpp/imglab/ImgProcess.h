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
///
/// \brief ref create a nfImage object associated with existed data. It doesn't own the data,
/// so the bufer is not destroed when nfImage is destroyed.
/// \param data
/// \param w
/// \param h
/// \param bpp
/// \return the nfImage object
///
static nfImage* ref(unsigned char* data, unsigned int w, unsigned int h, unsigned int stride);

////
/// \brief clone create a nfImage object but share the same buffer as source. The new object owns the buffer, so the source should use detach() to release the buffer
/// \param source
/// \return pointer to the clone nfImage
///
static nfImage* clone(nfImage* pSource);

static void destroy(nfImage** ppImage);
///
/// \brief dettach the buffer from a nfImage object and destroy this object
/// \param ppImage object to be destroyed
/// \return the image buffer
///
static nfPByte dettach(nfImage** ppImage);

    unsigned char* buffer;   /*<! image data */
    unsigned int width;  /*<! image width in pixel */
    unsigned int stride; /*<! bytes per scan line */
    unsigned int height; /*<! image height in lines */
    bool bRef; /*!<true: buffer is referenced, don't free */
};

class nfBuffer{
public:
	nfBuffer(){};
	nfBuffer(unsigned int elements);
	virtual ~nfBuffer();
    void set(unsigned int /* offset */, char /*value*/){};
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
void nfDoFec(float u, float v, float &x, float &y, FecParam* pFec);
void nfInvFec(float x, float y, float &u, float &v, FecParam* pFec);
void nfFindHomoMatrix(nfFloat2D s[4], nfFloat2D t[4], float hcoef[3][3]);
bool nfDoHomoTransform(float s, float t, float &u, float &v, float h[3][3]);
void nfCalculateHomoMatrix4(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo);
void nfCalculateHomoMatrix12(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo);
void nfCalculateHomoMatrix16(nfFloat2D* fps, nfFloat2D* fpt, HomoParam* homo);


class TexProcess
{
public:
	TexProcess();
	~TexProcess();
	bool update();
	int createVertices(vector<nfFloat3D> & vert, vector<unsigned short>& indices);
	/* find UV maps from vertex vector vert*/
	int updateUv(vector<nfFloat3D> vert, vector <nfFloat2D> &uv);
    int updateUvNoFisheye(vector <nfFloat2D> &uv);
    int reloadIndices(vector<unsigned short>& indices);
    /*<! ini process functions */
    ///
    /// \brief loadIniFile load settings from ini file, used for calibration
    /// \param filename
    /// \return
    ///
    bool loadIniFile(const char* filename);
    bool saveIniFile(const char* filename);
    void normalizeFpf(int nAreaId);
    void calculateFps(int nAreaId);
    void calculateHomo(int nAreaId);
    int getDataState(int nAreaId){return mAreaSettings[nAreaId].state;}
    ///
    /// \brief loadIniFile2 used for deployment. load calculated fps and home cooefficents
    /// \param filename
    /// \return
    ///
    bool loadIniFile2(const char* filename);

    nfImage* getSourceImage();
public:
	AreaSettings mAreaSettings[MAX_CAMERAS];
    char* mpSourceImageName;

private:
    /*<! returns numbers of vertex created in this region*/
	int initVertices(vector<nfFloat3D> & vert, nfRectF region);
	int initVertices_type1(vector<nfFloat3D> & vert, nfRectF region);
	int initVertices_type2(vector<nfFloat3D> & vert, nfRectF region);
	int initVertices_type3(vector<nfFloat3D> & vert, nfRectF region);
	int initVertices_type4(vector<nfFloat3D> & vert, nfRectF region);

	void updateIndices(vector<unsigned short>& indices, int nCam, int nRegion);
	void updateIndices_type1(vector<unsigned short>& indices, int nCam, int nRegion);
	void updateIndices_type2(vector<unsigned short>& indices, int nCam, int nRegion);
	void updateIndices_type3(vector<unsigned short>& indices, int nCam, int nRegion);
	void updateIndices_type4(vector<unsigned short>& indices, int nCam, int nRegion);

	static   nfFloat2D s_offsetCam[MAX_CAMERAS];
	///
	/// \brief m_RegionMap indicates the region should be shown or hiden
	///        0 for hidden
	int m_RegionMap[MAX_CAMERAS][MAX_FP_AREA];
};

#endif // ImgProcess_H
