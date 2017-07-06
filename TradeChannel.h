// TradeChannel.h
// ����ͨ���ӿڳ�����
#ifndef __TradeChannel_xkejj_234_sjsdfjejnnvbf_xej32212mbj__
#define __TradeChannel_xkejj_234_sjsdfjejnnvbf_xej32212mbj__
struct AccountSumItem
{
	char AcctId[32];
	char UserName[32];
	float TotalCash; // �ֽ�
	float TotalAsset;  // ���ʲ�
	float AvailableFunds; // �����ʽ�
};

class IChannel
{
public:
	IChannel();
	virtual ~IChannel();

	///�����Ʊ�µ�
	///������
	///   stockId -- ��Ʊ����
	///   price -- �۸�
	///   lot -- �µ�������λ���֣�
	virtual int buy(const char* stockId, float price, int lot) = 0;
	
	///������Ʊ�µ�
	///������
	///   stockId -- ��Ʊ����
	///   price -- �۸�
	///   lot -- �µ�������λ���֣�
	virtual int sell(const char* stockId, float price, int lot) = 0;

	///�ڻ����µ�
	///������
	///   instrumentId -- �ڻ���Լid
	///   price -- �۸�
	///   lot -- �µ�������λ���֣�
	virtual int buyopen(const char* instrumentId, float price, int lot) = 0;

	///�ڻ������µ�
	///������
	///   instrumentId -- �ڻ���Լid
	///   price -- �۸�
	///   lot -- �µ�������λ���֣�
	virtual int sellshort(const char* instrumentId, float price, int lot) = 0;

	///�ڻ���ƽ�µ�
	///������
	///   instrumentId -- �ڻ���Լid
	///   price -- �۸�
	///   lot -- �µ�������λ���֣�
	virtual int sell2cover(const char* instrumenId, float price, int lot) = 0;

	///�ڻ���ƽ�µ�
	///������
	///   instrumentId -- �ڻ���Լid
	///   price -- �۸�
	///   lot -- �µ�������λ���֣�
	virtual int buy2cover(const char* instrumentId, float price, int lot) = 0;

	///���뿴����Ȩ
	///������
	///   optionId -- ��Ȩ��Լid
	///   price -- �۸�
	///   lot -- �µ�������λ���֣�
	virtual int longcall(const char* optionId, float price, int lot) = 0;

	///����������Ȩ
	///������
	///   optionId -- ��Ȩ��Լid
	///   price -- �۸�
	///   lot -- �µ�������λ���֣�
	virtual int shortcall(const char* optionId, float price, int lot) = 0;

	///���뿴����Ȩ
	///������
	///   optionId -- ��Ȩ��Լid
	///   price -- �۸�
	///   lot -- �µ�������λ���֣�
	virtual int longput(const char* optionId, float price, int lot) = 0;

	///����������Ȩ
	///������
	///   optionId -- ��Ȩ��Լid
	///   price -- �۸�
	///   lot -- �µ�������λ���֣�
	virtual int shortput(const char* optionId, float price, int lot) = 0;

	///�˻���ѯ
	///����:
	///
	virtual int getAccountSummary(AccountSumItem& qryResult) = 0;

};
#endif