// 单程序均值背景差.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Picture.h"
#include <highgui.h>
#include <cxcore.h>
#include <cv.h>

void Rgb2Gray(IplImage* gray, const IplImage* rgb);
void GrayDif(IplImage* pDif, const IplImage* pImg1, const IplImage* pImg2);
void Multi(IplImage* pDif, unsigned int n);
void CaclBackground(IplImage* pBackground, const IplImage* pGrayFrame);
void Gray2Bin(IplImage* pBin, const IplImage* pGray);

int main(int argc, char* argv[])
{
	Picture * dif5 = NULL;//帧差5倍放大图
	Picture * difBin = NULL;//帧差二值化图

	if (argc!=2)
	{
		printf("need argument");
		return 1;
	}

	CvCapture* capture = cvCaptureFromFile(argv[1]);
	if (capture==NULL)
	{
		printf("open file error, %s", argv[1]);
		return 2;
	}
	IplImage *pRgb = cvQueryFrame(capture);

	//初始化各图像尺寸
	IplImage *pGray = cvCreateImage(cvSize(pRgb->width, pRgb->height), IPL_DEPTH_8U, 1);//灰度图
	IplImage *pDif = cvCreateImage(cvSize(pRgb->width, pRgb->height), IPL_DEPTH_8U, 1);//背景差
	IplImage *pDif5 = cvCreateImage(cvSize(pRgb->width, pRgb->height), IPL_DEPTH_8U, 1);//背景差5倍
	IplImage *pDifBin = cvCreateImage(cvSize(pRgb->width, pRgb->height), IPL_DEPTH_8U, 1);//帧差二值化
	IplImage *pBackground = cvCreateImage(cvSize(pRgb->width, pRgb->height), IPL_DEPTH_8U, 1);//均值背景图
	pGray->origin = 
	pBackground->origin = 
	pDif->origin = 
	pDif5->origin = 
	pDifBin->origin = pRgb->origin;

	//创建显示窗口
	cvNamedWindow("rgb");
	cvNamedWindow("gray");
	cvNamedWindow("background");
	cvNamedWindow("dif");
	cvNamedWindow("dif5");
	cvNamedWindow("bin");

	//循环读帧处理
	while ((pRgb = cvQueryFrame(capture)) != NULL)
	{
		//cvShowImage("rgb",pRgb);

		//灰度
		Rgb2Gray(pGray, pRgb);
		cvShowImage("gray", pGray);

		//背景
		cvShowImage("background", pBackground);

		//背景差
		GrayDif(pDif, pGray, pBackground);
		cvShowImage("dif", pDif);

		//帧差5倍
		memcpy(pDif5->imageData, pDif->imageData, pGray->widthStep * pGray->height);
		::Multi(pDif5, 5);
		cvShowImage("dif5", pDif5);

		//阈值分隔
		Gray2Bin(pDifBin, pDif);
		cvShowImage("bin", pDifBin);

		//更新背景
		CaclBackground(pBackground, pGray);

		cvWaitKey(100);
	}

	return 0;
}


//RGB图像转灰度图
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

//将灰度图的像素点的值放大n倍
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

void CaclBackground(IplImage* pBackground, const IplImage* pGrayFrame)
{
	Picture* picBg = new Picture(pBackground->width, pBackground->height, 1);
	Picture* picGray= new Picture(pGrayFrame->width, pGrayFrame->height, 1);
	memcpy((*picBg)[0], pBackground->imageData, pBackground->widthStep*pBackground->height);
	memcpy((*picGray)[0], pGrayFrame->imageData, pGrayFrame->widthStep*pGrayFrame->height);
	for (int i=0; i<pBackground->height; ++i)
	{
		for (int j=0; j<pBackground->width; ++j)
		{
			(*picBg)[i][j] = ((*picBg)[i][j] * 95 + (*picGray)[i][j] * 5 + 50) / 100;
		}
	}
	memcpy(pBackground->imageData, (*picBg)[0], pGrayFrame->widthStep*pGrayFrame->height);
	delete picGray;
	picGray = NULL;
	delete picBg;
	picBg = NULL;
}

void Gray2Bin(IplImage* pBin, const IplImage* pGray)
{
	Picture* pPicBin = new Picture(pGray->width, pGray->height, 1);
	Picture* picGray= new Picture(pGray->width, pGray->height, 1);
	memcpy((*picGray)[0], pGray->imageData, pGray->widthStep*pGray->height);
	for (int i=0; i<pGray->height; ++i)
	{
		for (int j=0; j<pGray->width; ++j)
		{
			(*pPicBin)[i][j] = (*picGray)[i][j] > 30 ? 255 : 0;
		}
	}
	memcpy(pBin->imageData, (*pPicBin)[0], pBin->widthStep * pBin->height);
	delete picGray;
	picGray = NULL;
	delete pPicBin;
	pPicBin = NULL;
}

