#ifndef _GMM_H_
#define _GMM_H_

#define MOD_COUNT 10

class MgModel
{
	//��ֵ�Ŵ�100���� Ȩ�طŴ�10000��
private:

	//��������׼����Ȩ
	short avg[MOD_COUNT];
	short s[MOD_COUNT];
	//long s_s[MOD_COUNT];
	short weight[MOD_COUNT];

	//����ȨֵneedModifyWeight�κ��ٹ�һ��
	char needModifyWeight;

	//��ֵ���ǲ��Ҵ�
	char writePos; 
	
public:
	MgModel();
	~MgModel();
	bool isBg(unsigned char value);
	unsigned char getValue();
};
#endif
