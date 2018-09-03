#include <stdio.h>
#include "Mat.h"
#include <assert.h>

Mat::Mat(int cx, int cy)
{
		data = new double[cx*cy];
		rows = cy;
		cols = cx;
		stride = cx;
}

Mat::~Mat(void)
{
	delete [] data;
}
int Mat::Cols(){return cols;}
int Mat::Rows(){return rows;}
	void Mat::Set(int x, int y, double value){
		data[y*stride+ x] = value;}
void Mat::Set(double* mydata, int length){
		int k=0;
		for(int i=0;i<rows;i++){
			for(int j=0;j<cols;j++) {
				Set(j,i,mydata[k]);
				k++;
				if (k>= length)
					break;
			}
		}
	}
double Mat::Get(int x, int y) {return data[y*stride+ x];}
void Mat::Identity() {
		for( int i=0; i<rows; i++) {
			for(int j=0; j< cols; j++)
				Set(i,j, (i==j)?1.0:0);
		}
}
void Mat::Print(){
    printf("{\n");
	for(int i=0;i<Rows();i++) {
		for (int j=0;j<Cols();j++) {
            printf("\t%8f", Get(j,i) );
		}
    printf("\n");
	}
    printf("}\n;");
}
void Mat::Transpose(Mat& t) {
		t.rows = cols;
		t.cols = rows;
		for( int i=0; i<rows; i++) {
			for(int j=0; j< cols; j++)
				t.Set(i,j, Get(j,i));
		}
}
void 	Mat::Multiply(Mat& s, Mat& t)
{
	assert( cols  == t.cols);
	assert(rows== s.rows);

	for( int i=0; i<rows; i++) {
		for(int j=0; j< cols; j++){
			double v = 0;
			for (int k=0;k<s.cols; k++) {
				v+= s.Get(k,i) * t.Get(j,k);
			}
			Set(j,i, v);
		}
	}
}
bool Mat::FindInverse(Mat& t)
{
	//step1 find minor matrix
	double det = Determinant();
	double cofactor = 1.0;
	if (det <0.0001 && det >0.0001)
        return false;
	Mat s(cols-1, rows-1);	//sub
	for(int i =0; i<rows; i++) {
		for (int j=0;j<cols; j++) {
			GetSubmatrix(j,i,s);
			cofactor = ((i+j)%2 == 0)?1.0:-1.0;
			t.Set(i,j, s.Determinant()*cofactor/det);
		}
	}

	return true;
}
void Mat::GetSubmatrix(int x, int y, Mat& t)
{
		assert(t.cols == cols -1);
		assert(t.rows == rows -1);
		int m,n;
		n=0;
		for(int i=0;i<rows;i++) {
			if (i == y) continue;
			m=0;
			for (int j=0;j< cols;j++) {
				if(j== x) continue;
				
				t.Set(m,n, Get(j,i));
				m++;
			}
			n++;
		}
}
double Mat::Determinant()
{
	if (cols == 2)
		return Get(0,0)*Get(1,1) - Get(0,1)*Get(1,0);
	double sign = 1.0;
	double v = 0;
	for(int j=0; j<cols ;j++){
		Mat t(cols-1, rows-1);
		GetSubmatrix( j,0, t);
		v += sign*t.Determinant()*Get(j,0);
		sign *= -1.0;
	}
	return v;

}
