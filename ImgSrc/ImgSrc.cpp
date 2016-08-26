// ImgSrc.cpp : Defines the entry point for the console application.
//
//Windows下Socket编程参见:http://www.360doc.com/content/14/0318/15/15257968_361585987.shtml

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

//结构体内变量4字节对齐
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

	//---------------既是服务器端又是客户端--------------
	
	WSADATA wsadata;
	
	//1.加载套接字库
	WSAStartup(MAKEWORD(2,2), &wsadata);

	//2.创建套接字
	//创建成功则返回新创建的套接字的描述符,套接字描述符是一个整数类型的值
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 1)
	{
		printf("socket error\n");
		return 4;
	}
	
	sockaddr_in myaddr;//地址结构体
	myaddr.sin_addr.s_addr = INADDR_ANY;//指定ip地址
	myaddr.sin_family = AF_INET;//指定地址簇为TCP/IP协议
	myaddr.sin_port = htons(3000);//指明端口号

	//3.绑定到本机的一个地址和端口上
	int n = bind(fd, (sockaddr*)&myaddr, sizeof(myaddr));
	//绑定是否成功
	if (n != 0)
	{
		printf("bind port error\n");
		return 3;
	}
	
	//4.使流套接字fd处于监听状态,处于监听状态的流套接字s将维护一个客户连接请求队列,该队列最多容纳backlog个客户连接请求.
	//假如该函数执行成功，则返回0；如果执行失败，则返回SOCKET_ERROR
	listen(fd, n);

	while (true)
	{
		printf("waitting for new client\n");
		int newfd = 0;
		sockaddr faraddr;
		memset(&faraddr, 0, sizeof(faraddr));
		int addrlen = sizeof(faraddr);
		
		//5.等待客户端请求到来,当请求到来后,接受连接请求,返回一个新的对应此连接的套接字
		//accept默认是阻塞方式监听端口连接的，如果没有连接，就会一直阻塞在这里;
		//服务程序调用accept函数从处于监听状态的流套接字fd的客户连接请求队列中取出排在最前的一个客户请求,
		//并且创建一个新的套接字来与客户套接字创建连接通道;
		//
		//如果连接成功，就返回新创建的套接字的描述符，以后与客户套接字交换数据的是新创建的套接字;
		//如果失败就返回INVALID_SOCKET
		newfd = accept(fd, (sockaddr*)&faraddr, &addrlen);
		
		//判断新accept函数创建的套接字是否成功
		if (newfd < 1)
		{
			printf("accept error\n");
			
			//关闭套接字
			closesocket(fd);
			
			return 0;
		}
		printf("got new client\n");
	
		if (true)
		{
			CvCapture* capture = NULL;//CvCapture用来保存图像捕获的信息
			
			std::list<char*>::iterator it;//双向链表的迭代器it
			
			//判断视频格式是否是avi
			if (isAvi)
			{
				//从.avi文件中读取视频,并返回CvCapture结构指针
				capture = cvCaptureFromFile(argv[1]);
				
				if (NULL == capture)
				{
					printf("capture from avi file error\n");
					return 2;
				}
				
				//从文件中获取一帧
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
					//从文件中获取一帧,并将信息存储在IplImage中
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
				
				//htonl函数,将主机的无符号长整形数转换成网络字节顺序
				h.headlen = htonl(sizeof(HEAD));
				h.taillen = htonl(img->widthStep * img->height);
				h.h = htonl(img->height);
				h.w = htonl(img->width);
		
				char recvbuf[10];//接受端缓冲区
				
				//不论是客户还是服务器应用程序都用recv函数从TCP连接的另一端接收数据
				//参数1-newfd指定接收端套接字描述符
				//参数2-recvbuf指明一个缓冲区,该缓冲区用来存放recv函数接收的数据
				//参数3-指明recvbuf的长度
				//参数4-一般置0
				int e = recv(newfd, recvbuf, 1, 0);
				
				//recv函数接收数据是否出错
				if (e != 1)
				{
					shutdown(newfd, 2);
					closesocket(newfd);
					break;
				}
				
				//不论是客户还是服务器应用程序都用send函数来向TCP连接的另一端发送数据
				//客户程序一般用send函数向服务器发送请求，而服务器则通常用send函数来向客户程序发送应答
				//参数1-指定发送端套接字描述符
				//参数2-指明一个存放应用程序要发送数据的缓冲区
				//参数3-指明实际要发送的数据的字节数
				//参数4-一般置0
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
			
			//关闭套接字
			closesocket(newfd);
			newfd = 0;
		}
		else
		{

		}
	}

	return 0;
}

