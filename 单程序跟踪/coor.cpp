#include <stdio.h>
#include "coor.h"

//边缘框,形成一个坐标系
//（公路平行线汇聚上方远处一发射点，距离0行的像素垂直高度v，映射到左下点为水平h零点，右方为正）
const float leftupR = 1;
const float leftupC = 56;
const float rightupR = 1;
const float rightupC = 149;
const float leftdownR = 180;
const float leftdownC = 27;
const float rightdownR = 180;
const float rightdownC = 240;
float coor_ov = 0;
float coor_oh = 0;
static void coor_calcVH()
{
	//是否已经计算
	if (coor_ov < -1 || coor_ov > 1)
		return;

	//直线方程Ax+By+C=0

	#define x1 (leftupC)
	#define y1 (leftupR)
	#define x2 (leftdownC)
	#define y2 (leftdownR)

	float A1 = y2-y1;
	float B1 = x1-x2;
	float C1 = y1*x2 -  x1*y2;

	#undef x1
	#undef y1
	#undef x2
	#undef y2

	#define x1 (rightupC)
	#define y1 (rightupR)
	#define x2 (rightdownC)
	#define y2 (rightdownR)

	float A2 = y2-y1;
	float B2 = x1-x2;
	float C2 = y1*x2 -  x1*y2;

	#undef x1
	#undef y1
	#undef x2
	#undef y2

	//两直线相交
	coor_ov = (A1*C2 - A2*C1) / (A2*B1 - A1*B2);
	coor_oh = -(B1*coor_ov + C1) / A1;
}

float coor_rc2v(float row, float col)
{
	coor_calcVH();
	return row;
}
float coor_rc2h(float row, float col)
{
	coor_calcVH();
	return (leftdownR - coor_ov) / (coor_ov - row) * (coor_oh - col) + coor_oh - leftdownC;
}
float coor_vh2r(float v, float h)
{ 
	coor_calcVH();
	return v;
}
float coor_vh2c(float v, float h)
{
	coor_calcVH();
	return (v-coor_ov) / (coor_ov - leftdownR) * (coor_oh - (leftdownC + h)) + coor_oh;
}


//车道线
/*
双向道路中间 1车道左(180,26) 
1车道右 (180,135)
2车道右 (180,240)
3车道右 (150,318)
1车道为0~1，
2车道1~2,
3车道2~3,
*/

float calcRoadNum(short row, short col)
{
	float f0 = coor_rc2h(180, 26);
	float f1 = coor_rc2h(180, 135);
	float f2 = coor_rc2h(180, 240);
	float f3 = coor_rc2h(150, 318);

	float h = coor_rc2h(row, col);
	if (h < f0)
		return (f0-h)/(f1-f0);
	else if (h < f1)
		return (h-f0)/(f1-f0) + 0;
	else if (h < f2)
		return (h-f1)/(f2-f1) + 1;
	else if (h < f3)
		return (h-f2)/(f3-f2) + 2;
	else
		return (h-f3)/(f3-f2) + 3;
}

//路面坐标与视频图像的映射公式问题
//const short y[13] = {122,105,90,77,65,55,46,37,29,23,15,10,4};
static void f()
{
	//上方点坐标(0,83.5 - 4), (300, 0) 摄像机上下视角约30度
	float x0 = 0,y0 = 83.5f-4,x1 =300,y1 =0;
	float A1 = y1-y0;
	float B1 = x0-x1;
	float C1 = (y0-y1)*x0+y0*(x1-x0);
	y0 = 83.5f-122;
	float A2 = y1-y0;
	float B2 = x0-x1;
	float C2 = (y0-y1)*x0+y0*(x1-x0);

	//y = k(x+1000); -2 < k < -y0/x1
	for (float k=-(83.5f-4)/x1-0.01f; k>-2; k-=0.01f)
	{
		float A3 = k;
		float B3 = -1;
		float C3 = 1000*k;

		float yup = (A1*C3-A3*C1)/(A3*B1-A1*B3);
		float ydown = (A2*C3-A3*C2)/(A3*B2-A2*B3);
		printf("%f, %f, %f, %f, %f\n", yup, ydown, yup/ydown, 9.5f/3.5f, k);
	}

	
	k=-1.515f;
	k=-0.7f;

	float y[13] = {122,105,90,77,65,55,46,37,29,23,15,10,4};
	float last = 0;
	for (int i=0; i<13; ++i)
	{
		y0 = y[i] = 83.5f - y[i];

		A1 = y1-y0;
		B1 = x0-x1;
		C1 = (y0-y1)*x0+y0*(x1-x0);


		float A3 = k;
		float B3 = -1;
		float C3 = 1000*k;
		float yup = (A1*C3-A3*C1)/(A3*B1-A1*B3);

		if (i > 0)
			printf("%f\n", yup - last);
		last = yup;
	}
}

//行转路面
float r2m(float r)
{
	const float x0 = 0;
	float y0 = 83.5f-r;
	const float x1 =300;
	const float y1 =0;
	
	float A1 = y1-y0;
	float B1 = x0-x1;
	float C1 = (y0-y1)*x0+y0*(x1-x0);

	const float k = -0.7f;
	const float A3 = k;
	const float B3 = -1;
	const float C3 = 1000*k;

	float y = (A1*C3-A3*C1)/(A3*B1-A1*B3);
	y = 600.671936f - y;
	y /= 50;//缩放比例
	return y;
}
//路面转行
float m2r(float m)
{
	m *= 50;
	float y = 600.671936f - m;

	const float k = -0.7f;
	const float A3 = k;
	const float B3 = -1;
	const float C3 = 1000*k;
	float x = (B3*y+C3)/(-A3);

	float f = (-x/(300-x));
	f *= (0-y);
	f += y;

	return 83.5f - f;
}