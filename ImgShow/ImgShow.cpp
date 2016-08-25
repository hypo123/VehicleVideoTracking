// ImgShow.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cv.h>
#include <highgui.h>
#include <cxcore.h>
#include <errno.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "cxcore.lib")
#pragma comment(lib, "cv.lib")
#pragma comment(lib, "ml.lib")
#pragma comment(lib, "cvaux.lib")
#pragma comment(lib, "highgui.lib")

#pragma pack(4)
struct HEAD
{
	long headlen;
	long h;
	long w;
	long id;
	long taillen;
};
#pragma pack()
int main(int argc, char* argv[])
{
	//---------------------服务器端-----------------------
	
	WSADATA wsadata;
	
	//WSA(Windows Sockets Asynchronous，Windows异步套接字)的启动命令
	WSAStartup(MAKEWORD(2,2), &wsadata);

	//创建套接字
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	
	sockaddr_in myaddr;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(3001);

	//绑定到本机地址和端口上
	int e = bind(fd, (sockaddr*)&myaddr, sizeof(myaddr));
	
	if (e < 0)
	{
		printf("bind port 3001 error\n");
		closesocket(fd);
		return 1;
	}
	
	//监听,准备接受客户端请求
	listen(fd, 5);

	printf("recv data for show\n");

	cvWaitKey(1);
	while (true)
	{
		printf("wait for connecting\n");
		sockaddr_in faraddr;
		int size = sizeof(faraddr);
		int newfd = accept(fd, (sockaddr*)&faraddr, &size);
		if (newfd < 1)
		{
			printf("accept error %d\n", GetLastError());
			closesocket(fd);
			return 1;
		}
		printf("connected on\n");

		IplImage* imgs[10] = {0};
		bool windows[10] = {false};
		while (true)
		{
			bool hasError = false;
			HEAD head;
			for (int i=0; i<4; ++i)
			{
				int n = recv(newfd, ((char*)&head)+i, 1, 0);
				if (n != 1)
				{
					hasError = true;
					printf("recv error %d\n", GetLastError());
					break;
				}
			}
			if (hasError)
			{
				break;
			}
			head.headlen = ntohl(head.headlen);
			if (head.headlen > 1024 || head.headlen < 8)
			{
				printf("head.headlen error %d\n", head.headlen);
				break;
			}

			for (int i=0; i<head.headlen - 4; ++i)
			{
				int n = recv(newfd, ((char*)&head)+4+i, 1, 0);
				if (n != 1)
				{
					hasError = true;
					printf("recv error %d\n", GetLastError());
					break;
				}
			}
			if (hasError)
			{
				break;
			}
			head.h = ntohl(head.h);
			head.id = ntohl(head.id);
			head.w = ntohl(head.w);
			head.taillen = ntohl(head.taillen);
			if (head.taillen > 10*1024*1024 && head.taillen < 1)
			{
				printf("head.taillen error %d\n", head.taillen);
				break;
			}

			if (head.id >= sizeof(imgs)/sizeof(imgs[0]) || head.id < 0)
			{
				printf("id error %d\n", head.id);
				break;
			}
			if (head.taillen !=  (((head.w*3+3) & (~3)) * head.h) && head.taillen !=  (((head.w+3) & (~3)) * head.h) )
			{
				printf("w h tail error %d %d %d\n", head.w, head.h, head.taillen);
				break;
			}
				
			IplImage* img = imgs[head.id];
			int nChannels = (head.taillen / head.w / head.h) >= 3 ? 3 : 1;
			bool needCreate = false;
			needCreate = (NULL == img);
			if (!needCreate && (img->height != head.h || img->width != head.w || img->nChannels != nChannels))
			{
				needCreate = true;
				cvReleaseImage(&(imgs[head.id]));
				imgs[head.id] = img = NULL;
			}
			if (needCreate)
			{
				imgs[head.id] = img = cvCreateImage(cvSize(head.w, head.h), IPL_DEPTH_8U, nChannels);
			}

			int receivedLen = 0;
			int needLen = head.taillen;

			while (needLen > 0)
			{
				int n = recv(newfd, img->imageData + receivedLen, needLen, 0);
				if (n < 1)
				{
					printf("recv error line:%d error:%d\n", __LINE__, GetLastError());
					hasError = true;
					break;
				}
				receivedLen += n;
				needLen -= n;
			}
			if (hasError)
			{
				shutdown(newfd, 2);
				closesocket(newfd);
				break;
			}

			char windowName[10] = {0};
			windowName[0] = '0' + head.id;
			if (!windows[head.id])
			{
				windows[head.id] = true;
				cvNamedWindow(windowName);
			}

			cvShowImage(windowName, img);
			cvWaitKey(1);
		}
		shutdown(newfd, 2);
		closesocket(newfd);
		
		for (int i=0; i<sizeof(windows)/sizeof(windows[0]); ++i)
		{
			if (windows[i])
			{
				windows[i] = false;
				char buf[10] = {0};
				buf[0] = i+'0';
				cvDestroyWindow(buf);
			}
		}
	}
	
	return 0;
}

