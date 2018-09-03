#pragma once

/*!< Matrix without real allocated data */
class Matlet{
public:
//	Matlet(){};
//	~Matlet(){};
protected:
	double* data;
	int rows; //number of row
	int stride;//bytes per row
	int cols; //number of columns
};

class Mat: Matlet{
public:
 Mat(int cx, int cy);
    ~Mat();
    int Cols();
    int Rows();
    void Set(int x, int y, double value);
    void Set(double* mydata, int length);
    double Get(int x, int y);
    void Identity();
    void Print();
	/*<! T= transpose of THIS */
    void Transpose(Mat& t);
	/*<! THIS= S xT */
    void 	Multiply(Mat& s, Mat& t);
	/*<! T is submatrix of THIS, excluded x column and y row*/
    void GetSubmatrix(int x, int y, Mat& t);
	/*<! return the determinant of THIS matrix */
    double Determinant();
	/*<! Find inverse of THIS and save as T, return FALSE if cannot find*/
    bool FindInverse(Mat& t);
};

