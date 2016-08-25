// PicOut.cpp: implementation of the PicOut class.
//
//////////////////////////////////////////////////////////////////////

#include "PicOut.h"
#include <stdio.h>
#include <assert.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

PicOut::PicOut()
{
	::WSADATA wsadata;
	::WSAStartup(MAKEWORD(2,2), &wsadata);
	m_nSocket = 0;
}

PicOut::~PicOut()
{
	if (m_nSocket > 0)
	{
		::closesocket(m_nSocket);
		m_nSocket = 0;
	}
	::WSACleanup();
}
bool PicOut::connect()
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
	faraddr.sin_port = htons(3001);

	int n = ::connect(fd, (sockaddr*)&faraddr, sizeof(faraddr));
	if (n < 0)
	{
		::printf("connect failed\n");
		::closesocket(fd);
		return false;
	}
	::printf("img out connected\n");
	m_nSocket = fd;
	return true;
}

#pragma pack(push,4)
struct HEAD
{
	long headlen;
	long h;
	long w;
	long id;
	long taillen;
};
#pragma pack(pop)

bool PicOut::sendFrame(long height, long width, void* imageBuf, long bufSize, bool isRgb, int id)
{
	if (this->m_nSocket < 1)
		return false;
	
	HEAD head;
	head.headlen = htonl(sizeof(HEAD));
	head.h = htonl(height);
	head.w = htonl(width);
	head.id = htonl(id);
	head.taillen = htonl(bufSize);

	assert( (((isRgb ? 3 : 1)* width+3) & (~3)) * height == bufSize);

	long n = send(m_nSocket, (char*)&head, sizeof(head), 0);
	n += send(m_nSocket, (char*)imageBuf, bufSize, 0);
	assert(n == (long)(bufSize + sizeof(head)));
	
	return true;
}
