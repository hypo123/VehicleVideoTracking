#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <list>
#include <process.h>
#include <math.h>
#pragma comment(lib, "ws2_32.lib")

//结构体中变量4字节对齐
#pragma pack(4)
struct HEAD
{
	long headlen;
	long taillen;
	long h;
	long w;
};
#pragma pack()

#pragma pack(4)
struct HEAD2
{
	long headlen;
	long h;
	long w;
	long id;
	long taillen;
};
#pragma pack()

unsigned char rgb[10*1024*1024] = {0}; //原图

unsigned int bg[10*1024*1024] = {0}; //灰度背景
unsigned char bgshow[10*1024*1024] = {0}; //背景显示

unsigned char gray[10*1024*1024] = {0}; //灰度
unsigned char gray2[10*1024*1024] = {0}; //均值滤波
unsigned char gray3[10*1024*1024] = {0}; //中值滤波
unsigned char gray4[10*1024*1024] = {0}; //高斯滤波

double param = 0;
void td(void*)
{
	while (true)
	{
		char c = 0;
		scanf("%c", &c);
		if (c == '-')
		{
			param -= 0.01;
			printf("%f\n", param);
		}else if (c == '=')
		{
			param += 0.01;
			printf("%f\n", param);
		}
	}
}


int main(int argc, char* argv[])
{
	//创建一个新线程
	_beginthread(td, 100*1024, 0);
	
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,2), &wsadata);
	
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	int fd2 = socket(AF_INET, SOCK_STREAM, 0);
	
	if (true)
	{
		if (fd < 1 || fd2 < 1)
		{
			printf("socket error\n");
			WSACleanup();
			return 0;
		}
		
		sockaddr_in faraddr;
		faraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		faraddr.sin_family = AF_INET;
		faraddr.sin_port = htons(3000);
		
		sockaddr_in faraddr2;
		faraddr2.sin_addr.s_addr = inet_addr("127.0.0.1");
		faraddr2.sin_family = AF_INET;
		faraddr2.sin_port = htons(3001);
		
		if (2 == argc)
		{
			unsigned long l = inet_addr(argv[1]);
			if (l != 0 && l != 0xffffffff)
			{
				faraddr.sin_addr.s_addr = l;
				faraddr2.sin_addr.s_addr = l;
				printf("connecting 127.0.0.1:3000\n");
			}
			else
			{
				printf("connecting %s:3000\n",argv[1]);
			}
		}
		else
		{
			printf("connecting 127.0.0.1:3000\n");
		}
		
		int e = connect(fd, (sockaddr*)&faraddr, sizeof(faraddr));
		if (e < 0)
		{
			printf("connect failed\n");
			closesocket(fd);
			return 0;
		}
		printf("connected\n");

		printf("connecting port 3001\n");
		e = connect(fd2, (sockaddr*)&faraddr2, sizeof(faraddr2));
		if (e < 0)
		{
			printf("connect failed\n");
			closesocket(fd);
			return 0;
		}

	}
	printf("connected\n");
	
	HEAD head;

	printf("0号窗口彩图，1号灰度图，2号均值滤波，3号中值滤波，4号高斯滤波\n");

	int recvlen = 0;
	while (true)
	{
		send(fd, "a", 1, 0);
		for (int i=0; i<4; ++i)
		{
			int e = recv(fd, ((char*)&head)+i, 1, 0);
			if (e != 1){
				printf("recv error\n");
				closesocket(fd);
				return 0;
			}
		}
		head.headlen = ntohl(head.headlen);
		
		for (i=0; i<head.headlen - 4; ++i)
		{
			int e = recv(fd, ((char*)&head)+ 4 + i, 1, 0);
			if (e != 1)
			{
				printf("recv error\n");
				closesocket(fd);
				return 0;
			}
		}
		head.taillen = ntohl(head.taillen);
		head.h = ntohl(head.h);
		head.w = ntohl(head.w);
		
		int recvdLen = 0;
		int recvlen = head.taillen;
		while (recvlen > 0)
		{
			int l = recv(fd, (char*)rgb + recvdLen, recvlen, 0);
			if (l < 1)
			{
				printf("recv error\n");
				closesocket(fd);
				return 0;
			}
			recvdLen += l;
			recvlen -= l;
		}
		//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


		const int WS_OLD = (head.w * 3 + 3) ^ 0x03;
		const int WS_NEW = (head.w + 3) ^ 0x03;

		//启动高更新率，逐步降低到最低值。
		static int u2 = 300;
		if (u2 > 10)
			--u2;
		int update = u2 / 10;

		//rgb转gray
		for (i=2; i<head.h-2; ++i)
		{
			unsigned char *prgb = rgb + i * WS_OLD;
			unsigned int *pbg = bg + i * WS_NEW;
			unsigned char *pbgshow = bgshow + i*WS_NEW;
			unsigned char *pgray = gray + i*WS_NEW;
			unsigned char *pgray2 = gray2+i*WS_NEW;

			for (int j=2; j<head.w-2; ++j)
			{
				int b = prgb[j*3+0];
				int g = prgb[j*3+1];
				int r = prgb[j*3+2];
				int n = (r*306 + g*601 + b*116 + 512) >> 10;
				pgray[j] = n;
			}
		}

		//滤波
		for (i=2; i<head.h-2; ++i)
		{
			unsigned char *pgraya = gray + i*WS_NEW;
			unsigned char *pgrayb = gray + i*WS_NEW;
			unsigned char *pgrayc = gray + i*WS_NEW;

			unsigned char *pgray2 = gray2+i*WS_NEW;
			unsigned char *pgray3 = gray3+i*WS_NEW;
			unsigned char *pgray4 = gray4+i*WS_NEW;

			for (int j=2; j<head.w-2; ++j)
			{
				//均值滤波
				unsigned int n = 4 + 
				pgraya[j-1] + 
				pgraya[j-0] + 
				pgraya[j+1] + 
				pgrayb[j-1] + 
				//pgrayb[j-0] + 
				pgrayb[j+1] + 
				pgrayc[j-1] + 
				pgrayc[j-0] + 
				pgrayc[j+1] ;
				pgray2[j] = n / 8;

				//中值滤波
				int a[9] = {
				pgraya[j-1] ,
				pgraya[j-0] , 
				pgraya[j+1] , 
				pgrayb[j-1] , 
				pgrayb[j-0] , 
				pgrayb[j+1] , 
				pgrayc[j-1] , 
				pgrayc[j-0] , 
				pgrayc[j+1] 
				};
				for (int m=0; m<5;++m)
				{
					for (int n = 0; n<9-m-1; ++n)
					{
						if (a[n] > a[n+1])
						{
							int l = a[n];
							a[n] = a[n+1];
							a[n+1] = l;
						}
					}
				}
				pgray3[j] = a[4];

				//高斯滤波
				n = 8 + 
				pgraya[j-1] + 
				pgraya[j-0]*2 + 
				pgraya[j+1] + 
				pgrayb[j-1]*2 + 
				pgrayb[j-0]*4 + 
				pgrayb[j+1]*2 + 
				pgrayc[j-1] + 
				pgrayc[j-0]*2 + 
				pgrayc[j+1] ;
				pgray4[j] = n / 16;
			}
		}


		//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		//彩图
		if (true){
			HEAD2 head2;
			head2.headlen = htonl(sizeof(HEAD2));
			head2.h = htonl(head.h);
			head2.w = htonl(head.w);
			head2.id = htonl(0);
			head2.taillen = htonl(head.taillen);
			send(fd2, (char*)&head2, sizeof(head2), 0);
			send(fd2, (char*)rgb, head.taillen, 0);
		}

		//灰度图
		if (true)
		{
			HEAD2 head2;
			head2.headlen = htonl(sizeof(HEAD2));
			head2.h = htonl(head.h);
			head2.w = htonl(head.w);
			head2.id = htonl(1);
			head2.taillen = htonl(WS_NEW * head.h);
			send(fd2, (char*)&head2, sizeof(head2), 0);
			send(fd2, (char*)gray, ntohl(head2.taillen), 0);
		}

		//均值滤波
		if (true)
		{
			HEAD2 head2;
			head2.headlen = htonl(sizeof(HEAD2));
			head2.h = htonl(head.h);
			head2.w = htonl(head.w);
			head2.id = htonl(2);
			head2.taillen = htonl(WS_NEW * head.h);
			send(fd2, (char*)&head2, sizeof(head2), 0);
			send(fd2, (char*)gray2, ntohl(head2.taillen), 0);
		}

		//中值滤波
		if (true)
		{
			HEAD2 head2;
			head2.headlen = htonl(sizeof(HEAD2));
			head2.h = htonl(head.h);
			head2.w = htonl(head.w);
			head2.id = htonl(3);
			head2.taillen = htonl(WS_NEW * head.h);
			send(fd2, (char*)&head2, sizeof(head2), 0);
			send(fd2, (char*)gray3, ntohl(head2.taillen), 0);
		}

		//高斯滤波
		if (true)
		{
			HEAD2 head2;
			head2.headlen = htonl(sizeof(HEAD2));
			head2.h = htonl(head.h);
			head2.w = htonl(head.w);
			head2.id = htonl(4);
			head2.taillen = htonl(WS_NEW * head.h);
			send(fd2, (char*)&head2, sizeof(head2), 0);
			send(fd2, (char*)gray4, ntohl(head2.taillen), 0);
		}
 
		//峰值信噪比 均值 中值 高斯
		long mse2 = 0;
		long mse3 = 0;
		long mse4 = 0;
		for (i=2; i<head.h-2; ++i)
		{
			unsigned char *pgray = gray + i*WS_NEW;

			unsigned char *pgray2 = gray2+i*WS_NEW;
			unsigned char *pgray3 = gray3+i*WS_NEW;
			unsigned char *pgray4 = gray4+i*WS_NEW;

			for (int j=2; j<head.w-2; ++j)
			{
				if (pgray2[j] > pgray[j])
					mse2 += (pgray2[j]-pgray[j]) * (pgray2[j]-pgray[j]);
				else
					mse2 += (pgray[j]-pgray2[j]) * (pgray[j]-pgray2[j]);
				if (pgray3[j] > pgray[j])
					mse3 += (pgray3[j]-pgray[j]) * (pgray3[j]-pgray[j]);
				else
					mse3 += (pgray[j]-pgray3[j]) * (pgray[j]-pgray3[j]);
				if (pgray4[j] > pgray[j])
					mse4 += (pgray4[j]-pgray[j]) * (pgray4[j]-pgray[j]);
				else
					mse4 += (pgray[j]-pgray4[j]) * (pgray[j]-pgray4[j]);
			}
		}
		mse2 /= ((head.h-4)*(head.w-4));
		mse3 /= ((head.h-4)*(head.w-4));
		mse4 /= ((head.h-4)*(head.w-4));
		double psnr2 = 10 * log(255.0*255.0/mse2) / log(10.0);
		double psnr3 = 10 * log(255.0*255.0/mse3) / log(10.0);
		double psnr4 = 10 * log(255.0*255.0/mse4) / log(10.0);

		{
			static int aa=0;
			++aa;
			if (aa == 311)
				aa = 311;
			if (aa == 300)
				printf("均值滤波   中值滤波   高斯滤波\n");
			if (aa >= 300 && aa <310)
				printf("%10f %10f %10f\n", psnr2, psnr3, psnr4);
		}
	}
	
	return 0;
}

