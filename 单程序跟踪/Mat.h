// Mat.h: interface for the Mat class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAT_H__8AD4AD6C_CF23_4512_B8E3_2D9F0117058E__INCLUDED_)
#define AFX_MAT_H__8AD4AD6C_CF23_4512_B8E3_2D9F0117058E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class Mat  
{
public:
	Mat(int row, int col);
	virtual ~Mat();

	void operator+=(const Mat& m);
	void operator-=(const Mat& m);
	void operator*=(const Mat& m);
	void operator=(const Mat& m);
	void inverse();
	void t();
	double* operator[](int n);
	void print();
private:
	int m_nRow;
	int m_nCol;
	double m_data[4][4];
};

#endif // !defined(AFX_MAT_H__8AD4AD6C_CF23_4512_B8E3_2D9F0117058E__INCLUDED_)
