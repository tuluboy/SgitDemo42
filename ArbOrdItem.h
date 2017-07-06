#ifndef __ArbOrdItem_h_ck2e8rfjfhd_fknbcjfror_ei39849_cmndhey__
#define __ArbOrdItem_h_ck2e8rfjfhd_fknbcjfror_ei39849_cmndhey__
#include "common.h"
// һ����������һ��״̬��
namespace zc
{
	class ArbOrdItem
	{
	public:
		ArbOrdItem();
		~ArbOrdItem();

	public:
		TRADE_DIR dir; // ������Լ�ķ���
		TRADE_OCFLAG oc;
		PlannedOrderItem* first;
		PlannedOrderItem* second;
		
		int timeout;
		ARBI_STATUS status; // ��ǰ״̬
		time_t time0; // ��������ʱ��
		int sendDelay; // ������ʱ ��0ʱ��ʾ���Ϸ�
		int cancelDelay; // ���͵ĵ�������ʱ��û�гɽ��ͳ���
		float entrySpread; // �볡����
		float exitSpread; // ��������
		float entryCommission; // �볡ʱ���׳ɱ�
		float exitCommission; // ����ʱ���׳ɱ�
		Arbitrage* arbit; // �����ĸ�������Լ
		void setLeftFirst();
		void setRightFirst();
		void setBothFirst();
	//private:
		PlannedOrderItem* left;
		PlannedOrderItem* right;
	};
};
#endif