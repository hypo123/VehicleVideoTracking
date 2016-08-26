// 单程序帧差.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Picture.h"
#include <highgui.h>
#include <cxcore.h>
#include <cv.h>

void Rgb2Gray(IplImage* gray, const IplImage* rgb);
void GrayDif(IplImage* pDif, const IplImage* pImg1, const IplImage* pImg2);
void Multi(IplImage* pDif, unsigned int n);

int main(int argc, char* argv[])
{
	Picture * dif5 = NULL;//帧差5倍放大图
	Picture * difBin = NULL;//帧差二值化图

	if (argc!=2) return 0;

	const char* fileName = argv[1];
	
	CvCapture* capture = cvCaptureFromFile(fileName);
	IplImage *pRgb = cvQueryFrame(capture);

	//初始化各图像尺寸
	//cvCreateImage函数 参数1-结构体cvSize表示图像宽和高 参数2-图像元素的位深度 参数3-每个元素(像素)通道数
	IplImage *pGray = cvCreateImage(cvSize(pRgb->width, pRgb->height), IPL_DEPTH_8U, 1);//灰度图
	IplImage *pLast = cvCreateImage(cvSize(pRgb->width, pRgb->height), IPL_DEPTH_8U, 1);//上一帧灰度图
	IplImage *pDif = cvCreateImage(cvSize(pRgb->width, pRgb->height), IPL_DEPTH_8U, 1);//帧差
	IplImage *pDif5 = cvCreateImage(cvSize(pRgb->width, pRgb->height), IPL_DEPTH_8U, 1);//帧差5倍
	IplImage *pDifBin = cvCreateImage(cvSize(pRgb->width, pRgb->height), IPL_DEPTH_8U, 1);//帧差二值化
	
	//int origin为IplImage结构的变量表示图像原点位置
	pGray->origin = 
	pLast->origin = 
	pDif->origin = 
	pDif5->origin = 
	pDifBin->origin = pRgb->origin;

	//创建显示窗口
	cvNamedWindow("rgb");
	cvNamedWindow("gray");
	cvNamedWindow("last");
	cvNamedWindow("dif");
	cvNamedWindow("dif5");

	//循环读帧处理
	while ((pRgb = cvQueryFrame(capture)) != NULL)
	{
		cvShowImage("rgb",pRgb);

		//灰度
		Rgb2Gray(pGray, pRgb);
		cvShowImage("gray", pGray);

		//上一帧
		cvShowImage("last", pLast);

		//帧差
		GrayDif(pDif, pGray, pLast);
		cvShowImage("dif", pDif);

		//帧差5倍
		memcpy(pDif5->imageData, pDif->imageData, pGray->widthStep * pGray->height);
		::Multi(pDif5, 5);
		cvShowImage("dif5", pDif5);

		//设置上一帧
		memcpy(pLast->imageData, pGray->imageData, pGray->widthStep * pGray->height);
		cvWaitKey(100);
	}

	return 0;
}

//RGB图转灰度图
void Rgb2Gray(IplImage* gray, const IplImage* rgb)
{
	Picture* picRgb = new Picture(rgb->width, rgb->height, 3);
	Picture* picGray= new Picture(rgb->width, rgb->height, 1);
	memcpy((*picRgb)[0], rgb->imageData, rgb->widthStep*rgb->height);
	for (int i=0; i<rgb->height; ++i)
	{
		for (int j=0; j<rgb->width; ++j)
		{
			(*picGray)[i][j] = ((*picRgb)[i][j*3+0] + (*picRgb)[i][j*3+1] * 6 + (*picRgb)[i][j*3+2] * 3 + 5)/10;
		}
	}
	memcpy(gray->imageData, (*picGray)[0], gray->widthStep*gray->height);
	delete picGray;
	picGray = NULL;
	delete picRgb;
	picRgb = NULL;
}

//两幅灰度图的差
void GrayDif(IplImage* pDif, const IplImage* pImg1, const IplImage* pImg2)
{
	Picture* picGray1= new Picture(pImg1->width, pImg1->height, 1);
	Picture* picGray2= new Picture(pImg1->width, pImg1->height, 1);
	Picture* picDif= new Picture(pImg1->width, pImg1->height, 1);
	memcpy((*picGray1)[0], pImg1->imageData, pImg1->widthStep * pImg1->height);
	memcpy((*picGray2)[0], pImg2->imageData, pImg1->widthStep * pImg1->height);
	for (int i=0; i<pImg1->height; ++i)
	{
		for (int j=0; j<pImg1->width; ++j)
		{
			(*picDif)[i][j] = (*picGray1)[i][j] > (*picGray2)[i][j] ? ((*picGray1)[i][j] - (*picGray2)[i][j]) : ((*picGray2)[i][j] - (*picGray1)[i][j]);
		}
	}
	memcpy(pDif->imageData, (*picDif)[0], pImg1->widthStep*pImg1->height);
	delete picDif;
	picDif = NULL;
	delete picGray2;
	picGray2 = NULL;
	delete picGray1;
	picGray1 = NULL;
}

//放大
void Multi(IplImage* pDif, unsigned int n)
{
	int count = pDif->widthStep * pDif->height;
	for (int i=0; i<count; ++i)
	{
		int v = ((unsigned char*)(pDif->imageData))[i];
		v = v * n > 255 ? 255 : v * 5;
		((unsigned char*)(pDif->imageData))[i] = v;
	}
}

