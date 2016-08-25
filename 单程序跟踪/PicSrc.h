// PicSrc.h: interface for the PicSrc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PICSRC_H__96B6D711_AC19_4A14_A25C_02F3FFD840D5__INCLUDED_)
#define AFX_PICSRC_H__96B6D711_AC19_4A14_A25C_02F3FFD840D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//ªÒ»° ”∆µÕº∆¨
class PicSrc  
{
public:
	PicSrc();
	virtual ~PicSrc();

	bool connect();
	bool getFrame(long* pHeight, long* pWidth, void* imageBuf, long bufSize);
private:
	int m_nSocket;


};

#endif // !defined(AFX_PICSRC_H__96B6D711_AC19_4A14_A25C_02F3FFD840D5__INCLUDED_)
