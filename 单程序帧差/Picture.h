// GrayPicture.h: interface for the GrayPicture class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRAYPICTURE_H__D2BBB793_DB93_4751_88FB_809148356126__INCLUDED_)
#define AFX_GRAYPICTURE_H__D2BBB793_DB93_4751_88FB_809148356126__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class Picture  
{
public:
	Picture(unsigned int nWidth, unsigned int nHeight, unsigned int nChannel);
	virtual ~Picture();
	unsigned char* operator[](unsigned int nLineId);
	inline unsigned int getWidth(){return this->m_nWidth;}
	inline unsigned int getHeight(){return this->m_nHeight;}
private:
	unsigned char* m_data;
	unsigned int m_nWidth;
	unsigned int m_nHeight;
	unsigned int m_nChannel;

};

#endif // !defined(AFX_GRAYPICTURE_H__D2BBB793_DB93_4751_88FB_809148356126__INCLUDED_)
