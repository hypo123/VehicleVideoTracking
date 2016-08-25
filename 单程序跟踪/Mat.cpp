// Mat.cpp: implementation of the Mat class.
//
//////////////////////////////////////////////////////////////////////

#include "Mat.h"
#include <stdio.h>
#include <assert.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Mat::Mat(int row, int col)
{
	this->m_nRow = row;
	m_nCol = col;
	for (int i=0; i<sizeof(m_data)/sizeof(m_data[0]); ++i)
	for (int j=0; j<sizeof(m_data[0])/sizeof(m_data[0][0]); ++j)
	{
		m_data[i][j] = 0;
	}
}

Mat::~Mat()
{

}
void Mat::operator+=(const Mat& m)
{
	assert(this->m_nCol == m.m_nCol);
	assert(m_nRow == m.m_nRow);

	for (int i=0; i<m_nRow; ++i)
	for (int j=0; j<m_nCol; ++j)
	{
		m_data[i][j] += m.m_data[i][j];
	}
}
void Mat::operator-=(const Mat& m)
{
	assert(this->m_nCol == m.m_nCol);
	assert(m_nRow == m.m_nRow);

	for (int i=0; i<m_nRow; ++i)
	for (int j=0; j<m_nCol; ++j)
	{
		m_data[i][j] -= m.m_data[i][j];
	}
}

void Mat::operator*=(const Mat& m)
{
	assert(m_nCol == m.m_nRow);
	double t[4][4] = {0};
	for (int i=0; i<m_nRow; ++i)
		for (int j=0; j<m.m_nCol; ++j)
		{
			//需要多余的空间为0
			t[i][j] += (m_data[i][0]*m.m_data[0][j] +
						m_data[i][1]*m.m_data[1][j] +
						m_data[i][2]*m.m_data[2][j] +
						m_data[i][3]*m.m_data[3][j]
						);
		}
	for (i=0; i<4; ++i)
	for (int j=0; j<4; ++j)
	{
		m_data[i][j] = t[i][j];
	}
	m_nCol = m.m_nCol;
}

void Mat::inverse()
{
	assert(m_nRow == m_nCol);

	double** rows = new double*[m_nRow];
	for (int i=0; i<m_nRow; ++i)
	{
		rows[i] = new double[m_nRow*2];
		for (int j=0; j<m_nRow*2; ++j)
		{
			if (j < m_nRow)
				rows[i][j] = m_data[i][j];
			else if (j == (i+m_nRow))
				rows[i][j] = 1;
			else
				rows[i][j] = 0;
		}
	}
	for (i=0; i<m_nRow; ++i)
	{
		//i行及以下，取i列最大,移到i行
		int maxrowid = i;
		for (int j=i+1; j<m_nRow; ++j)
		{
			if (rows[j][i] > rows[maxrowid][i])
				maxrowid = j;
		}
		double* pD = rows[maxrowid];
		rows[maxrowid] = rows[i];
		rows[i] = pD;

		//rows[i][i] 归1
		{
			double d=rows[i][i];
			for (j=0; j<m_nRow*2; ++j)
			{
				rows[i][j] /= d;
			}
		}

		//其他行i列归0
		for (j=0; j<m_nRow; ++j)
		{
			if (j == i)
				continue;
			if (rows[j][i] > 0.00001 || rows[j][i] < -0.00001)
			{
				double d=rows[j][i];
				for (int k=0; k<m_nRow*2; ++k)
				{
					rows[j][k] /= d;
					rows[j][k] -= rows[i][k];
				}
			}
			rows[j][i] = 0;
		}

	}
	//对角线归1，复制数据
	for (i=0; i<m_nRow; ++i)
	{
		double d = rows[i][i];
		for (int j=0; j<m_nRow; ++j)
		{
			this->m_data[i][j] = rows[i][j+m_nRow] /= d;
		}
		delete[] rows[i];
	}
	delete[] rows;
}
void Mat::t()
{
	int n = m_nRow > m_nCol ? m_nRow : m_nCol;
	for (int i=0; i<n; ++i)
	{
		for (int j=i+1; j<n; ++j)
		{
			double d = m_data[i][j];
			m_data[i][j] = m_data[j][i];
			m_data[j][i] = d;
		}
	}
	n = m_nRow;
	m_nRow = m_nCol;
	m_nCol = n;
}
double* Mat::operator[](int n)
{
	return m_data[n];
}

void Mat::print()
{
	printf("%2d*%2d\n", m_nRow, m_nCol);
	for (int i=0; i<4; ++i)
	{
		for (int j=0; j<4; ++j)
			printf("%f\t\t", m_data[i][j]);
		printf("\n");
	}
	printf("\n");
}

void Mat::operator=(const Mat& m)
{
	this->m_nCol = m.m_nCol;
	this->m_nRow = m.m_nRow;

	for (int i=0; i<4; ++i)
	for (int j=0; j<4; ++j)
	{
		this->m_data[i][j] = m.m_data[i][j];
	}
}