// Arbitrage.h
#ifndef __Arbitrage_h_xdkej9358_dkejdkk_djmnvkfgoe0a_sdsfsd45__
#define __Arbitrage_h_xdkej9358_dkejdkk_djmnvkfgoe0a_sdsfsd45__
#include "cirque.h"
#include "../s_include/SgitFtdcMdApi.h"
#include <string>
#include <vector>
#include "common.h"

class CTradeSpi;
class CMdSpi;
namespace zc
{
	// ���������࣬һ���������һ��������ϵĽ���
	class Arbitrage
	{
	public:
		static cirque<fstech::CThostFtdcDepthMarketDataField> ticks;
		static void initArbi(CMdSpi* pMd, CTradeSpi* pTrd);
		static std::vector<zc::Arbitrage> ArbiTrades; // ����ͬʱ���׶�������ԣ�Ŀǰֻ������
		static std::vector<TradeRecItem> tradeRecord; // ���׼�¼
		static std::vector<PlannedOrderItem*> ordbook; // ȫ���͵��ƻ����ӵ�
		static void UpdateTick(fstech::CThostFtdcDepthMarketDataField& tick);
		static void UpdateParams(); // ��ѯ���ݿ��ȡ�۲�Ȳ���
		static void UpdateSubOrdStatus();
		static LONGLONG spin_Locker_ordbook;
		static LONGLONG spin_Locker_arbOrders;
		static bool AutoTradingEnabled;
		static QryInstrumentFB qrInstFB;
		static QryPositionFB qrPosFB;
		static int getArbiPos(CTradeSpi* pTrd); // ��ѯ��ǰ�˻�������Ե������ֲ����
	public:
		Arbitrage(CMdSpi* pMd, CTradeSpi* pTrd);
		~Arbitrage();
	public:
		void Init(const char* leftId, const char* rightId, int leftunit, int rightunit);
		
		void SetMaxTradeUnit(int nMax){ maxTradeUnit = nMax; }
		//void SetLeft(std::string& leftID){ ArbiInst.leftleg = leftID; if (ArbiInst.rightleg.length() > 0)ArbiInst.ArbiInstID = ArbiInst.leftleg + ArbiInst.rightleg; }
		//void SetRight(std::string& rightID){ ArbiInst.rightleg = rightID; if (ArbiInst.leftleg.length() > 0)ArbiInst.ArbiInstID = ArbiInst.leftleg + ArbiInst.rightleg; }
		float getLeftPrice(int priceType, TRADE_DIR dir); // priceType 1 2 3 4
		float getRightPrice(int priceType, TRADE_DIR dir); // priceType 1 2 3 4
		
		void CancelArbi(ArbOrdItem& arbiOrd);
		void procLeftLame(ArbOrdItem& arbiOrd);
		void procRightLame(ArbOrdItem& arbiOrd);
		void procInitSend(ArbOrdItem& ord);
		zc::ARBI_STATUS GetArbiStatus(ArbOrdItem& it);
		//int __tradeUnit; // ÿ�ν��׵�������Լ�������ܹ������������Ԫ��С��������2��������5�����ȣ�������Ϊһ�飩
		ARBI_BALANCE openBalanceMethod; // �����ȵ�ƽ���� 1-���ֵ�ƽ 2-���ֵ�ƽ
		ARBI_BALANCE closeBalanceMethod; // ƽ��ʱ�ĵ�ƽ����
		int getLeftStatus();
		int getRightStatus();
		
		void Send(int unit, TRADE_DIR arb_dir, TRADE_OCFLAG arb_oc);

		int buy(int unit); // ����,unit-���׵�λ��priceType-�۸����ͣ�1 2 3 4
		int buy(int unit, float basisSpread, int price); // �û���Ҽۣ�����֮�󰴼۸������͵�

		int sell(int unit); // ƽ��
		int sell(int unit, float basisSpread, int price);

		int sellshort(int unit); // ����
		int sellshort(int unit, float basisSpread, int price);

		int buy2cover(int unit); // ƽ��
		int buy2cover(int unit, float basisSpread, int price);

		float cumProfit;
		float cumCommission;
		
		void ProcSubOrd();

		// ��ʱ������Ҫ����ʱδ�ɽ���ȳ�Ⱥ�Լ��Ĵ���
		void ProcTimeOut(ArbOrdItem& arbiOrd);

		int ArbiPos; // ������Լ�ֲ֣���������Сƥ��Գ��һ����������һ��������Լ��long��Ϊ����short��Ϊ��
		LEG_TYPE sendFirst; // ��������ʵʱ���㵱ǰ�׶��͵�Ӧ������������
		void getAutoFirst();
		void setSpot(LEG_TYPE lg);
		void setOpenBalance(ARBI_BALANCE blc){ openBalanceMethod = blc; };
		void setCloseBalance(ARBI_BALANCE blc){ closeBalanceMethod = blc; };
	public:
		void UpdateLeftTick(fstech::CThostFtdcDepthMarketDataField& tick); // ���������߼�����
		void UpdateRightTick(fstech::CThostFtdcDepthMarketDataField& tick); // ���������߼�����
		volatile LONGLONG spinLock_arbOrders; // arbOrders����
		std::vector<ArbOrdItem> arbOrders; // ��������ĸ����
		zc::LEG_TYPE GetCurFirst();
		void UpdateFirst(ArbOrdItem& newOrd); // ȷ����ǰfirst

		void UpdateArbiOrd();
		void SendSubOrd(); // ���±������Ե��ӵ�
		ArbiInstrument ArbiInst;
		CMdSpi* pMdSpi;
		CTradeSpi* pTrdSpi;
		int SendFlag; // ���������͵�
		int CloseFlag; // ƽ��ʱ���������͵�
		int priceType; // 1 2 3 4
		ParamRowItem4DbQry tradeParam; // ��������
	private:
		int maxTradeUnit; // ���׵�λ������
		void DoTrade();
		int buyLeft(PlannedOrderItem& t); // ������
		int buyRight(PlannedOrderItem& t); // ������

		int sellShortLeft(PlannedOrderItem& t); // ��������
		int sellShortRight(PlannedOrderItem& t); // ��������

		int sellLeft(PlannedOrderItem& t); // ƽ������
		int sellRight(PlannedOrderItem& t); // ƽ������

		int buy2coverLeft(PlannedOrderItem& t); // ƽ������
		int buy2coverRight(PlannedOrderItem& t); // ƽ������
	};
}
#endif