#ifndef _GMM_H_
#define _GMM_H_

#define MOD_COUNT 10

class MgModel
{
	//数值放大100倍， 权重放大10000倍
private:

	//期望，标准差，方差，权
	short avg[MOD_COUNT];
	short s[MOD_COUNT];
	//long s_s[MOD_COUNT];
	short weight[MOD_COUNT];

	//更改权值needModifyWeight次后，再归一化
	char needModifyWeight;

	//新值覆盖查找处
	char writePos; 
	
public:
	MgModel();
	~MgModel();
	bool isBg(unsigned char value);
	unsigned char getValue();
};
#endif
