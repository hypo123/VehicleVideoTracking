#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <windows.h>
#include "PicSrc.h"
#include "PicOut.h"
#include "gmm.h"
#include "Kalman.h"
#include "coor.h"

#pragma warning(disable:4786)
#include <map>
#include <list>

#define ABS1(a) ( a >= 0 ? (a) : (-(a)))
#define DIF(a,b) ( (a) >= (b) ? (a - (b)) : (b-(a)) )
#define MAX3(a,b,c) ( (a)>(b) ? ((a)>(c)?(a):(c)) : ((b)>(c)?(b):(c)))
#define MIN3(a,b,c) ( (a)<(b) ? ((a)<(c)?(a):(c)) : ((b)<(c)?(b):(c)))
#define MID3(a,b,c) ( (a)>(b) ? (((a)>(c))?(((b)>(c))?(b):(c)):(a)) : (((c)>(b))?(b):((a)>(c))?(a):(c)) )
class Trajectory
{
public:
	Trajectory(){count = 0; color = rand() % 3;}

	void add(short row, short col){this->row[count] = row; this->col[count] = col; ++count;}
	int getCount(){return count;}
	short getRow(int which){assert(which>=0);assert(which<count); return row[which];}
	short getCol(int which){assert(which>=0);assert(which<count); return col[which];}

	bool isShow;
	char color;
	long id;
private:
	short row[1000];
	short col[1000];
	short count;
};

struct Charcter
{
	short startGray;
	short endGray;
	short pixCount;
};
struct RowInfo
{
	short row;//行
	short col;//列
	short count;//像素个数

	short hist[256/16];//灰度直方图
};

struct CompareResult
{
	//旧目标找到新目标了
	bool isMatched;

	//旧目标未找到新帧目标的帧数
	short missTime;

	//连续匹配上的帧数
	short matchCount;

};
//二值化前景中的一个物体
class ForeObject
{
public:
	ForeObject()
	{
		top = left = 32767;
		right = bottom = -1;
		startx = starty = -1;
		pixRowCount = 0;

		object = 0;

		pixColCount = 0;

		memset(pixRow, 0, sizeof(pixRow));
	
		charRowId = -1;
		charColId = -1;
	
		memset(rowChar, 0, sizeof(rowChar));
		memset(colChar, 0, sizeof(colChar));

		memset(pixCol, 0, sizeof(pixCol));
		pixColCount = 0;

		memset(&cmpResult, 0, sizeof(cmpResult));

		tj = new Trajectory();

		kalmanRow = new Kalman();
		kalmanCol = new Kalman();
	}
	~ForeObject()
	{
		while (edge.size() > 0)
		{
			long* p = edge.front();
			edge.pop_front();
			delete[] p;
		}

		if (object != 0)
			delete[] object;

		if (tj != 0)
			delete tj;

		if (kalmanRow != 0)
			delete kalmanRow;
		if (kalmanCol != 0)
			delete kalmanCol;
	}
	void addEdge(short row, short col)
	{
		if (col < left) left = col;
		if (col > right) right = col;
		if (row < top) top = row;
		if (row > bottom) bottom = row;

		bool create = false;
		long *p = 0;
		if (edge.size() < 1)
			create = true;
		else{
			p = edge.back();
			create = p[1000] >= 1000;
		}
		if (create)
		{
			p = new long[1001];
			p[1000] = 0;
			edge.push_back(p);
		}
		p[p[1000]] = (((long)row)<<16) | ((long)col);
		++p[1000];

		if (startx < 0)
		{
			startx = col;
			starty = row;
		}
	}

	void addPix(short row, short col, short count)
	{
		pixRow[pixRowCount].row = row;
		pixRow[pixRowCount].col = col;
		pixRow[pixRowCount].count = count;
		++pixRowCount;
		assert(pixRowCount < sizeof(pixRow)/sizeof(pixRow[0]));
	}

	//外框坐标
	short vhRect[10];
	void calcVHRect()
	{
		float left=10000;
		float right = -10000;
		float up = 10000;
		float down = -10000;
		for (int i=0; i<pixRowCount; ++i)
		{
			float h = coor_rc2h(pixRow[i].row + this->top, pixRow[i].col + this->left);
			if (h < left)
				left = h;

			h = coor_rc2h(pixRow[i].row + this->top, pixRow[i].col  + pixRow[i].count+ this->left);
			if (h > right)
				right = h;

			float v = coor_rc2v(pixRow[i].row + this->top, pixRow[i].col + this->left);
			if (v < up)
				up = v;
			if (v > down)
				down = v;
		}
		vhRect[0] = coor_vh2r(up, left);
		vhRect[1] = coor_vh2c(up, left);
		vhRect[2] = coor_vh2r(up, right);
		vhRect[3] = coor_vh2c(up, right);
		vhRect[4] = coor_vh2r(down, right);
		vhRect[5] = coor_vh2c(down, right);
		vhRect[6] = coor_vh2r(down, left);
		vhRect[7] = coor_vh2c(down, left);
		vhRect[8] = vhRect[0];
		vhRect[9] = vhRect[1];
	}


	//剪切时，需要范围信息
	short left;
	short top;
	short right;
	short bottom;

	//第一个像素的左标
	short startx;
	short starty;

	std::list<long*> edge;//存long[1001]的数组，末尾一个存本组个数，前面的，高2B存行，低2B存列
	RowInfo pixRow[1000];
	short pixRowCount;

	//剪切出的图片
	unsigned char* object;

	//特征行号(剪切图)
	short charRowId;
	short charColId;
	//特征数据
	Charcter rowChar[4];
	Charcter colChar[4];

	//列信息
	RowInfo pixCol[1000];
	short pixColCount;

	CompareResult cmpResult;

	Trajectory* tj;

	Kalman* kalmanRow;
	Kalman* kalmanCol;
};

void static int2pic(int n, unsigned char* pic, int row, int col, int width, int height);
bool handleNewFrame(const unsigned char* rgbImage, int height, int width, long frameId,
						 unsigned short* backgroundR, unsigned short* backgroundG, unsigned short* backgroundB,	//被更新的背景
						 unsigned char* backgrounddif	//接收背景差
						 );
void ushort2uchar(const unsigned short* background, unsigned char* image, int height, int width);
void ushort_background2uchar_rgb(const unsigned short* backgroundR, const unsigned short* backgroundG, const unsigned short* backgroundB,
								 unsigned char* rgbImage, int height, int width);
void ucharmulti(unsigned char* image, int height, int width);
unsigned char calcThresold(const unsigned char* image, int height, int width);
void backgroundDifModify(unsigned char* image, int height, int width, unsigned char thresold);
void inflate(const unsigned char* src, unsigned char* dst, int height, int width);
void fillhole(unsigned char* image, int height, int width, ForeObject** objects);
void cutObject(const unsigned char* rgbImage, int height, int width,
			   ForeObject* object);
void addLine(unsigned char* rgbImage, short height, short width, 
			 short startRow, short startCol, short endRow, short endCol, long color);
void drawRoadNum(unsigned char* rgbImage, short h, short w, short r, short c, const char* filename);
void midValue(unsigned char *rgbImage, short height, short width);
void save(const unsigned char* data, long width, long height, bool needConvertToRgb, long id)
{
	BITMAPFILEHEADER bmfile;
	bmfile.bfOffBits = 54;
	bmfile.bfSize = ((width*3+3) & (~3))*height + 54;
	bmfile.bfType = 'MB';
	bmfile.bfReserved1 = 0;
	bmfile.bfReserved2 = 0;
	BITMAPINFOHEADER bminfo;
	bminfo.biSize = 40+0;
	bminfo.biWidth = width;
	bminfo.biHeight = height;
	bminfo.biPlanes = 1;
	bminfo.biBitCount = 24;
	bminfo.biCompression = 0;;
	bminfo.biSizeImage = ((width*3+3) & (~3))*height;
	bminfo.biXPelsPerMeter = 0;
	bminfo.biYPelsPerMeter = 0;
	bminfo.biClrUsed = 0;
	bminfo.biClrImportant = 0;

	char filename[256] = {0};
	sprintf(filename, "c:\\%05d.bmp", id);
	FILE* f = fopen(filename, "wb");

	fwrite(&bmfile, 1, 14, f);
	fwrite(&bminfo, 1, 40, f);

	for (int i=height-1; i>=0; --i)
	{
		unsigned char buf[1024*3];
		const unsigned char* row = data + i * ((width*3+3) & (~3));
		if (needConvertToRgb)
		{
			row = data + i * ((width+3) & (~3));
			for (int j=0; j<width; ++j)
			{
				buf[j*3+0] = buf[j*3+1] = buf[j*3+2] = row[j];
			}
			row = buf;
		}
		fwrite(row, 1, ((width*3+3) & (~3)), f);
	}
	fclose(f);
}
/*
void shadow(const unsigned char* rgbImage, short height, short width,
			const unsigned short* bgR, const unsigned short* bgG, const unsigned short* bgB,
			unsigned char* objectBin)
{
	//rgb减小，亮度减小，颜色相似，认为是背景
	const short ws3 = (width*3+3)&(~3);
	const short ws  = (width+3)&(~3);

	for (int i=1; i<height-1; ++i)
	{
		const unsigned char* rgbLine = rgbImage + ws3 * i;
		const unsigned short* rLine = bgR + ws * i;
		const unsigned short* gLine = bgG + ws * i;
		const unsigned short* bLine = bgB + ws * i;
		unsigned char* binLine = objectBin + ws * i;
		for (int j=1; j<width-1; ++j)
		{
			if (binLine[j] == 0)
				continue;

			//前景
			short b = rgbLine[j*3];
			short g = rgbLine[j*3+1];
			short r = rgbLine[j*3+2];

			//背景
			short b2 = bLine[j*3] >> 6;
			short g2 = bLine[j*3+1] >> 6;
			short r2 = bLine[j*3+2]  >> 6;

			//某通道增加，保留
			if (b>=b2-10 || g>=g2-10 || r>=r2-10)
				continue;
			//亮度增大的,保留
			short vBG = MAX3(b2,g2,r2);
			short v   = MAX3(b,g,r);
			if (v >= vBG - 20)
				continue;
			//太黑的保留
			if (v < 40)
				continue;
			
			*(binLine+j) = 0;

		}
	}

}
*/
void main()
{
	PicSrc src;
	if(!src.connect())
		return ;
	PicOut out;
	if(!out.connect())
		return ;
	
	//新帧原图
	unsigned char* rgbImage = new unsigned char[500*500*3];

	//3通道背景
	unsigned short* backgroundR = new unsigned short[500*500];
	unsigned short* backgroundG = new unsigned short[500*500];
	unsigned short* backgroundB = new unsigned short[500*500];
	::memset(backgroundR, 0, 2*500*500);
	::memset(backgroundG, 0, 2*500*500);
	::memset(backgroundB, 0, 2*500*500);

	unsigned char* backgrounddif = new unsigned char[500*500];

	unsigned char* show = new unsigned char[500*500*3];

	long height=0;
	long width = 0;
	long widthStep3 = 0;
	long widthStep1 = 0;

	for (long scapeFrame=0; scapeFrame<0; ++scapeFrame)
		src.getFrame(&height, &width, rgbImage, 500*500*3);

	ForeObject *oldObjects[200] = {0};

	for (long frameId = 0; src.getFrame(&height, &width, rgbImage, 500*500*3); ++frameId)
	{
		//Sleep(1000/25);
		printf("frame%5d\n", frameId);

		widthStep3 = (width*3+3) & (~3);
		widthStep1 = (width+3) & (~3);

		midValue(rgbImage, height, width);

		ushort_background2uchar_rgb(backgroundR, backgroundG, backgroundB, show, height, width);
		out.sendFrame(height, width,show, height*widthStep3,true, 2);

		if (!handleNewFrame(rgbImage, height, width, frameId, backgroundR, backgroundG, backgroundB,backgrounddif))
		{
			continue;
		}

		if (frameId == 180)
		{
			frameId = frameId;
		}

		//画坐标线
		if (true)
		{
			memcpy(show, rgbImage, widthStep3*height);
			for (int i=0; i<20; i+=5)
			{
				float r = m2r(i);
				float c1 = coor_vh2c(r, 0);
				float h = coor_rc2h(150, 318);
				float c2 = coor_vh2c(r, h);
				addLine(show, height, width,
					r, c1, r, c2, 1);
			}
			for (i=0; i<=12; i+=2)
			{
				float r1 =0;
				float r2 = height-1;
				float c1 = coor_vh2c(r1, (240-26)/8*i);
				float c2 = coor_vh2c(r2, (240-26)/8*i);
				addLine(show, height, width, r1,c1,r2,c2, 1);
			}
			out.sendFrame(height, width,show, height*widthStep3,true, 1);

		}

		//memcpy(show, backgrounddif, widthStep1*height);
		//::ucharmulti(show, height, width);
		//out.sendFrame(height, width, show, widthStep1*height, false, 8);

		//背景差统计均值方差，修正。
		unsigned char thresold = calcThresold(backgrounddif, height, width);
		backgroundDifModify(backgrounddif, height, width, thresold);//已经灰度增强
		
		//膨胀腐蚀
		inflate(backgrounddif, show, height, width);
		inflate(show, backgrounddif, height, width);
		inflate(backgrounddif, show, height, width);
		inflate(show, backgrounddif, height, width);
		inflate(backgrounddif, show, height, width);
		inflate(show, backgrounddif, height, width);
		backgroundDifModify(backgrounddif, height, width, 60);
		//填充，统计
		ForeObject *objects[40] = {0};
		fillhole(backgrounddif, height, width, objects);
		out.sendFrame(height, width, backgrounddif, widthStep1*height, false, 8);

		//去掉小目标
		int objectCount = 0;
		for (;;)
		{
			if (objects[objectCount] == 0)
				break;
			if (objects[objectCount]->right - objects[objectCount]->left < width/20 
				|| objects[objectCount]->bottom - objects[objectCount]->top < width/20) 
			{
				
				delete objects[objectCount]; 
				for (int i = objectCount; ; ++i)
				{
					objects[i] = objects[i+1];
					if (objects[i] == 0)
						break;
				}
				continue;
			}
			++objectCount;
		}

		for (int i=0; objects[i]!=0; ++i)
		{
			cutObject(rgbImage, height, width, objects[i]);
			for (int j=0; j<objects[i]->pixRowCount; ++j)
			{
				objects[i]->pixRow[j].row -= objects[i]->top;
				objects[i]->pixRow[j].col -= objects[i]->left;
			}
		}
		for (i=0; objects[i]!=0; ++i)
		{
			ForeObject& obj = *(objects[i]);
			const short ws3 = ((obj.right - obj.left + 1)*3+3) & (~3);

			// 每一行的直方图
			for (int j=0; j<objects[i]->pixRowCount; ++j)
			{
				RowInfo& rowInfo = obj.pixRow[j];
				memset(rowInfo.hist, 0, sizeof(rowInfo.hist));
				const unsigned char* pLine = obj.object + ws3*rowInfo.row + rowInfo.col*3;
				for (int k=0; k<rowInfo.count; ++k)
				{
					short b = pLine[k*3+0];
					short g = pLine[k*3+1];
					short r = pLine[k*3+2];
					++rowInfo.hist[(r*3+g*6+b+5)/10 / 16];
				}

				// 做积分方便整体比较
				for (k=1; k<16; ++k)
					rowInfo.hist[k] += rowInfo.hist[k-1];
			}

			//统计列
			for (j=0; j<objects[i]->right - objects[i]->left + 1; ++j)
			{
				bool lastZero = true;
				for (int k=0; k<objects[i]->bottom - objects[i]->top + 1; ++k)
				{
					bool zero = obj.object[k*ws3+j*3+0] == 0;
					if (zero)
						zero = obj.object[k*ws3+j*3+1] == 0;
					if (zero)
						zero = obj.object[k*ws3+j*3+2] == 0;
					if (zero)
					{
						if (!lastZero)
						{
							++obj.pixColCount;
							assert(obj.pixColCount < sizeof(obj.pixCol) / sizeof(obj.pixCol[0]));
						}
						lastZero = true;
					}
					else
					{
						++obj.pixCol[obj.pixColCount].count;
						if (lastZero)
						{
							obj.pixCol[obj.pixColCount].row = k;// + objects[i]->top;
							obj.pixCol[obj.pixColCount].col = j;// + objects[i]->left;
							obj.pixCol[obj.pixColCount].count = 1;
						}
						lastZero = false;
					}
				}
				if (!lastZero)
				{
					++obj.pixColCount;
				}
			}

			// 每个列的直方图
			for (j=0; j<obj.pixColCount; ++j)
			{
				RowInfo& colInfo = obj.pixCol[j];
				memset(colInfo.hist, 0, sizeof(colInfo.hist));
				for (int k=0; k<colInfo.count; ++k)
				{
					const unsigned char* pix = obj.object + ws3 * (colInfo.row+k) + colInfo.col*3;
					++colInfo.hist[(pix[0]+pix[1]*6+pix[2]*3+5)/10 / 16];
				}

				// 做积分方便整体比较
				for (k=1; k<16; ++k)
					colInfo.hist[k] += colInfo.hist[k-1];
			}

		}

		//取特征线
		const float characterStart = 0.2f;
		const float characterEnd = 0.8f;
		for (i=0; oldObjects[i]!=0; ++i)
		{
			ForeObject& old = *(oldObjects[i]);

			//长度均值
			long pixSum = 0;
			for (int j=0; j<old.pixRowCount; ++j)
			{
				pixSum += old.pixRow[j].count;
			}
			long rowAvg = pixSum / old.pixRowCount;

			rowAvg *= 0.9f;

			//个别出现2/3都是超短线
			short longLineCount = 0;
			for (j=0; j<old.pixRowCount; ++j)
			{
				if (old.pixRow[j].count >= rowAvg)
					++longLineCount;
			}
			
			const long ws3 = ((old.right - old.left+1)*3+3) & (~3);//边缘上的线不要
			long difMax = 0;
			old.charRowId = 0;
			for (j=0; j<old.pixRowCount; ++j)
			{
				RowInfo& rowInfo = old.pixRow[j];

				if (j < 0.25f * old.pixRowCount || j > 0.75f * old.pixRowCount)
					continue;
				if (rowInfo.count < rowAvg)
					continue;
				
				long start = rowInfo.count * characterStart + rowInfo.col;
				long end = rowInfo.count * characterEnd + rowInfo.col;

				//相邻差
				if (true)
				{
					long difSum = 0;
					const unsigned char* pix = old.object + ws3 * rowInfo.row;
					for (int k=start; k<end; ++k)
					{
						if (pix[k*3] > pix[k*3+3])
							difSum += pix[k*3] - pix[k*3+3];
						else
							difSum += pix[k*3+3] - pix[k*3];

						if (pix[k*3+1] > pix[k*3+3+1])
							difSum += pix[k*3+1] - pix[k*3+3+1];
						else
							difSum += pix[k*3+3+1] - pix[k*3+1];

						if (pix[k*3+2] > pix[k*3+3+2])
							difSum += pix[k*3+2] - pix[k*3+3+2];
						else
							difSum += pix[k*3+3+2] - pix[k*3+2];

					}
					if (end - start >= 10)
					{
						long dif = difSum / (end-start);
						if (dif > difMax)
						{
							difMax = dif;
							old.charRowId = j;
						}
					}
				}

				//方差
				if (false)
				{
					long sqSum = 0;//平方和
					long sum = 0;//和
					const unsigned char* pix = old.object + ws3 * rowInfo.row;
					for (int k=start; k<=end; ++k)
					{
						sqSum += pix[k*3] * pix[k*3];
						sum += pix[k*3];
					}
					long dif = sqSum / (end+1-start) -  (sum/end+1-start)*(sum/end+1-start);
					if (dif > difMax)
					{
						difMax = dif;
						old.charRowId = j;
					}
				}
				
			}

			
			//特征线直方图
			long start = old.pixRow[old.charRowId].count * characterStart;
			long end = old.pixRow[old.charRowId].count * characterEnd;
			short hist[256/16] = {0};
			const unsigned char* pLine = old.object + ws3*old.pixRow[old.charRowId].row + old.pixRow[old.charRowId].col * 3;
			for (j=start; j<=end; ++j)
			{
				short b = pLine[j*3+0];
				short g = pLine[j*3+1];
				short r = pLine[j*3+2];
				++hist[(r*3+g*6+b+5)/10 / 16];
			}

			//特征数据
			old.rowChar[0].pixCount = 
			old.rowChar[1].pixCount = 
			old.rowChar[2].pixCount = 
			old.rowChar[3].pixCount = 0;
			int scopeId=0;
			bool lastZero=true;
			for (j=0; j<256/16; ++j)
			{
				if (hist[j] == 0)
				{
					if (!lastZero)
					{
						if (old.rowChar[scopeId].pixCount < 5)
							old.rowChar[scopeId].pixCount = 0;//太少，不要
						else
						{
							if (++scopeId > 3)
								break;
						}
					}
					lastZero = true;
				}
				else
				{
					if (lastZero)
					{
						old.rowChar[scopeId].pixCount = hist[j];
						old.rowChar[scopeId].startGray = j-1 >= 0 ? j-1 : 0;
						old.rowChar[scopeId].endGray = j+1+1 <= 256/16 ? j+1+1 : 256/16;
					}
					else
					{
						old.rowChar[scopeId].pixCount += hist[j];
						old.rowChar[scopeId].endGray = j+1+1 <= 256/16 ? j+1+1 : 256/16;
					}
					lastZero = false;
				}
			}
		}

		//取列特征线
		for (i=0; oldObjects[i]!=0; ++i)
		{
			ForeObject& old = *(oldObjects[i]);
			const short ws3 = ((old.right - old.left + 1)*3+3) & (~3);

			//列线平均长度
			long avgLen = 0;
			for (int j=0; j<old.pixColCount; ++j)
			{
				avgLen += old.pixCol[j].count;
			}
			avgLen /= old.pixColCount;

			//列线相邻差
			long difMax = 0;
			long maxId = -1;
			if (true)
			{
				for (j=old.pixColCount*0.35f; j<old.pixColCount*0.35f; ++j)
				{
					RowInfo& colInfo = old.pixCol[j];
					
					if (colInfo.count < avgLen || colInfo.count < 10)
						continue;
					
					long difSum = 0;
					short end = colInfo.count * characterEnd + colInfo.row;
					short start = colInfo.count * characterStart + colInfo.row;
					const unsigned char* pix = old.object + start * ws3 + colInfo.col * 3;
					for (int k=start; k<end; ++k)
					{
						if (k != start)
							pix += ws3;
						
						if (pix[0] > pix[ws3+0])
							difSum += (pix[0] - pix[ws3+0]);
						else
							difSum += (pix[ws3+0] - pix[0]);
						
						if (pix[1] > pix[ws3+1])
							difSum += (pix[1] - pix[ws3+1]);
						else
							difSum += (pix[ws3+1] - pix[1]);
						
						if (pix[0] > pix[ws3+2])
							difSum += (pix[2] - pix[ws3+2]);
						else
							difSum += (pix[ws3+2] - pix[2]);
					}
					long dif = difSum / (end - start);
					if (dif > difMax)
					{
						difMax = dif;
						maxId = j;
					}
				}
				if (maxId < 0)
					maxId = old.pixColCount / 2;
				old.charColId = maxId;
			}

			//方差
			if (false)
			{
				for (j=old.pixColCount*0.35f; j<old.pixColCount*0.65f; ++j)
				{
					RowInfo& colInfo = old.pixCol[j];
					
					if (colInfo.count < avgLen || colInfo.count < 10)
						continue;
					
					long sum = 0;//和
					long sqsum = 0;//平方和
					short end = colInfo.count * characterEnd + colInfo.row;
					short start = colInfo.count * characterStart + colInfo.row;
					const unsigned char* pix = old.object + start * ws3 + colInfo.col * 3;
					for (int k=start; k<=end; ++k)
					{
						if (k != start)
							pix += ws3;
						
						sum += pix[0];
						sqsum += pix[0] * pix[0];
					}
					long dif = sqsum/(end-start+1) - sum / (end - start+1) * sum / (end - start+1);
					if (dif > difMax)
					{
						difMax = dif;
						maxId = j;
					}
				}
				if (maxId < 0)
					maxId = old.pixColCount / 2;
				old.charColId = maxId;
			}

			//图像保存
			if (false)
			{

				save(old.object, old.right - old.left + 1, old.bottom - old.top + 1, false, 0);

				long w1 = old.right - old.left + 1;
				long h1 = old.bottom - old.top + 1;
				long step1 = (w1*3+3) & (~3);
				unsigned char* buf = new unsigned char[step1 * h1];
				memset(buf, 0, step1 * h1);

				long start = old.pixRow[old.charRowId].count * characterStart + old.pixRow[old.charRowId].col;
				long end = old.pixRow[old.charRowId].count * characterEnd + old.pixRow[old.charRowId].col;
				for (j=start; j<=end; ++j)
				{
					buf[old.pixRow[old.charRowId].row * step1 + j*3 + 0] = 
						old.object[old.pixRow[old.charRowId].row * step1 + j*3 + 0];
					buf[old.pixRow[old.charRowId].row * step1 + j*3 + 1] = 
						old.object[old.pixRow[old.charRowId].row * step1 + j*3 + 1];
					buf[old.pixRow[old.charRowId].row * step1 + j*3 + 2] = 
						old.object[old.pixRow[old.charRowId].row * step1 + j*3 + 2];
				}

				start = old.pixCol[old.charColId].count * characterStart + old.pixCol[old.charColId].row;
				end = old.pixCol[old.charColId].count * characterEnd + old.pixCol[old.charColId].row;
				for (j=start; j<=end; ++j)
				{
					buf[j * step1 + old.pixCol[old.charColId].col*3 + 0] = 
						old.object[j * step1 + old.pixCol[old.charColId].col*3 + 0];
					buf[j * step1 + old.pixCol[old.charColId].col*3 + 1] = 
						old.object[j * step1 + old.pixCol[old.charColId].col*3 + 1];
					buf[j * step1 + old.pixCol[old.charColId].col*3 + 2] = 
						old.object[j * step1 + old.pixCol[old.charColId].col*3 + 2];
				}
				save(buf, w1, h1, false, 1);
				delete[] buf;
			}
			
			//列特征数据
			short hist[256/16] = {0};
			for (j=old.pixCol[maxId].count * characterStart; j<=old.pixCol[maxId].count * characterEnd; ++j)
			{
				short b = old.object[j * ws3 + old.pixCol[maxId].col * 3 + 0];
				short g = old.object[j * ws3 + old.pixCol[maxId].col * 3 + 1];
				short r = old.object[j * ws3 + old.pixCol[maxId].col * 3 + 2];
				++hist[(r*3+b*6+g+5)/10 / 16];
			}

			old.colChar[0].pixCount = 
			old.colChar[1].pixCount = 
			old.colChar[2].pixCount = 
			old.colChar[3].pixCount = 0;
			short scanPos = 0;
			for (j=0; j<4;)
			{
				//跳过空白
				for (; scanPos<256/16 && hist[scanPos] == 0; ++scanPos)
					;
				if (scanPos<256/16)
					old.colChar[j].startGray = scanPos-1 >= 0 ? scanPos-1 : 0;
				else
					break;

				//统计个数
				for (; scanPos<256/16 && hist[scanPos] != 0; ++scanPos)
					old.colChar[j].pixCount += hist[scanPos];
				//数量小的丢弃
				if (old.colChar[j].pixCount < 5)
					old.colChar[j].pixCount = 0;
				else
				{
					old.colChar[j].endGray = scanPos+1 >= 256/16 ? 256/16 : scanPos + 1;
					++j;
				}
			}
		}

		//预测位置，找其目标，对其匹配。
		for (i=0; oldObjects[i]!=0; ++i)
		{
			ForeObject& old = *(oldObjects[i]);

			short predictRow = old.kalmanRow->getPredict();
			short predictCol = old.kalmanCol->getPredict();

			long distanceThresold = (old.right-old.left+1+old.bottom-old.top+1) / 4;
			distanceThresold *= distanceThresold;

			//寻找预测位置的目标
			short count = 0;
			short aimsId[100] = {0};

			//匹配出错时使用
			long minDistance = 0x7fffffff;
			short minDistanceId = 0;

			for (int j=0; objects[j] != 0; ++j)
			{
				ForeObject& obj = *(objects[j]);
				long row = (obj.bottom + obj.top)/2;
				long col = (obj.right + obj.left)/2;
				long distance = (row-predictRow)*(row-predictRow) + (col-predictCol)*(col-predictCol);
				if (distance < distanceThresold)
				{
					aimsId[count] = j;
					++count;
					if (distance < minDistance)
					{
						minDistance = distance;
						minDistanceId = j;
					}
				}
			}

			if (count > 0)
			{
				//取更相似者
				RowInfo searchRow;
					searchRow.row = old.pixRow[old.charRowId].row;
					searchRow.col = old.pixRow[old.charRowId].col + ((short)(old.pixRow[old.charRowId].count * characterStart));
					searchRow.count = ((short)(old.pixRow[old.charRowId].count * characterEnd)) - ((short)(old.pixRow[old.charRowId].count * characterStart));
				RowInfo searchCol;
					searchCol.col = old.pixCol[old.charColId].col;
					searchCol.row = old.pixCol[old.charColId].row + ((short)(old.pixCol[old.charColId].count * characterStart));
					searchCol.count = ((short)(old.pixCol[old.charColId].count * characterEnd)) - ((short)(old.pixCol[old.charColId].count * characterStart));

				long similarity = 0x7fffffff;
				short rowMove;
				short colMove;
				short selectedId = -1;

				//特征线除交点四方长度
				short up = searchRow.row - searchCol.row;
				short down = searchCol.row + searchCol.count - 1 - searchRow.row;
				short left = searchCol.col - searchRow.col;
				short right = searchRow.col + searchRow.count - 1 - searchCol.col;

				for (j=0; j<count; ++j)
				{
					ForeObject& obj = *(objects[aimsId[j]]);

					//行搜索范围
					char rowScope[200] = {0};//使用剪切的目标图行号
					//assert(200 > obj.pixRowCount);
					for (int k=0; k<obj.pixRowCount; ++k)
					{
						RowInfo& objRow = obj.pixRow[k];

						bool rowOk = true;
						for (int l=0; l<sizeof(old.rowChar)/sizeof(old.rowChar[0]); ++l)
						{
							Charcter& chr = old.rowChar[l];
							if (chr.pixCount < 5)
								continue;
							else
							{
								int below = chr.startGray < 1 ? 0 : objRow.hist[chr.startGray - 1];
								int all = objRow.hist[chr.endGray - 1];
								int newCount = all - below;
								if (newCount < ((long)(chr.pixCount * 0.9)) - 5)
								{
									rowOk = false;
									break;
								}
							}
						}
						rowScope[k] = rowOk ? 1 : 0;
					}

					//列搜索范围 范围扩大问题////////////////////////////
					char colScope[300] = {0};//使用剪切的目标图列号
					for (k=0; k<obj.pixColCount; ++k)
					{
						RowInfo& colInfo = obj.pixCol[k];

						bool colOk = true;
						for (int l=0; l<sizeof(old.colChar)/sizeof(old.colChar[0]); ++l)
						{
							Charcter& chr = old.colChar[l];

							if (chr.pixCount < 10)
								continue;
							else
							{
								int below = chr.startGray < 1 ? 0 : colInfo.hist[chr.startGray - 1];
								int all = colInfo.hist[chr.endGray - 1];
								int newCount = all - below;
								if (newCount < ((long)(chr.pixCount * 0.9)) - 5)
								{
									colOk = false;
									break;
								}
							}
						}
						colScope[k] = colOk ? 1 : 0;
					}
					
					// 十字线匹配
					short resultRow[20] = {0};//匹配上的十字线的行号
					short resultCol[20] = {0};
					long resultDif[20] = {0};
					short resultCount = 0;
					
					const short ws3 = ((obj.right - obj.left + 1)*3+3) & (~3);
					const short ws3old = ((old.right - old.left + 1)*3+3) & (~3);
					
					short lineDifThresold = 2560;
					
					for (k=0; k<obj.pixRowCount; ++k)
					{
						const RowInfo& objRow = obj.pixRow[k];
						
						if (rowScope[k] == 0)
							continue;
						if (objRow.row < up)
							continue;
						if (objRow.row + down >= old.bottom - old.top + 1)
							continue;
						for (int l=0; l<obj.pixColCount; ++l)
						{
							const RowInfo& objCol = obj.pixCol[l];

							if (colScope[l] == 0)
								continue;
							if (objRow.row - objCol.row < up)
								continue;
							if (objCol.row + objCol.count - objRow.row - 1 < down)
								continue;
							if (objCol.col - objRow.col < left)
								continue;
							if (objRow.col + objRow.count - objCol.col - 1 < right)
								continue;
							
							const unsigned char* objPix = obj.object + objRow.row * ws3 + (objCol.col - left)*3;
							const unsigned char* oldPix = old.object + searchRow.row * ws3old + searchRow.col*3;
							long difSum = 0;
							for (int m=0,m3=0; m<searchRow.count; ++m, m3+=3)
							{
								long a = objPix[m3+0] + objPix[m3+1]*6 + objPix[m3+2]*3;
								long b = oldPix[m3+0] + oldPix[m3+1]*6 + oldPix[m3+2]*3;
								if (a > b)
									difSum += a-b;
								else
									difSum += b-a;
							}
							
							objPix = obj.object + (objRow.row - up) * ws3 + objCol.col*3;
							oldPix = old.object + searchCol.row * ws3old + searchCol.col*3;
							for (m=0; m<searchCol.count; ++m)
							{
								if (m != 0)
								{
									objPix+=ws3;
									oldPix+=ws3old;
								}
								long a = objPix[0] + objPix[1]*6 + objPix[2]*3;
								long b = oldPix[0] + oldPix[1]*6 + oldPix[2]*3;
								if (a > b)
									difSum += a-b;
								else
									difSum += b-a;

							}
							difSum /= (searchRow.count + searchCol.count); //灰度差的10倍的平均
							
							if (difSum < lineDifThresold)
							{
								//装满了，则替换最大的，并降低lineDifThresold
								if (resultCount >= sizeof(resultDif)/sizeof(resultDif[0]))
								{
									long max = -1;
									long maxId = -1;
									for (m=0; m<sizeof(resultDif)/sizeof(resultDif[0]); ++m)
									{
										if (resultDif[m] > max)
										{
											max = resultDif[m];
											maxId = m;
										}
									}
									if (max < difSum)
									{
										lineDifThresold = max;
									}
									else
									{
										lineDifThresold = difSum;
										resultDif[maxId] = difSum;
										resultRow[maxId] = objRow.row;
										resultCol[maxId] = objCol.col;
									}
								}
								else
								{
									resultRow[resultCount] = objRow.row;
									resultCol[resultCount] = objCol.col;
									resultDif[resultCount] = difSum;
									++resultCount;
								}
								
							}
						}
					}
					
					//全图匹配,取最小值
					long picDifMin = 300;
					long picDifMinId = -1;
					for (k=0; k<resultCount; ++k)
					{
						long difSum = 0;
						short pixCount = 0;
						
						//旧帧目标矩形范围
						short oldStartRow = 0;
						short oldEndRow = old.bottom - old.top + 1;
						short oldStartCol = 0;
						short oldEndCol = old.right - old.left + 1;
						
						//旧帧目标矩形对应到新目标上的矩形范围
						short objStartRow = resultRow[k] - searchRow.row;
						short objEndRow = objStartRow + oldEndRow;
						short objStartCol = resultCol[k] - searchCol.col;
						short objEndCol = objStartCol + oldEndCol;
						
						//去掉没重叠部分
						if (objStartRow < 0)
						{
							oldStartRow -= objStartRow;
							objStartRow = 0;
						}
						if (objEndRow > obj.bottom - obj.top + 1)
						{
							oldEndRow -= (oldEndRow - (obj.bottom - obj.top + 1));
							objEndRow = obj.bottom - obj.top + 1;
						}
						if (objStartCol < 0)
						{
							oldStartCol -= objStartCol;
							objStartCol = 0;
						}
						if (objEndCol > obj.right - obj.left + 1)
						{
							oldEndCol -= (objEndCol - (obj.right - obj.left + 1));
							objEndCol = obj.right - obj.left + 1;
						}
						for (int l=objStartRow, m=oldStartRow; l<objEndRow; ++l, ++m)
						{
							const unsigned char* objRow = obj.object + ws3 * l;
							const unsigned char* oldRow = old.object + ws3old * m;
							for (int n=objStartCol, o=oldStartCol; n<objEndCol; ++n,++o)
							{
								short b = objRow[n*3+0];
								short g = objRow[n*3+1];
								short r = objRow[n*3+2];
								short b2 = oldRow[o*3+0];
								short g2 = oldRow[o*3+1];
								short r2 = oldRow[o*3+2];
								if (r + g + b == 0 || 
									r2+ g2+ b2== 0)
									continue;
								long gray1 = r*3+g*6+b;
								long gray2 = r2*3+g2*6+b2;
								if (gray1 > gray2)
									difSum += gray1-gray2;
								else
									difSum += gray2-gray1;
								++pixCount;
							}
						}
						if (pixCount == 0)
							difSum = 0x7fffffff;
						else
							difSum /= pixCount;
						if (picDifMin > difSum)
						{
							picDifMin = difSum;
							picDifMinId = k;
						}
					}//循环十字线的结果,全图匹配
					
					if (similarity > picDifMin)
					{
						//printf(" picDifMin %3d %5d\n", old.tj->id, picDifMin);
						similarity = picDifMin;
						rowMove = obj.top + resultRow[picDifMinId] - old.top - searchRow.row;
						colMove = obj.left + resultCol[picDifMinId] - old.left - searchCol.col;
						selectedId = j;
					}
				}

				//assert(selectedId >= 0);
				if (selectedId < 0)
				{
					
					if (count == 1)
					{
						//匹配得不好，没匹配上,且只有一个，那就是它吧。
						selectedId = 0;
					}
					else
					{
						selectedId = minDistanceId;
					}
					rowMove = objects[selectedId]->top - old.top;
					colMove = objects[selectedId]->left - old.left;
				}

				// 替换
				ForeObject& obj = *(objects[selectedId]);

				obj.cmpResult.isMatched = true;
				obj.cmpResult.missTime = 0;
				obj.cmpResult.matchCount = old.cmpResult.matchCount + 1;

				// 轨迹
				Trajectory* t = old.tj;
				old.tj = obj.tj;
				obj.tj = t;

				short posRow = t->getRow(t->getCount() - 1) + rowMove;
				short posCol = t->getCol(t->getCount() - 1) + colMove;

				/*
				//取点向中心修正
				if (posRow > (obj.top + obj.bottom)/2 + 1 && (rowMove >=1 || rowMove <= -1))
					--posRow;
				else if (posRow < (obj.top + obj.bottom)/2 - 1 && (rowMove >=3 || rowMove <= -1))
					++posRow;
				if (posCol > (obj.left + obj.right)/2 + 1 && (colMove >=1 || colMove <= -1))
					--posCol;
				else if (posCol < (obj.left + obj.right)/2 - 1 && (colMove >=1 || colMove <= -1))
					++posCol;
*/
				//模板匹配的轨迹
				/**/
				if (posRow >= 0 && posRow < height && posCol >=0 && posCol < width)
					t->add(posRow, posCol);


				//重心轨迹
//				t->add((objects[selectedId]->bottom + objects[selectedId]->top) / 2, (objects[selectedId]->right + objects[selectedId]->left)/2);


				Kalman* temp = oldObjects[i]->kalmanRow;
				oldObjects[i]->kalmanRow = objects[selectedId]->kalmanRow;
				objects[selectedId]->kalmanRow = temp;
				temp->goNext((objects[selectedId]->bottom + objects[selectedId]->top) / 2);

				temp = oldObjects[i]->kalmanCol;
				oldObjects[i]->kalmanCol = objects[selectedId]->kalmanCol;
				objects[selectedId]->kalmanCol = temp;
				temp->goNext((objects[selectedId]->right + objects[selectedId]->left)/2);
				
				delete oldObjects[i];
				oldObjects[i] = objects[selectedId];
				for (j=selectedId; objects[j] != 0; ++j)
					objects[j] = objects[j+1];
			}
			else
			{
				//没有目标
				old.cmpResult.isMatched = false;
				++(old.cmpResult.missTime);
				//old.cmpResult.matchCount
			}
		}

		//old中未被匹配的
		for (i=0; oldObjects[i]!=0;)
		{
			// 长期未被匹配的删除(延迟删除，防止由于分割尺寸导致末尾处重现)
			if (oldObjects[i]->cmpResult.missTime >= 20)
			{
				delete oldObjects[i];
				for (int j=i; oldObjects[j]!=0; ++j)
					oldObjects[j] = oldObjects[j+1];
				continue;
			}
			
			//未被匹配的做预测，预测到边界外的隐藏
			if (!oldObjects[i]->cmpResult.isMatched)
			{
				++oldObjects[i]->cmpResult.missTime;

				oldObjects[i]->kalmanRow->goNext();
				oldObjects[i]->kalmanCol->goNext();
				short row = oldObjects[i]->kalmanRow->getCurrent();
				short col = oldObjects[i]->kalmanCol->getCurrent();

				if (row < 0 || row >= height || col < 0 || col > width)
					oldObjects[i]->tj->isShow = false;
				else
					oldObjects[i]->tj->isShow = true;

			}
			else
				oldObjects[i]->tj->isShow = true;
			++i;
		}
		
		// 新目标中未被匹配上的目标放入old
		for (i=0; objects[i]!=0; ++i)
		{
			for (int l=0; ;++l)
			{
				if (oldObjects[l] == 0)
				{
					oldObjects[l] = objects[i];
					oldObjects[l]->cmpResult.missTime = 0;
					oldObjects[l]->kalmanRow->goNext((oldObjects[l]->top + oldObjects[l]->bottom)/2);
					oldObjects[l]->kalmanCol->goNext((oldObjects[l]->left + oldObjects[l]->right)/2);
					oldObjects[l]->tj->add((oldObjects[l]->top + oldObjects[l]->bottom)/2, (oldObjects[l]->left + oldObjects[l]->right)/2);
					static int id = 0;
					++id;
					oldObjects[l]->tj->id = id;
					break;
				}
			}
		}
		
		
		for (i=0; oldObjects[i]!=0; ++i)
		{
			if (oldObjects[i]->cmpResult.matchCount >= 1 && oldObjects[i]->tj->isShow)
			{
				Trajectory* tj = oldObjects[i]->tj;

				for (int j=0; j<tj->getCount() - 1; ++j)
				{
					addLine(rgbImage, height, width,
						tj->getRow(j), tj->getCol(j),
						tj->getRow(j+1), tj->getCol(j+1), tj->color);
				}

				//添加外框
				oldObjects[i]->calcVHRect();
				for (j=0; j<4; ++j)
				{
					addLine(rgbImage, height, width, oldObjects[i]->vhRect[j*2], oldObjects[i]->vhRect[j*2+1],
						oldObjects[i]->vhRect[j*2+2], oldObjects[i]->vhRect[j*2+3], tj->color);
				}

				//车辆位置
				float carleft = calcRoadNum(oldObjects[i]->vhRect[0], oldObjects[i]->vhRect[1]);
				float carright = calcRoadNum(oldObjects[i]->vhRect[2], oldObjects[i]->vhRect[3]);
				//修正
				carright = carleft+(carright-carleft)*0.9;

				float carwidth = carright - carleft;
				
				float center = (carleft+carright)/2;

				char filename[100] = {0};
				if (carright < 1)
					strcpy(filename, "road1.bmp");//1车道
				else if (carright - carwidth/3 < 1)
					strcpy(filename, "压线.bmp");//1车道压线2
				else if (carright - carwidth/3*2 < 1)
					strcpy(filename, "线上行驶.bmp");//1 2间线上行驶
				else if (carleft < 1)
					strcpy(filename, "压线.bmp");//2车道压线1
				else if (carright < 2)
					strcpy(filename, "road2.bmp");//2车道
				else if (carright - carwidth/3 < 2)
					strcpy(filename, "压线.bmp");//2车道压线3
				else if (carright - carwidth/3*2 < 2)
					strcpy(filename, "线上行驶.bmp");//2 3间线上行驶
				else if (carleft < 2)
					strcpy(filename, "压线.bmp");//3车道压线2
				else
					strcpy(filename, "road3.bmp");//3车道

				drawRoadNum(rgbImage, height, width, oldObjects[i]->top, oldObjects[i]->left, filename);
			}
		}
		out.sendFrame(height, width,rgbImage, height*widthStep3,true, 0);

	}
}

//rgb上添加线条
void addLine(unsigned char* rgbImage, short height, short width, 
			 short startRow, short startCol, short endRow, short endCol, long color)
{
	//起点在像素数据中的位置调整在前，终点在后
	if (startRow > endRow)
	{
		short t = startRow;
		startRow = endRow;
		endRow = t;
		t = startCol;
		startCol = endCol;
		endCol = t;
	}
	if (startRow == endRow && startCol > endCol)
	{
		short t = startCol;
		startCol = endCol;
		endCol = t;
	}

	//水平与垂直线单独处理
	if (startRow == endRow)
	{
		if (endCol < 0 || startCol >= width)
			return ;
		if (startCol < 0)
			startCol = 0;
		if (endCol >=width)
			endCol = width - 1;
	}
	if (startCol == endCol)
	{
		if (endRow < 0 || startRow >= height)
			return ;
		if (startRow < 0)
			startRow = 0;
		if (endRow >=height)
			endRow = height - 1;
	}

	//其余计算直线与矩形的相交线段
	{
		short& x1 = startCol;
		short& y1 = startRow;
		short& x2 = endCol;
		short& y2 = endRow;

		short x = 0;
		short y = 0;

		long d1 = (y2-y1)*x + (x1-x2)*y + y1*x2 - x1*y2;

		x = width-1;
		y = 0;
		long d2 = (y2-y1)*x + (x1-x2)*y + y1*x2 - x1*y2;

		x = 0;
		y = height-1;
		long d3 = (y2-y1)*x + (x1-x2)*y + y1*x2 - x1*y2;

		x = width-1;
		y = height-1;
		long d4 = (y2-y1)*x + (x1-x2)*y + y1*x2 - x1*y2;
		//直线与矩形无重叠
		if ((d1 > 0 && d2 >0 && d3>0 && d4>0) || (d1<0 && d2<0 && d3<0 && d4<0))
			return;

	}
	//线段与矩形无重叠
	if (endRow < 0 || startRow >= height || endCol < 0 || startCol >= width)
		return;

	//去除矩形外部的线段
	{
		float x1 = startCol;
		float y1 = startRow;
		float x2 = endCol;
		float y2 = endRow;

		if (y1 < 0)
		{
			x1 = (0-y1)/(y2-y1)*(x2-x1)+x1;
			y1 = 0;
		}
		if (y2 > height-1+0.1)
		{
			x2 = (height-1 - y1)/(y2-y1)*(x2-x1)+x1;
			y2 = height-1;
		}

		if (x1 < 0)
		{
			y1 = (0-x1)/(x2-x1)*(y2-y1) + y1;
			x1 = 0;
		}else if (x2 < 0)
		{
			y2 = (0-x1)/(x2-x1)*(y2-y1) + y1;
			x2 = 0;
		}
		if (x1 > width-1)
		{
			y1 = (width-1 - x1)/(x2-x1)*(y2-y1) + y1;
			x1 = width-1;
		}else if (x2 > width-1)
		{
			y2 = (width-1 - x1)/(x2-x1)*(y2-y1) + y1;
			x2 = width-1;
		}

		startCol= x1;
		startRow = y1;
		endCol = x2;
		endRow = y2;
	}

	const short ws = (width*3+3) & (~3);
	color %= 3;

	short horizon = startCol - endCol;
	if (horizon < 0)
		horizon *= -1;
	short vertical = startRow - endRow;
	if (vertical < 0)
		vertical *= -1;

	if (horizon > vertical)
	{
		int step = startCol > endCol ? -1 : 1;
		for (short i=startCol; i!=endCol; i+=step)
		{
			short row = startRow + (endRow - startRow) * (i - startCol) / (endCol - startCol);

			//rgbImage[row*ws + i*3 + color] = 255;
			rgbImage[row*ws + i*3 + color] = 0;
			rgbImage[row*ws + i*3 + (color+1)%3] = 0;
		}
	}
	else
	{
		int step = startRow > endRow ? -1 : 1;
		for (short i=startRow; i!= endRow; i+=step)
		{
			short col = startCol + (endCol - startCol) * (i - startRow) / (endRow - startRow);
			//rgbImage[i*ws + col*3 + color] = 255;
			rgbImage[i*ws + col*3 + color] = 0;
			rgbImage[i*ws + col*3 + (color+1)%3] = 0;
		}
	}
}

//剪切目标
void cutObject(const unsigned char* rgbImage, int height, int width,
			   ForeObject* object)
{
	long ws3big = (width*3 + 3) & (~3);
	long ws3small = ((object->right - object->left+1)*3 + 3) & (~3);

	object->object = new unsigned char[ws3small * (object->bottom - object->top+1)];
	memset(object->object, 0, ws3small * (object->bottom - object->top+1));

	for (int i=0; i<object->pixRowCount; ++i)
	{
		memcpy(object->object + ws3small * (object->pixRow[i].row - object->top) + 3 * (object->pixRow[i].col - object->left),
			rgbImage + ws3big * object->pixRow[i].row + 3*object->pixRow[i].col,
			object->pixRow[i].count * 3);
	}
}

//放大数值
void ucharmulti(unsigned char* image, int height, int width)
{
	const int widthstep = (width+3)&(~3);
	const int widthstep3 = (width*3+3)&(~3);
	const long len = widthstep * height;
	for (int i=0; i<len; ++i)
	{
		if (image[i] > 51)
			image[i] = 255;
		else
			image[i] *= 5;
	}
}

//3通道short转彩图
void ushort_background2uchar_rgb(const unsigned short* backgroundR, const unsigned short* backgroundG, const unsigned short* backgroundB,
								 unsigned char* rgbImage, int height, int width)
{
	const int widthstep = (width+3)&(~3);
	const int widthstep3 = (width*3+3)&(~3);
	for (int i=0; i<height; ++i)
	{
		unsigned char* rgbline = rgbImage + widthstep3 * i;
		const unsigned short* rline = backgroundR + widthstep * i;
		const unsigned short* gline = backgroundG + widthstep * i;
		const unsigned short* bline = backgroundB + widthstep * i;
		for (int j=0, j3=0; j<width; ++j, j3+=3)
		{
			rgbline[j3] = bline[j]>>6;
			rgbline[j3+1] = gline[j]>>6;
			rgbline[j3+2] = rline[j]>>6;
		}
	}
}

//ushort转uchar
void ushort2uchar(const unsigned short* background, unsigned char* image, int height, int width)
{
	const int widthstep = (width+3)&(~3);
	const int widthstep3 = (width*3+3)&(~3);
	for (int i=0; i<height; ++i)
	{
		const unsigned short* usline = background + widthstep * i;
		unsigned char* ucline = image + widthstep * i;
		for (int j=0; j<width; ++j)
		{
			ucline[j] = (usline[j]+32) >> 6;
		}
	}
}

void inflate(const unsigned char* src, unsigned char* dst, int height, int width)
{
	const int SPACE_COUNT = 3;
	const int widthstep = (width+3)&(~3);

	::memset(dst, 0, widthstep*(SPACE_COUNT+3));

	for (int i=SPACE_COUNT; i<height-SPACE_COUNT; ++i)
	{
		const unsigned char* line0 = src + widthstep * i;
		unsigned char* line3 = dst + widthstep * (i-2);
		unsigned char* line4 = dst + widthstep * (i-1);
		unsigned char* line5 = dst + widthstep * i;
		unsigned char* line6 = dst + widthstep * (i+1);
		unsigned char* line7 = dst + widthstep * (i+2);
		for (int j=SPACE_COUNT; j<width-SPACE_COUNT; ++j)
		{
			//把原图的当前像素值，分发到新图中
			short div = (((short)(line0[j]))+4)/9;
			line7[j] = div & 0xff;
			if (div == 0)
				continue;
			short temp;
			temp = line3[j] + div; line3[j] = temp > 255 ? 255 : temp;
			temp = line4[j] + div; line4[j] = temp > 255 ? 255 : temp;
			temp = line5[j-2] + div; line5[j-2] = temp > 255 ? 255 : temp;
			temp = line5[j-1] + div; line5[j-1] = temp > 255 ? 255 : temp;
			temp = line5[j] + div; line5[j] = temp > 255 ? 255 : temp;
			temp = line5[j+1] + div; line5[j+1] = temp > 255 ? 255 : temp;
			temp = line5[j+2] + div; line5[j+2] = temp > 255 ? 255 : temp;
			temp = line6[j] + div; line6[j] = temp > 255 ? 255 : temp;
			//line7[j] = div & 0xff;
		}
	}
	::memset(dst, 0, widthstep*SPACE_COUNT);
	::memset(dst+widthstep*(height-SPACE_COUNT), 0, widthstep*SPACE_COUNT);

	int set0len = widthstep - width + SPACE_COUNT + SPACE_COUNT;
	unsigned char* startpos = dst + width - SPACE_COUNT;
	for (i=0; i<height-1; ++i)
	{
		memset(startpos, 0, set0len);
		startpos += widthstep;
	}
}


//背景差修正.低于阈值的去除，高于的，放大5倍。
void backgroundDifModify(unsigned char* image, int height, int width, unsigned char thresold)
{
	const int widthstep = (width+3)&(~3);
	for (int i=0; i<height; ++i)
	{
		unsigned char* line = image + widthstep * i;
		for (int j=0; j<width; ++j)
		{
			if (line[j] < thresold || i<3  || i>=height-3 || j<3 || j >= width-3)
				line[j] = 0;
			else if (line[j] > 51)
				line[j] = 255;
			else
				line[j] *= 5;
		}
	}
}

long mySqrt(unsigned long n)
{
	long table[256] = 
	{
		0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 
		4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10,
		10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 
		11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 
		11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 
		12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 
		13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 
		13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
		14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 
		14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 
		15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 
		15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	};
	if (n < 256)
		return table[n];
	else
		return (long)sqrt(n);
}

//计算背景差分割阈值
unsigned char calcThresold(const unsigned char* image, int height, int width)
{
	const int widthstep = (width+3)&(~3);
	long sumxx = 0;//平方的和
	long sumx = 0;//和
	for (int i=0; i<height; ++i)
	{
		const unsigned char* line = image + widthstep * i;
		for (int j=0; j<width; ++j)
		{
			unsigned char n = line[j];
			if (n > 50){
				sumxx += 2500;
				sumx += 50;
			}else{
				sumxx += n*n;
				sumx += n;
			}
		}
	}
	//方差 = 平方的期望 - 期望的平方
	long qiwang = sumx / (height * width);
	long pingfangqiwang = sumxx / (height * width);
	long D2 = pingfangqiwang - qiwang * qiwang;
	
	return qiwang + mySqrt(D2) + 5;
}

//更新3通道背景,提取前景
bool handleNewFrame(const unsigned char* rgbImage, int height, int width, long frameId,
						 unsigned short* backgroundR, unsigned short* backgroundG, unsigned short* backgroundB,	//被更新的背景
						 unsigned char* backgrounddif	//接收背景差
						 )
{
	const int widthstep = (width+3)&(~3);
	const int widthstep3 = (width*3+3)&(~3);
	const bool isStartStep1 = frameId < 60;
	const bool isStartStep2 = frameId >= 60 && frameId < 120;
	const bool isStartStep3 = frameId >= 120 && frameId < 180;
	for (int i=0; i<height; ++i)
	{
		const unsigned char* rgbline = rgbImage + widthstep3 * i;
		unsigned short* rline = backgroundR + widthstep * i;
		unsigned short* gline = backgroundG + widthstep * i;
		unsigned short* bline = backgroundB + widthstep * i;
		unsigned char* difline = backgrounddif + widthstep * i;
		for (int j=0, j3=0; j<width; ++j, j3+=3)
		{
			long newr,newg,newb,oldr,oldg,oldb;
			newb = rgbline[j3]<<6;	//放大64倍
			newg = rgbline[j3+1]<<6;
			newr = rgbline[j3+2]<<6;
			oldb = bline[j];
			oldg = gline[j];
			oldr = rline[j];

			//计算背景差，后面统计背景差，再分割前景。
			long difb=DIF(newb,oldb);
			long difg=DIF(newg,oldg);
			long difr=DIF(newr,oldr);
			long dif = difb > difg ? (difb > difr ? difb : difr) : (difg>difr ? difg : difr);
			difline[j] = (dif + 32) >> 6;

			if (isStartStep1)
			{
				oldb = ((oldb << 4) + newb+8) / 17;
				oldg = ((oldg << 4) + newg+8) / 17;
				oldr = ((oldr << 4) + newr+8) / 17;
			}else if(isStartStep2)
			{
				oldb = ((oldb << 6) + newb+32) / 65;
				oldg = ((oldg << 6) + newg+32) / 65;
				oldr = ((oldr << 6) + newr+32) / 65;
			}else if(isStartStep3)
			{
				oldb = ((oldb << 8) + newb+128) / 257;
				oldg = ((oldg << 8) + newg+128) / 257;
				oldr = ((oldr << 8) + newr+128) / 257;
			}else{//前景仍更新，慢更新
				oldb = ((oldb << 10) + newb+512) / 1025;
				oldg = ((oldg << 10) + newg+512) / 1025;
				oldr = ((oldr << 10) + newr+512) / 1025;
			}

			bline[j] = oldb & 0xffff;
			gline[j] = oldg & 0xffff;
			rline[j] = oldr & 0xffff;
		}
	}
	return frameId >= 180;
}



//(单通道)图像中添加数字
static unsigned char numPic[8][64] = {	"100011110111100011100011111011000001100011000001100011100011",
								"011101100111011101011101110011011111011011011011011101011101",
								"011101110111011101111101101011011111011111111011011101011101",
								"011101110111111011110011101011000011000011110111100011011101",
								"011101110111110111111101011011111101011101110111011101100001",
								"011101110111101111111101100001111101011101110111011101111101",
								"011101110111011111011101111011011101011101110111011101101101",
								"100011100011000001100011111001100011100011110111100011100011"};
void static numPicInit()
{
	for (int i=0; i<sizeof(numPic); ++i)
	{
		if (((unsigned char*)numPic)[i] != '0')
		{
			((unsigned char*)numPic)[i] = 0;
		}
		else
		{
			((unsigned char*)numPic)[i] = 255;
		}
	}
}
void static int2pic(int n, unsigned char* pic, int row, int col, int width, int height)
{
	row -= 15;
	col -= 15;
	if (row < 0)
		row = 0;
	if (col < 0)
		col = 0;
	static bool inited = false;
	if (!inited)
	{
		numPicInit();
		inited = true;
	}

	char str[100] = {0};
	sprintf(str, "%d", n);

	int len = strlen(str);
	for (int i = 0; i<8; ++i)
	{
		if (row + i >= height)
		{
			break;
		}

		char buf[120] = {0};
		for (int j=0; j<len; ++j)
		{
			memcpy(buf + j * 6, &(numPic[i][(str[j]-'0')*6]), 6);
		}

		int cpyLen = len*6;
		if (col + cpyLen >= width)
		{
			cpyLen = width - col -1;
		}
		memcpy(pic + (row+i)*width + col, buf, cpyLen);
	}
}




//空洞填充。全图扫描一遍，再加外边缘扫描一遍
void fillhole(unsigned char* image, int height, int width, ForeObject** objects)
{
	const int widthstep = (width+3)&(~3);
	const int widthstep3 = (width*3+3)&(~3);
	
	unsigned char** line = new unsigned char*[height];
	for (int i=0; i<height; ++i)
	{
		line[i] = image + widthstep * i;
	}

	//扫描轮廓时，把目标的左边缘记录下，坐标与id映射.
	std::map<long, long> leftEdgeMap;
	long nextId = 0;
	std::map<long, long>::iterator it;

	//objects数量不够时，数据倒到这里
	ForeObject* garbage = new ForeObject();

	//填充时，当前像素所属目标。
	ForeObject* objCurrentArea = 0;
	
	bool inArea = false;
	for (i=1; i<height-1; ++i)
	{
		//记录目标行线
		short aimRowLineRow;
		short aimRowLineCol;
		short aimRowLinePixCount = 0;
		for (int j=1; j<width-1; ++j)
		{
			if (!inArea)
			{
				if (line[i][j] == 0)
					continue;
				else if (line[i][j] == 255)
				{					
					//扫描轮廓。 起始点，以左侧开始，顺时针。上下侧的边缘标记，不会使用到。
					const char up=1, right=2,down=4,left=8;
					short posRow=i, posCol=j;//当前像素
					char edge = left;	//当前边缘在当前像素左边缘
					line[posRow][posCol] &= (~left);

					leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;

					ForeObject* pObject = 0; 
					if (nextId < 100){
						pObject = objects[nextId] = new ForeObject();
						objects[nextId+1] = 0;
					}else{
						pObject = garbage;
					}

					while (true)
					{
						pObject->addEdge(posRow, posCol);

						switch(edge)
						{
						case left:
							if (line[posRow-1][posCol-1] != 0)
							{
								--posRow;
								--posCol;
								edge = down;
							}else if (line[posRow-1][posCol] != 0)
							{
								--posRow;
								line[posRow][posCol] &= ~left;								
								edge = left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
							}else if (line[posRow-1][posCol+1] != 0)
							{
								//up
								--posRow;
								++posCol;
								line[posRow][posCol] &= ~left;
								edge = left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
							}else if (line[posRow][posCol+1] != 0)
							{
								//up
								++posCol;
								//up
								edge = up;
							}else if (line[posRow+1][posCol+1] != 0)
							{
								//up,right
								line[posRow][posCol] &= ~right;
								++posRow;
								++posCol;
								//up
								edge = up;
							}else if (line[posRow+1][posCol] != 0)
							{
								//up,right
								line[posRow][posCol] &= ~right;
								++posRow;
								//right
								line[posRow][posCol] &= ~right;
								edge = right;
							}else if (line[posRow+1][posCol-1] != 0)
							{
								//up,right,down
								line[posRow][posCol] &= ~right;
								++posRow;
								--posCol;
								//right
								line[posRow][posCol] &= ~right;
								edge = right;
							}else
							{
								//单个像素
								line[posRow][posCol] &= ((~right) & (~left));
								edge = left; //回到起点（有可能还有边缘）
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
							}
							break;
						case up:
							if (line[posRow-1][posCol+1] != 0)
							{
								--posRow;
								++posCol;
								line[posRow][posCol] &= ~left;
								edge = left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
							}else if (line[posRow][posCol+1] != 0)
							{
								++posCol;
								edge = up;
							}else if (line[posRow+1][posCol+1] != 0)
							{
								line[posRow][posCol] &= ~right;
								++posRow;
								++posCol;
								edge = up;
							}else if (line[posRow+1][posCol] != 0)
							{
								line[posRow][posCol] &= ~right;
								++posRow;
								line[posRow][posCol] &= ~right;
								edge = right;
							}else if (line[posRow+1][posCol-1] != 0)
							{
								line[posRow][posCol] &= ~right;
								++posRow;
								--posCol;
								line[posRow][posCol] &= ~right;
								edge = right;
							}else if (line[posRow][posCol-1] != 0)
							{
								line[posRow][posCol] &= ~right;
								--posCol;
								edge = down;
							}else if (line[posRow-1][posCol-1] != 0)
							{
								line[posRow][posCol] &= ((~right) & (~left));
								--posRow;
								--posCol;
								edge = down;
							}else
							{
								assert(false);
							}
							break;
						case right:
							if (line[posRow+1][posCol+1] != 0)
							{
								++posRow;
								++posCol;
								edge = up;
							}else if (line[posRow+1][posCol] != 0)
							{
								++posRow;
								line[posRow][posCol] &= ~right;
								edge = right;
							}else if (line[posRow+1][posCol-1] != 0)
							{
								++posRow;
								--posCol;
								line[posRow][posCol] &= ~right;
								edge = right;
							}else if (line[posRow][posCol-1] != 0)
							{
								--posCol;
								edge = down;
							}else if (line[posRow-1][posCol-1] != 0)
							{
								line[posRow][posCol] &= ~left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								--posRow;
								--posCol;
								edge = down;
							}else if (line[posRow-1][posCol] != 0)
							{
								line[posRow][posCol] &= ~left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								--posRow;
								line[posRow][posCol] &= ~left;
								edge = left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
							}else if (line[posRow-1][posCol+1] != 0)
							{
								line[posRow][posCol] &= ~left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								--posRow;
								++posCol;
								line[posRow][posCol] &= ~left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								edge = left;
							}else
							{
								assert(false);
							}
							break;
						case down:
							if (line[posRow+1][posCol-1] != 0)
							{
								++posRow;
								--posCol;
								line[posRow][posCol] &= ~right;
								edge = right;
							}else if (line[posRow][posCol-1] != 0)
							{
								--posCol;
								edge = down;
							}else if (line[posRow-1][posCol-1] != 0)
							{
								line[posRow][posCol] &= ~left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								--posRow;
								--posCol;
								edge = down;
							}else if (line[posRow-1][posCol] != 0)
							{
								line[posRow][posCol] &= ~left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								--posRow;
								line[posRow][posCol] &= ~left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								edge = left;
							}else if (line[posRow-1][posCol+1] != 0)
							{
								line[posRow][posCol] &= ~left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								--posRow;
								++posCol;
								line[posRow][posCol] &= ~left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								edge = left;
							}else if (line[posRow][posCol+1] != 0)
							{
								line[posRow][posCol] &= ~left;
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								++posCol;
								edge = up;
							}else if (line[posRow+1][posCol+1] != 0)
							{
								line[posRow][posCol] &= ((~left) & (~right));
								leftEdgeMap[(((long)posRow) << 16) | ((long)posCol)] = nextId;
								++posRow;
								++posCol;
								edge = up;
							}else{
								assert(false);
							}
							
							break;
						default:
							assert(false);
							break;
						}
						
						if (posRow == i && posCol == j)
						{
							bool over = true;
							switch(edge)
							{
							case left:
								//结束
								break;
							case up:
							case right:
								assert(false);
								break;
							case down:
								if (line[posRow+1][posCol-1] != 0)
								{
									++posRow;
									--posCol;
									line[posRow][posCol] &= ~right;
									over = false;
								}
								break;
							default:
								assert(false);
								break;
							}
							if (over)
								break;
						}
					}//while(true)
					++nextId;					
				}//else if (line[i][j] == 255)
				else{
				}
				
				if (line[i][j] == (255 & (~8/*left*/)))
				{
					inArea = true;
				}else if (line[i][j] == (255 & (~8/*left*/) & (~2/*right*/)))
				{
				}else
				{
					//不该到达这里
					assert(false);
				}

				it = leftEdgeMap.find((i<<16) | j);
				//assert(it != leftEdgeMap.end());
				long id = it->second;
				//assert(id >= 0 && id < nextId);
				if (id >= 0 && id < nextId)
					objCurrentArea = objects[id];
				else
					objCurrentArea = garbage;
				//objCurrentArea->addPix(i, j);

				aimRowLineRow = i;
				aimRowLineCol = j;
				aimRowLinePixCount = 1;
				
				line[i][j] = 255;
			}	//if (!inArea)
			else
			{
				// 上1 右2 下4 左8
				switch(line[i][j])
				{
				case 0:
					break;
				case 255:
					break;
				case (255 & (~2)):
					inArea = false;
					objCurrentArea->addPix(aimRowLineRow, aimRowLineCol, aimRowLinePixCount);
					break;
				default:
					assert(false);
					break;
				}
				line[i][j] = 255;
				++aimRowLinePixCount;
				//objCurrentArea->addPix(i,j);
			}
		}
	}
	delete[] line;
	delete garbage;
}

//使目标像素区域突出显示
void markObject(unsigned char* rgbImage, ForeObject* object)
{
	//屏幕取点，左上，右上，左下，右下
}



//添加车道号
void drawRoadNum(unsigned char* rgbImage, short h, short w, short r, short c, const char* filename)
{
	BITMAPFILEHEADER bf;
	FILE *in = fopen(filename, "rb");
	if (in == NULL)
		return;
	fread(&bf, 1, sizeof(bf), in);
	BITMAPINFOHEADER bi;
	fread(&bi, 1, sizeof(bi), in);

	assert(bf.bfSize < 2000 + bf.bfOffBits);
	char buf[2000];
	fseek(in, bf.bfOffBits, SEEK_SET);
	fread(buf, 1, bf.bfSize - bf.bfOffBits, in);
	fclose(in);

	short h2 = bi.biHeight;
	short w2 = bi.biWidth;
	short ws2 = (w2*3+3)&(~3); 
	short ws = (w*3+3)&(~3); 

	for (int i=0; i<h2; ++i)
		for (int j=0; j<w2; ++j)
		{
			rgbImage[(i+r)*ws+(j+c)*3] = buf[(h2-i-1)*ws2+j*3];
			rgbImage[(i+r)*ws+(j+c)*3+1] = buf[(h2-i-1)*ws2+j*3+1];
			rgbImage[(i+r)*ws+(j+c)*3+2] = buf[(h2-i-1)*ws2+j*3+2];
		}

}

void midValue(unsigned char *rgbImage, short height, short width)
{
	const short ws = (width*3+3) & (~3);
	unsigned char* temp = new unsigned char[width*height*9];
	for (short i=0; i<height; ++i)
	{
		unsigned char* line=rgbImage + ws*i;
		unsigned char* line2=temp + width*9*i;
		for (short j=1; j<width-1; ++j)
		{
			{
				unsigned char a=line[(j-1)*3];
				unsigned char b=line[j*3];
				unsigned char c=line[(j+1)*3];
				unsigned char t;
				t=a;
				if (a>b){
					a=b;
					b=t;
				}
				t=c;
				if (b>c){
					c=b;
					b=t;
					if (b<a){
						b=a;
						a=t;
					}
				}
				line2[j*9+0] = a;
				line2[j*9+1] = b;
				line2[j*9+2] = c;
			}
			{
				unsigned char a=line[(j-1)*3+1];
				unsigned char b=line[j*3+1];
				unsigned char c=line[(j+1)*3+1];
				unsigned char t;
				t=a;
				if (a>b)
				{
					a=b;
					b=t;
				}
				t=c;
				if (b>c)
				{
					c=b;
					b=t;
					if (b<a)
					{
						b=a;
						a=t;
					}
				}
				line2[j*9+3] = a;
				line2[j*9+4] = b;
				line2[j*9+5] = c;
			}
			{
				unsigned char a=line[(j-1)*3+2];
				unsigned char b=line[j*3+2];
				unsigned char c=line[(j+1)*3+2];
				unsigned char t;
				t=a;
				if (a>b)
				{
					a=b;
					b=t;
				}
				t=c;
				if (b>c)
				{
					c=b;
					b=t;
					if (b<a)
					{
						b=a;
						a=t;
					}
				}
				line2[j*9+6] = a;
				line2[j*9+7] = b;
				line2[j*9+8] = c;
			}
		}
	}

	for ( i=1; i<height-1; ++i)
	{
		unsigned char* oldLine1=temp + width*9*(i-1);
		unsigned char* oldLine2=temp + width*9*(i  );
		unsigned char* oldLine3=temp + width*9*(i+1);
		unsigned char* newLine2=rgbImage + ws*i;
		for (short j=1; j<width-1; ++j)
		{
			{
				unsigned char a,b,c;
				{
					unsigned char o = oldLine1[j*9+0];
					unsigned char p = oldLine2[j*9+0];
					unsigned char q = oldLine3[j*9+0];
					a = MAX3(o,q,p);
				}
				{
					unsigned char o = oldLine1[j*9+1];
					unsigned char p = oldLine2[j*9+1];
					unsigned char q = oldLine3[j*9+1];
					b = MID3(o,p,q);
				}
				{
					unsigned char o = oldLine1[j*9+2];
					unsigned char p = oldLine2[j*9+2];
					unsigned char q = oldLine3[j*9+2];
					c = MIN3(o,p,q);
				}
				newLine2[j*3+0] = MID3(a,b,c);
			}
			
			//g
			{
				unsigned char a,b,c;
				{
					unsigned char o = oldLine1[j*9+3];
					unsigned char p = oldLine2[j*9+3];
					unsigned char q = oldLine3[j*9+3];
					a = MAX3(o,q,p);
				}
				{
					unsigned char o = oldLine1[j*9+4];
					unsigned char p = oldLine2[j*9+4];
					unsigned char q = oldLine3[j*9+4];
					b = MID3(o,p,q);
				}
				{
					unsigned char o = oldLine1[j*9+5];
					unsigned char p = oldLine2[j*9+5];
					unsigned char q = oldLine3[j*9+5];
					c = MIN3(o,p,q);
				}
				newLine2[j*3+1] = MID3(a,b,c);
			}
			
			//r	 
			{
				unsigned char a,b,c;
				{
					unsigned char o = oldLine1[j*9+6];
					unsigned char p = oldLine2[j*9+6];
					unsigned char q = oldLine3[j*9+6];
					a = MAX3(o,q,p);
				}
				{
					unsigned char o = oldLine1[j*9+7];
					unsigned char p = oldLine2[j*9+7];
					unsigned char q = oldLine3[j*9+7];
					b = MID3(o,p,q);
				}
				{
					unsigned char o = oldLine1[j*9+8];
					unsigned char p = oldLine2[j*9+8];
					unsigned char q = oldLine3[j*9+8];
					c = MIN3(o,p,q);
				}
				newLine2[j*3+2] = MID3(a,b,c);
			}
		}
	}
	delete[] temp;
}
