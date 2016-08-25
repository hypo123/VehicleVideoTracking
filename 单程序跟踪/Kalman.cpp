// Kalman.cpp: implementation of the Kalman class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "Kalman.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//帧间时间
const double t = 1.0 / 15;

Kalman::Kalman() : X(Mat(2,1)), A(Mat(2,2)), At(Mat(2,2)), Z(Mat(1,1)), 
				H(Mat(1,2)), Ht(Mat(2,1)), Q(Mat(2,2)), R(Mat(1,1)), 
				P(Mat(2,2)), Kg(Mat(2,1))

{
	frameCount = 0;
}

Kalman::~Kalman()
{
	
}

double Kalman::getCurrent()
{
	return X[0][0];	
}



void Kalman::goNext(double newData)
{
	this->newData = newData;
	if (frameCount < 1)
		init();
	else
	{
		step1();
		step2();
		step4();
		step3();
		step5();
	}
	++frameCount;
}


void Kalman::goNext()
{
	if (frameCount > 0)
	{
		step1();
		step2();
		step5();
		++frameCount;
	}
}



void Kalman::init()
{
	frameCount = 1;

	// Mat中未设置值的，认为构造时设为了0
	X[0][0] = newData;

	//匀变速运动
	A[0][0] = 1;
	A[0][1] = t;
	A[1][1] = 1;

	At = A;
	At.t();

	//Z[0][0] = 0;

	H[0][0] = 1;
	Ht = H;
	Ht.t();

	Q[0][0] = 4;
	Q[1][1] = 4;
	R[0][0] = 100;
	P[0][0] = 100;
	P[1][1] = 100;
}

//X(k|k-1)=A X(k-1|k-1)+B U(k) ……….. (1)
void Kalman::step1()
{
	Mat m = A;
	m *= X;
	X = m;
}

//P(k|k-1)=AP(k-1}k-1)At+Q
void Kalman::step2()
{
	Mat m = A;
	m*=P;
	m*=At;
	m+=Q;
	P=m;
}

//X(k|k) = X(k|k-1) + Kg(k)(Z(k)-HX(k|k-1))    (3)
void Kalman::step3()
{
	Mat m2 = Kg;
	Z[0][0] = newData;
	Mat m3 = Z;
	Mat m4 = H;
	m4*=X;
	m3-=m4;
	m2*=m3;
	X += m2;
}

//Kg(k)= P(k|k-1) H’ / (H P(k|k-1) H’ + R) ……… (4)
void Kalman::step4()
{
	Mat up=P;
	up *= Ht;
	Mat down = H;
	down *= P;
	down *= Ht;
	down += R;
	
	down.inverse();
	up *= down;
	
	Kg = up;
}

//P(k|k)=（I-Kg(k) H）P(k|k-1) ……… (5)
void Kalman::step5()
{
	Mat m(2,2);
	m[0][0] = m[1][1] = m[2][2] = 1;
	Mat m2=Kg;
	m2*=H;
	m-=m2;
	m*=P;
	P=m;
}


Kalman& Kalman::operator=(const Kalman& k)
{
	memcpy(this, &k, sizeof(Kalman));
	return *this;
}

double Kalman::getPredict()
{
	Kalman k;
	k = *this;
	k.goNext();
	return k.getCurrent();
}
