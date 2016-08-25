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
		//�������䣨��-1.96�ң���+1.96�ң��ڵ����Ϊ95.449974%
		//����-2.58�ң���+2.58�ң��ڵ����Ϊ99.730020%

		short distance = (value - avg[i]) * 100 / s[i];

		//����
		short flag = distance >= 0 ? 1 : -1;
		//����ֵ
		distance /= flag;

		//��¼��Сֵ
		if (distance < distanceMin)
		{
			distanceMin = distance;
			minId = i;
		}

		//�ҵ�ģ�ͣ�ʣ�಻�ټ���. �������ڱ���.
		if (distance <= 258)
		{
			//���·���Ȩֵ������

			//����5��
			avg[i] += flag*(5 + s[i] * distance / 30000);

			//��׼��
			if (distance < 60)
			{
				s[i] -= 10;

				//��ֹ̫С
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
				//Ȩ�ܺ�����
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

				//��Ȩֵ��ǰ
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

	//������ĳģ�ͣ����ֵ,�������ǷǱ���
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
