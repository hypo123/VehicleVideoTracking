// Pic.h: interface for the Pic class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PIC_H__D41CB794_B09B_4FF0_AADD_A5CBB143BD15__INCLUDED_)
#define AFX_PIC_H__D41CB794_B09B_4FF0_AADD_A5CBB143BD15__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class PicWhiteBlock
{
public:
	PicWhiteBlock(){init();}
	//���η�Χ
	short left;
	short right;
	short top;
	short down;

	//ĳ��������
	short x,y;

	long pixCount;

	PicWhiteBlock* next;

	void init()
	{
		down = -1;
		top = 30000;
		left = 30000;
		right = -1;
		pixCount = 0;
		x = -1;
		y = -1;
		next = 0;
	}

	void addPix(short row, short col)
	{
		++pixCount;
		if (row > down)
			down = row;
		else if (row < top)
			top = row;
		if (col > right)
			right = col;
		else if (col < left)
			left = col;

		if (-1 == x)
		{
			x = col;
			y = row;
		}
	}
};

class Pic  
{
public:
	Pic(long height, long width);
	virtual ~Pic();

	inline unsigned char* operator[](int n)
	{
		return line[n];
	}

	void inflate();	//����
	void corrode();	//��ʴ

	void fillhole();	//���,����ȡǰ������Ϣ
	
private:
	long ws;
	long w,h;
	unsigned char *data;
	unsigned char **line;

	//����ͳ��
private:
	void clearBlocks();
	PicWhiteBlock* addNewBlock();
public:
	void midvaluefilter();
	PicWhiteBlock* blocks;
};

#endif // !defined(AFX_PIC_H__D41CB794_B09B_4FF0_AADD_A5CBB143BD15__INCLUDED_)
