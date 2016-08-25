// ImgSrc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string.h>
#include <list>
#include <algorithm>
#include <windows.h>
#include <highgui.h>
#include <cxcore.h>
#include <cv.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#pragma pack(4)
struct HEAD
{
	long headlen;
	long taillen;
	long h;
	long w;
};
#pragma pack()

int main(int argc, char* argv[])
{
	printf("this is tcp server for sending bitmap\n");
	if (argc != 2)
	{
		printf("usage: argv[0] x.avi or argv[0] c:\\jpgs\\\n");
		return 1;
	}

	bool isAvi = false;
	if (strcmp(argv[1]+strlen(argv[1]) - 4, ".avi") == 0)
	{
		//
		isAvi = true;
	}
	if (strcmp(argv[1]+strlen(argv[1]) - 4, ".mp4") == 0)
	{
		//
		isAvi = true;
	}

	std::list<char*> jpgNames;
	if (!isAvi)
	{
		char path1[260]={0};
		char path2[260]={0};
		strcpy(path1, argv[1]);
		strcpy(path2, argv[1]);
		if (path1[strlen(path1) -1] != '\\')
		{
			strcat(path1, "\\");
			strcat(path2, "\\");
		}
		strcat(path1, "*.jpg");
		strcat(path2, "*.jpeg");

		WIN32_FIND_DATA findfiledata;
		HANDLE h = NULL;
		h = FindFirstFile(path1, &findfiledata);
		if (INVALID_HANDLE_VALUE == h)
		{
			h = FindFirstFile(path2, &findfiledata);
		}
		if (INVALID_HANDLE_VALUE == h)
		{
			printf("usage: argv[0] x.avi or argv[0] c:\\jpgs\\\n");
			return 2;
		}

		do
		{
			char *p = new char[260];
			strcpy(p, findfiledata.cFileName);
			jpgNames.push_front(p);
		}while(FindNextFile(h, &findfiledata));

		char **names = new char*[jpgNames.size()];
		int n = 0;
		for (std::list<char*>::iterator it = jpgNames.begin(); it != jpgNames.end(); ++it)
		{
			names[n++] = *it;
		}

		for (int i=0; i<n-1; ++i)
		{
			for (int j=0; j<n-i-1; ++j)
			{
				if (strcmp(names[j], names[j+1]) > 0)
				{
					char *p = names[j];
					names[j] = names[j+1];
					names[j+1] = p;
				}
			}
		}
		jpgNames.clear();
		char buf[260] = {0};
		for (i=0; i<n; ++i)
		{
			strcpy(buf, argv[1]);
			if (buf[strlen(buf)-1] != '\\'){
				strcat(buf,"\\");
			}
			strcat(buf, names[i]);
			strcpy(names[i],buf);
			jpgNames.push_back(names[i]);
		}
	}

	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,2), &wsadata);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 1)
	{
		printf("socket error\n");
		return 4;
	}
	sockaddr_in myaddr;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(3000);

	int n = bind(fd, (sockaddr*)&myaddr, sizeof(myaddr));
	if (n != 0)
	{
		printf("bind port error\n");
		return 3;
	}
	listen(fd, n);

	while (true)
	{
		printf("waitting for new client\n");
		int newfd = 0;
		sockaddr faraddr;
		memset(&faraddr, 0, sizeof(faraddr));
		int addrlen = sizeof(faraddr);
		newfd = accept(fd, (sockaddr*)&faraddr, &addrlen);
		if (newfd < 1)
		{
			printf("accept error\n");
			closesocket(fd);
			return 0;
		}
		printf("got new client\n");
	
		if (true)
		{
			CvCapture* capture = NULL;
			std::list<char*>::iterator it;
			if (isAvi)
			{
				capture = cvCaptureFromFile(argv[1]);
				if (NULL == capture)
				{
					printf("capture from avi file error\n");
					return 2;
				}
				
				//Ê×Ö¡¶ªÆú
				cvQueryFrame(capture);
			}
			else
			{
				it = jpgNames.begin();
			}
			while (true)
			{
				IplImage *img = NULL;
				if (isAvi)
				{
					img = cvQueryFrame(capture);
				}
				else
				{
					if (it == jpgNames.end())
					{
						break;
					}
					img = cvLoadImage(*it);
					++it;
				}
				if (NULL == img)
				{
					break;
				}
				HEAD h;
				h.headlen = htonl(sizeof(HEAD));
				h.taillen = htonl(img->widthStep * img->height);
				h.h = htonl(img->height);
				h.w = htonl(img->width);

				char recvbuf[10];
				int e = recv(newfd, recvbuf, 1, 0);
				if (e != 1)
				{
					shutdown(newfd, 2);
					closesocket(newfd);
					break;
				}
				send(newfd, (char*)&h, sizeof(h), 0);
				if (0 == img->origin)
				{
					send(newfd, img->imageData, img->height * img->widthStep, 0);
				}
				else
				{
					for (int i=img->height-1; i>=0; --i)
					{
						send(newfd, img->imageData + i*img->widthStep, img->widthStep, 0);
					}
				}
				if (isAvi)
				{
				}
				else
				{
					cvReleaseImage(&img);
					img = NULL;
				}
			}
			closesocket(newfd);
			newfd = 0;
		}
		else
		{

		}
	}

	return 0;
}

