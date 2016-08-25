#include <assert.h>
#include "gmm.h"

#define WEIGHT_MODIFY_TIME 20
#define avgweight (10000/MOD_COUNT)
#define initS 1000

#define EXCHANGE(x,y) {int temp=x; x=y; y=temp;}

MgModel::MgModel()
{
	for (int i=0; i<MOD_COUNT; ++i)
	{
		avg[i] = 25500 * i/ MOD_COUNT;
		s[i] = initS;
		weight[i] = 10000/MOD_COUNT;
	}
	needModifyWeight = WEIGHT_MODIFY_TIME;
	writePos = 0;
}
MgModel::~MgModel()
{
}
bool MgModel::isBg(unsigned char valueOld)
{
	const short value = valueOld * 100;
	
	long distanceMin = 1000000;
	long minId = -1;


	for (int i=0; i<MOD_COUNT; ++i)
	{
		//横轴区间（μ-1.96σ，μ+1.96σ）内的面积为95.449974%
		//（μ-2.58σ，μ+2.58σ）内的面积为99.730020%

		short distance = (value - avg[i]) * 100 / s[i];

		//正负
		short flag = distance >= 0 ? 1 : -1;
		//绝对值
		distance /= flag;

		//记录最小值
		if (distance < distanceMin)
		{
			distanceMin = distance;
			minId = i;
		}

		//找到模型，剩余不再计算. 多数属于背景.
		if (distance <= 258)
		{
			//更新方差权值，排序

			//至少5点
			avg[i] += flag*(5 + s[i] * distance / 30000);

			//标准差
			if (distance < 60)
			{
				s[i] -= 10;

				//禁止太小
				if (s[i] < 200)
				{
					s[i] = 200;
				}
			}
			else
			{
				s[i] += 10;
			}
			//s_s[i] = s[i]*s[i];

			bool rst = weight[i] >= avgweight;

			assert(10 == MOD_COUNT);
			weight[i] += 20;
			for (i=0; i<MOD_COUNT; ++i)
			{
				weight[i] -= 2;
			}

			--needModifyWeight;
			if (needModifyWeight <= 0)
			{
				//权总和修正
				int sum = 0;
				for (i=0; i<MOD_COUNT; ++i)
				{
					if (weight[i] < 120)
					{
						weight[i] = 120;
					}
					sum += weight[i];
				}
				for (i=0; i<MOD_COUNT; ++i)
				{
					weight[i] = weight[i] * 10000 / sum;
				}

				//高权值提前
				for (i=0; i<MOD_COUNT-1; ++i)
				{
					if (weight[i+1] > avgweight && weight[i] < weight[i+1])
					{
						EXCHANGE(weight[i], weight[i+1]);
						EXCHANGE(avg[i], avg[i+1]);
						EXCHANGE(s[i], s[i+1]);
					}
				}
				needModifyWeight = WEIGHT_MODIFY_TIME;
			}

			return rst;
		}
		
	}

	//不属于某模型，添加值,轮流覆盖非背景
	for (i=writePos; i< writePos+MOD_COUNT; ++i)
	{
		if (weight[i%MOD_COUNT] <= avgweight)
		{
			writePos = (i+1)%MOD_COUNT;

			avg[i%MOD_COUNT] = value;
			s[i%MOD_COUNT] = initS;
			break;
		}
	}
	
	return false;
}

unsigned char MgModel::getValue()
{
	return (avg[0] + 50) / 100;
}
