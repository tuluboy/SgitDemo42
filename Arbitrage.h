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
	// 套利交易类，一个对象代表一个套利组合的交易
	class Arbitrage
	{
	public:
		static cirque<fstech::CThostFtdcDepthMarketDataField> ticks;
		static void initArbi(CMdSpi* pMd, CTradeSpi* pTrd);
		static std::vector<zc::Arbitrage> ArbiTrades; // 可以同时交易多个套利对，目前只有三对
		static std::vector<TradeRecItem> tradeRecord; // 交易记录
		static std::vector<PlannedOrderItem*> ordbook; // 全局送单计划表，子单
		static void UpdateTick(fstech::CThostFtdcDepthMarketDataField& tick);
		static void UpdateParams(); // 查询数据库获取价差等参数
		static void UpdateSubOrdStatus();
		static LONGLONG spin_Locker_ordbook;
		static LONGLONG spin_Locker_arbOrders;
		static bool AutoTradingEnabled;
		static QryInstrumentFB qrInstFB;
		static QryPositionFB qrPosFB;
		static int getArbiPos(CTradeSpi* pTrd); // 查询当前账户可以配对的套利持仓组合
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
		//int __tradeUnit; // 每次交易的套利合约套数（能够配齐的套利单元最小量，比如2手左腿配5手右腿，配齐了为一组）
		ARBI_BALANCE openBalanceMethod; // 左右腿调平方法 1-增仓调平 2-减仓调平
		ARBI_BALANCE closeBalanceMethod; // 平仓时的调平方法
		int getLeftStatus();
		int getRightStatus();
		
		void Send(int unit, TRADE_DIR arb_dir, TRADE_OCFLAG arb_oc);

		int buy(int unit); // 开多,unit-交易单位，priceType-价格类型：1 2 3 4
		int buy(int unit, float basisSpread, int price); // 用基差挂价，到了之后按价格类型送单

		int sell(int unit); // 平多
		int sell(int unit, float basisSpread, int price);

		int sellshort(int unit); // 开空
		int sellshort(int unit, float basisSpread, int price);

		int buy2cover(int unit); // 平空
		int buy2cover(int unit, float basisSpread, int price);

		float cumProfit;
		float cumCommission;
		
		void ProcSubOrd();

		// 超时处理，主要处理超时未成交的瘸腿合约组的处理
		void ProcTimeOut(ArbOrdItem& arbiOrd);

		int ArbiPos; // 套利合约持仓，完整的最小匹配对冲的一对左右腿算一组套利合约，long仓为正，short仓为负
		LEG_TYPE sendFirst; // 根据行情实时计算当前阶段送单应该哪条腿优先
		void getAutoFirst();
		void setSpot(LEG_TYPE lg);
		void setOpenBalance(ARBI_BALANCE blc){ openBalanceMethod = blc; };
		void setCloseBalance(ARBI_BALANCE blc){ closeBalanceMethod = blc; };
	public:
		void UpdateLeftTick(fstech::CThostFtdcDepthMarketDataField& tick); // 触发交易逻辑计算
		void UpdateRightTick(fstech::CThostFtdcDepthMarketDataField& tick); // 触发交易逻辑计算
		volatile LONGLONG spinLock_arbOrders; // arbOrders的锁
		std::vector<ArbOrdItem> arbOrders; // 套利交易母单表
		zc::LEG_TYPE GetCurFirst();
		void UpdateFirst(ArbOrdItem& newOrd); // 确定当前first

		void UpdateArbiOrd();
		void SendSubOrd(); // 更新本套利对的子单
		ArbiInstrument ArbiInst;
		CMdSpi* pMdSpi;
		CTradeSpi* pTrdSpi;
		int SendFlag; // 避免连续送单
		int CloseFlag; // 平仓时避免连续送单
		int priceType; // 1 2 3 4
		ParamRowItem4DbQry tradeParam; // 套利参数
	private:
		int maxTradeUnit; // 交易单位的上限
		void DoTrade();
		int buyLeft(PlannedOrderItem& t); // 买开左腿
		int buyRight(PlannedOrderItem& t); // 买开右腿

		int sellShortLeft(PlannedOrderItem& t); // 卖开左腿
		int sellShortRight(PlannedOrderItem& t); // 卖开右腿

		int sellLeft(PlannedOrderItem& t); // 平多左腿
		int sellRight(PlannedOrderItem& t); // 平多右腿

		int buy2coverLeft(PlannedOrderItem& t); // 平空左腿
		int buy2coverRight(PlannedOrderItem& t); // 平空左腿
	};
}
#endif