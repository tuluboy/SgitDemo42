#ifndef __ArbOrdItem_h_ck2e8rfjfhd_fknbcjfror_ei39849_cmndhey__
#define __ArbOrdItem_h_ck2e8rfjfhd_fknbcjfror_ei39849_cmndhey__
#include "common.h"
// 一个订单就是一个状态机
namespace zc
{
	class ArbOrdItem
	{
	public:
		ArbOrdItem();
		~ArbOrdItem();

	public:
		TRADE_DIR dir; // 套利合约的方向
		TRADE_OCFLAG oc;
		PlannedOrderItem* first;
		PlannedOrderItem* second;
		
		int timeout;
		ARBI_STATUS status; // 当前状态
		time_t time0; // 订单生成时间
		int sendDelay; // 发单延时 置0时表示马上发
		int cancelDelay; // 已送的单超出此时间没有成交就撤单
		float entrySpread; // 入场基差
		float exitSpread; // 出厂基差
		float entryCommission; // 入场时交易成本
		float exitCommission; // 出厂时交易成本
		Arbitrage* arbit; // 来自哪个套利合约
		void setLeftFirst();
		void setRightFirst();
		void setBothFirst();
	//private:
		PlannedOrderItem* left;
		PlannedOrderItem* right;
	};
};
#endif