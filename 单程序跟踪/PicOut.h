// PicOut.h: interface for the PicOut class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PICOUT_H__2D8F424C_08D1_438A_8AB4_7DCE59072B0D__INCLUDED_)
#define AFX_PICOUT_H__2D8F424C_08D1_438A_8AB4_7DCE59072B0D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//œ‘ æ ”∆µÕº∆¨
class PicOut  
{
public:
	PicOut();
	virtual ~PicOut();
	bool connect();
	bool sendFrame(long height, long width, void* imageBuf, long bufSize, bool isRgb, int id);
private:
	int m_nSocket;
};

#endif // !defined(AFX_PICOUT_H__2D8F424C_08D1_438A_8AB4_7DCE59072B0D__INCLUDED_)
