// PicSrc.cpp: implementation of the PicSrc class.
//
//////////////////////////////////////////////////////////////////////

#include "PicSrc.h"
#include <stdio.h>
#include <assert.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

PicSrc::PicSrc()
{
	::WSADATA wsadata;
	::WSAStartup(MAKEWORD(2,2), &wsadata);
	m_nSocket = 0;
}

PicSrc::~PicSrc()
{
	if (m_nSocket > 0)
	{
		::closesocket(m_nSocket);
		m_nSocket = 0;
	}
	::WSACleanup();
}
bool PicSrc::connect()
{
	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 1)
	{
		printf("socket error\n");
		return false;
	}
	
	sockaddr_in faraddr;
	faraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	faraddr.sin_family = AF_INET;
	faraddr.sin_port = htons(3000);

	int n = ::connect(fd, (sockaddr*)&faraddr, sizeof(faraddr));
	if (n < 0)
	{
		::printf("connect failed\n");
		::closesocket(fd);
		return false;
	}
	::printf("img src connected\n");
	m_nSocket = fd;
	return true;
}

#pragma pack(push,4)
struct HEAD
{
	long headlen;
	long taillen;
	long h;
	long w;
};
#pragma pack(pop)

bool PicSrc::getFrame(long* pHeight, long* pWidth, void* imageBuf, long bufSize)
{
	if (this->m_nSocket < 1)
		return false;

	//请求数据
	int n = ::send(m_nSocket, "a", 1, 0);
	if (n != 1)
	{
		printf("send error\n");
		return false;
	}

	//接收
	HEAD head;
	for (int i=0; i<4; ++i)
	{
		n = ::recv(m_nSocket, ((char*)&head)+i, 1, 0);
		if (n != 1)
		{
			printf("recv error\n");
			return false;
		}
	}
	head.headlen = ntohl(head.headlen);
	
	for (i=0; i<head.headlen - 4; ++i)
	{
		int e = ::recv(m_nSocket, ((char*)&head)+ 4 + i, 1, 0);
		if (e != 1)
		{
			printf("recv error\n");
			return false;
		}
	}
	head.taillen = ntohl(head.taillen);
	head.h = ntohl(head.h);
	head.w = ntohl(head.w);

	assert((head.h * ((head.w*3 +3) & (~3))) == head.taillen);
	
	if (pHeight != 0)
		*pHeight = head.h;
	if (pWidth != 0)
		*pWidth = head.w;
	
	if (bufSize < head.taillen)
	{
		printf("recv buf too short\n");
		return false;
	}

	int recvd = 0;
	int needRecv = head.taillen;
	while (needRecv > 0)
	{
		int l = recv(m_nSocket, ((char*)imageBuf) + recvd, needRecv, 0);
		if (l < 1)
		{
			printf("recv error\n");
			return false;
		}
		recvd += l;
		needRecv -= l;
	}
	return true;
}
