// gmm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <math.h>
#include <list>
#include <assert.h>
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



/////////////////////////////////////////////////////////>>>>>>>>>>>>>>>>>>>>>>>>>>
/************************************************************************/
/* 混合高斯模型         
	 可以运行
*/
/************************************************************************/

#define EXCHANGE(x,y) {int temp=x; x=y; y=temp;}
const int MOD_COUNT = 10;
const int WEIGHT_MODIFY_TIME = 20;

class MgModel
{
	//数值放大100倍， 权重放大10000倍
private:
	static const int avgweight;
	static const int initS;

	//期望，标准差，方差，权
	long avg[MOD_COUNT];
	long s[MOD_COUNT];
	//long s_s[MOD_COUNT];
	long weight[MOD_COUNT];

	//更改权值needModifyWeight次后，再归一化
	char needModifyWeight;

	//新值覆盖查找处
	char writePos; 
	
public:
	MgModel();
	~MgModel();
	bool isBg(unsigned char value, int i, int j);
};

const int MgModel::avgweight = 10000/MOD_COUNT;
const int MgModel::initS = 1000;

MgModel::MgModel()
{
	for (int i=0; i<MOD_COUNT; ++i)
	{
		avg[i] = 25500 * i/ MOD_COUNT;
		s[i] = initS;
		weight[i] = 10000/MOD_COUNT;
	}
	needModifyWeight = WEIGHT_MODIFY_TIME;
	writePos = 0;
}

MgModel::~MgModel()
{
}

bool MgModel::isBg(unsigned char valueOld, int ii, int ji)
{
	const long value = valueOld * 100;
	
	long distanceMin = 1000000;
	long minId = -1;

	//0代表没计算
	long distance[MOD_COUNT] = {0};

	if (40 == ii && 40 == ji)
	{
		ii = ii;
	}

	for (int i=0; i<MOD_COUNT; ++i)
	{
		//横轴区间（μ-1.96σ，μ+1.96σ）内的面积为95.449974%
		//（μ-2.58σ，μ+2.58σ）内的面积为99.730020%
		//标准差必须大于0. 
		distance[i] = (value - avg[i]) * 100 / s[i];

		//正负
		long flag = distance[i] >= 0 ? 1 : -1;
		//绝对值
		distance[i] /= flag;

		//记录最小值
		if (distance[i] < distanceMin)
		{
			distanceMin = distance[i];
			minId = i;
		}

		//找到模型，剩余不再计算. 多数属于背景.
		if (distance[i] <= 258)
		{
			//更新期望权值，排序

			//至少5点
			avg[i] += flag*(5 + s[i] * distance[i] / 30000);

			//标准差
			if (distance[i] < 60)
			{
				s[i] -= 10;

				//禁止太小
				if (s[i] < 200)
				{
					s[i] = 200;
				}
			}
			else
			{
				s[i] += 10;
			}
			//s_s[i] = s[i]*s[i];

			bool rst = weight[i] >= avgweight;

			assert(10 == MOD_COUNT);
			weight[i] += 20;
			for (i=0; i<MOD_COUNT; ++i)
			{
				weight[i] -= 2;
			}

			--needModifyWeight;
			if (needModifyWeight <= 0)
			{
				//权总和修正
				int sum = 0;
				for (i=0; i<MOD_COUNT; ++i)
				{
					if (weight[i] < 120)
					{
						weight[i] = 120;
					}
					sum += weight[i];
				}
				for (i=0; i<MOD_COUNT; ++i)
				{
					weight[i] = weight[i] * 10000 / sum;
				}

				//高权值提前
				for (i=0; i<MOD_COUNT-1; ++i)
				{
					if (weight[i+1] > avgweight && weight[i] < weight[i+1])
					{
						EXCHANGE(weight[i], weight[i+1]);
						EXCHANGE(avg[i], avg[i+1]);
						EXCHANGE(s[i], s[i+1]);
					}
				}
				needModifyWeight = WEIGHT_MODIFY_TIME;
			}

			return rst;
		}
		
	}

	//不属于任一模型，添加值,轮流覆盖非背景
	for (int i=writePos; i< writePos+MOD_COUNT; ++i)
	{
		int j = i%MOD_COUNT;
		if (weight[j] <= avgweight)
		{
			writePos = (i+1)%MOD_COUNT;

			avg[j] = value;
			s[j] = initS;
			/*
			weight[i%MOD_COUNT] += 20;
			for (i=0; i<MOD_COUNT; ++i)
			{
				weight[i] -= 2;
			}
			*/
			break;
		}
	}
	
	return false;
}


unsigned char buf1[10*1024*1024] = {0}; //原图
unsigned char buf2[10*1024*1024] = {0}; //前景

unsigned char **picnew = NULL;

int main(int argc, char* argv[])
{
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
	printf("混合高斯背景模型：0号窗口彩图，1号前景图\n");
	HEAD head;

	MgModel **pgmm = NULL;
	
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
		
		for (int i=0; i<head.headlen - 4; ++i)
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
			int l = recv(fd, (char*)buf1 + recvdLen, recvlen, 0);
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

		if (NULL == pgmm)
		{
			pgmm = new MgModel*[head.h];
			for (int i=0; i<head.h; ++i)
			{
				pgmm[i] = new MgModel[head.w];
			}
		}

		if (true)
		{
			static int n = 0; 
			++n;
			if ((n%10) == 0)
			{
				printf("\r%d    ", n);
			}
		}

		const int WS_OLD = (head.w * 3 + 3) ^ 0x03;
		const int WS_NEW = (head.w + 3) ^ 0x03;

		::memset(buf2, 0, WS_NEW*3);
		::memset(buf2+WS_NEW*(head.h - 3), 0, WS_NEW*3);
		for (int i=head.h-1-3; i>=3; --i)
		{
			unsigned char *pLine1 = buf1 + i * WS_OLD;
			unsigned char *pLine2 = buf2 + i * WS_NEW;

			pLine2[0] = 
			pLine2[1] = 
			pLine2[2] = 
			pLine2[head.w-3] = 
			pLine2[head.w-2] = 
			pLine2[head.w-1] = 0;
			for (int j=head.w-1-3; j>=3; --j)
			{
				//转灰度
				int r = pLine1[j*3+2];
				int g = pLine1[j*3+1];
				int b = pLine1[j*3+0];
				int n = (r*306 + g*601 + b*116) >> 10;
				if (n > 255)
				{
					n = 255;
				}

				//高斯模型判断背景
				if (pgmm[i][j].isBg(n , i, j))
				{
					pLine2[j] = 0;
				}
				else
				{
					pLine2[j] = 255;
				}
			}
		}

		//彩图
		if (true){
			HEAD2 head2;
			head2.headlen = htonl(sizeof(HEAD2));
			head2.h = htonl(head.h);
			head2.w = htonl(head.w);
			head2.id = htonl(0);
			head2.taillen = htonl(head.taillen);
			send(fd2, (char*)&head2, sizeof(head2), 0);
			send(fd2, (char*)buf1, head.taillen, 0);
		}

		//前景图
		if (true)
		{
			HEAD2 head2;
			head2.headlen = htonl(sizeof(HEAD2));
			head2.h = htonl(head.h);
			head2.w = htonl(head.w);
			head2.id = htonl(1);
			head2.taillen = htonl(WS_NEW * head.h);
			send(fd2, (char*)&head2, sizeof(head2), 0);
			send(fd2, (char*)buf2, ntohl(head2.taillen), 0);
		}

		Sleep(30);
	}
	
	return 0;
}

