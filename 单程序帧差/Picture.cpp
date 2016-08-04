// GrayPicture.cpp: implementation of the GrayPicture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Picture.h"
#include <assert.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Picture::Picture(unsigned int nWidth, unsigned int nHeight, unsigned int nChannel)
{
	assert(nWidth > 0);
	assert(nHeight > 0);
	assert(1 == nChannel || 3 == nChannel);
	this->m_nWidth = nWidth;
	this->m_nHeight = nHeight;
	this->m_nChannel = nChannel;
	this->m_data = new unsigned char[((nWidth*nChannel+3)&(~3)) * nHeight];
	memset(m_data, 0, ((nWidth*nChannel+3)&(~3)) * nHeight);
}

Picture::~Picture()
{
	delete[] m_data;
	m_data = 0;
}
unsigned char* Picture::operator[](unsigned int nLineId)
{
	assert(nLineId < this->m_nWidth);
	return m_data + ((m_nWidth * m_nChannel+3)&(~3)) * nLineId;
}