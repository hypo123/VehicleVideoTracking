// Kalman.h: interface for the Kalman class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KALMAN_H__D89CBEAC_92E1_47E1_9FF6_4B5BC038155F__INCLUDED_)
#define AFX_KALMAN_H__D89CBEAC_92E1_47E1_9FF6_4B5BC038155F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Mat.h"

class Kalman  
{
public:
	Kalman();
	~Kalman();

	double getCurrent();
	double getPredict();

	void goNext(double newData);
	void goNext();
private:
	Kalman& operator=(const Kalman& k);

	long frameCount;
	double newData;

	//速度位移数据
	Mat X;

	//位移变换及其转置
	Mat A;
	Mat At;

	Mat Z;

	Mat H;
	Mat Ht;

	Mat Q;
	Mat R;
	Mat P;

	Mat Kg;

	void init();

	//对应5个公式
	void step1();
	void step2();
	void step3();
	void step4();
	void step5();
};

#endif // !defined(AFX_KALMAN_H__D89CBEAC_92E1_47E1_9FF6_4B5BC038155F__INCLUDED_)
