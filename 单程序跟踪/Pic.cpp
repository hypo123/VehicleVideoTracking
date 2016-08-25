// Pic.cpp: implementation of the Pic class.
//
//////////////////////////////////////////////////////////////////////

#include "Pic.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Pic::Pic(long height, long width)
{
	h = height;
	w = width;
	ws = (width+3) & (~3);
	data = new unsigned char[ws*h];
	line = new unsigned char*[h];
	for (int i=0; i<h; ++i)
		line[i] = data + ws*i;
	blocks = 0;
}

Pic::~Pic()
{
	delete[] line;
	delete[] data;
	this->clearBlocks();
}
void Pic::clearBlocks()
{
	while (this->blocks != 0)
	{
		PicWhiteBlock* p = blocks;
		blocks = blocks->next;
		delete p;
	}
}

PicWhiteBlock* Pic::addNewBlock()
{
	PicWhiteBlock* p = new PicWhiteBlock();
	p->next = this->blocks;
	this->blocks = p;
	return p;
}

void Pic::inflate()
{
	::memset(line[0], 0, ws);
	::memset(line[h-1], 0, ws);
	for (int i=1; i<h-1; ++i)
	{
		line[i][0] = line[i][w-1] = 0;
	}

	for (i=1; i<h-1; ++i)
	{
		for (int j=1; j<w-1; ++j)
		{
			switch(line[i][j])
			{
			case 0:
				break;
			case 255:
				line[i-1][j-1] |= 255;
				line[i-1][j  ] |= 255;
				line[i-1][j+1] |= 255;
				line[i  ][j-1] |= 255;
				line[i  ][j+1] |= 1;
				line[i+1][j-1] |= 1;
				line[i+1][j  ] |= 1;
				line[i+1][j+1] |= 1;
				break;
			case 1:
				line[i  ][j  ] |= 255;
				break;
			default:
				//����255Ϊǰ��,  ����Ϊ0
				assert(false);
				break;
			}
		}
	}
}

void Pic::corrode()
{
	::memset(line[0], 0, ws);
	::memset(line[h-1], 0, ws);
	for (int i=1; i<h-1; ++i)
	{
		line[i][0] = line[i][w-1] = 0;
	}

	for (i=1; i<h-1; ++i)
	{
		for (int j=1; j<w-1; ++j)
		{
			switch(line[i][j])
			{
			case 0:
				break;
			case 255:
				if (line[i-1][j-1] == 0 || 
					line[i-1][j  ] == 0 || 
					line[i-1][j+1] == 0 ||
					line[i  ][j-1] == 0 ||
					line[i  ][j+1] == 0 ||
					line[i+1][j-1] == 0 ||
					line[i+1][j  ] == 0 ||
					line[i+1][j+1] == 0 )
				{
					//Ӧ�ø�ʴ��
					//���а׵�Ҫʹ�ñ�����Ϣʱ���豣������ ԭ����ɫ ��Ϣ
					if (line[i  ][j+1] == 255 || 
						line[i+1][j-1] == 255 || 
						line[i+1][j  ] == 255 || 
						line[i+1][j+1] == 255)
					{
						line[i][j] = 1;
					}else
						line[i][j] = 0;
				}

				//���ǰ���� ԭ����ɫ ��Ϣ,�������ʹ����.
				if (line[i-1][j-1] == 1)
					line[i-1][j-1] = 0;
				if (line[i-1][j  ] == 1 && line[i  ][j+1] == 0)
					line[i-1][j  ] = 0;
				if (line[i-1][j+1] == 1 && line[i  ][j+1] == 0 && (assert(j+2<w),line[i  ][j+2] == 0))
					line[i-1][j+1] = 0;
				if (line[i  ][j-1] == 1 && (assert(j-2>=0), line[i+1][j-2] == 0) && line[i+1][j-1] == 0 && line[i+1][j  ] == 0)
					line[i  ][j-1] = 0;
				break;
			default:
				assert(false);
				break;
			}
		}
	}
}



void Pic::fillhole()
{
	//ȫͼɨ��һ�飬�ټӱ�Ե��������ɨ�裬
	::memset(line[0], 0, ws);
	::memset(line[h-1], 0, ws);
	for (int i=1; i<h-1; ++i)
	{
		line[i][0] = line[i][w-1] = 0;
	}


	clearBlocks();

	//�մ�0������255

	bool inArea = false;
	for (i=1; i<h-1; ++i)
	{
		for (int j=1; j<w-1; ++j)
		{
			if (!inArea)
			{
				if (line[i][j] == 0)
					continue;
				else if (line[i][j] == 255)
				{
					PicWhiteBlock* pBlock = this->addNewBlock();

					//ɨ�������� ��ʼ�㣬����࿪ʼ��˳ʱ�롣���²�ı�Ե��ǣ�����ʹ�õ���
					const char up=1, right=2,down=4,left=8;
					short posRow=i, posCol=j;//��ǰ����
					char edge = left;	//��ǰ��Ե�ڵ�ǰ�������Ե
					while (true)
					{
						pBlock->addPix(posRow, posCol);
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
							}else if (line[posRow-1][posCol+1] != 0)
							{
								//up
								--posRow;
								++posCol;
								line[posRow][posCol] &= ~left;
								edge = left;
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
								//��������
								line[posRow][posCol] &= ((~right) & (~left));
								edge = left; //�ص����
							}
							break;
						case up:
							if (line[posRow-1][posCol+1] != 0)
							{
								--posRow;
								++posCol;
								line[posRow][posCol] &= ~left;
								edge = left;
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
								--posRow;
								--posCol;
								edge = down;
							}else if (line[posRow-1][posCol] != 0)
							{
								line[posRow][posCol] &= ~left;
								--posRow;
								line[posRow][posCol] &= ~left;
								edge = left;
							}else if (line[posRow-1][posCol+1] != 0)
							{
								line[posRow][posCol] &= ~left;
								--posRow;
								++posCol;
								line[posRow][posCol] &= ~left;
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
								--posRow;
								--posCol;
								edge = down;
							}else if (line[posRow-1][posCol] != 0)
							{
								line[posRow][posCol] &= ~left;
								--posRow;
								line[posRow][posCol] &= ~left;
								edge = left;
							}else if (line[posRow-1][posCol+1] != 0)
							{
								line[posRow][posCol] &= ~left;
								--posRow;
								++posCol;
								line[posRow][posCol] &= ~left;
								edge = left;
							}else if (line[posRow][posCol+1] != 0)
							{
								line[posRow][posCol] &= ~left;
								++posCol;
								edge = up;
							}else if (line[posRow+1][posCol+1] != 0)
							{
								line[posRow][posCol] &= ((~left) & (~right));
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
								//����
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
					}

					//����ǰ����
					if (line[i][j] == (255&(~left)))
					{
						inArea = true;
					}else if (line[i][j] == (255 & ((~8/*left*/)&(2/*right*/))))
					{
					}else
					{
						assert(false);
					}
				}else if (line[i][j] == (255 & (~8/*left*/)))
				{
					inArea = true;
				}else if (line[i][j] == (255 & (~8/*left*/) & (~2/*right*/)))
				{
				}else
				{
					assert(false);
				}

				line[i][j] = 255;
			}
			else
			{
				// ��1 ��2 ��4 ��8
				switch(line[i][j])
				{
				case 0:
					break;
				case 255:
					break;
				case (255 & (~2)):
					inArea = false;
					break;
				default:
					assert(false);
					break;
				}
				line[i][j] = 255;
			}
		}
	}
}

void Pic::midvaluefilter()
{
	::memset(line[0], 0, ws);
	::memset(line[h-1], 0, ws);
	for (int i=1; i<h-1; ++i)
	{
		line[i][0] = line[i][w-1] = 0;
	}

	for (i=1; i<h-1; ++i)
	{
		for (int j=1; j<w-1; ++j)
		{
			unsigned char sort[9]={	line[i-1][j-1], line[i-1][j  ], line[i-1][j+1], 
									line[i  ][j-1], line[i  ][j  ], line[i  ][j+1], 
									line[i+1][j-1], line[i+1][j  ], line[i+1][j+1] };
			for (unsigned char k = 0; k<5; ++k)
			{
				unsigned char max = 0;
				unsigned char maxId = 0;
				for (unsigned char l = 0; l<8-k; ++l)
				{
					if (sort[l] > max)
					{
						max = sort[l];
						maxId = l;
					}
				}
				sort[maxId] = sort[8-k];
				//sort[8-k] = max;
			}
			line[i][j] = sort[4];
			assert(false);
		}
	}
}
