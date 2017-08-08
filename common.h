#ifndef __common_h_xjdkjwe_dkje3433242_dkkjjkdsfa__
#define __common_h_xjdkjwe_dkje3433242_dkkjjkdsfa__
#include "../s_include/SgitFtdcUserApiStruct.h"
#include <functional>
#include <vector>
#include <iostream>

// 订单状态定义：
// 已送 - 表示调用下单接口返回成功，收到交易所回报，表明交易所收到了报单
// 已报 - 收到交易所回报，表明订单已经进入撮合队列等待撮合
// 已成 - 订单全部成交
// 部成 - 订单部分成交

class CTradeSpi;
namespace zc
{
	typedef long long LONGLONG;
	typedef enum TRADE_DIR{ EM_Long = THOST_FTDC_D_Buy, EM_Short = THOST_FTDC_D_Sell };
	typedef enum TRADE_OCFLAG{EM_Open = 3, EM_Close};
	typedef enum LEG_TYPE{ EM_NullLeg, EM_LeftLeg, EM_RightLeg, EM_Auto, EM_BothLegs }; // EM_BothLegs -- 两腿一起发
	typedef enum LEG_STATUS{ EM_LEG_STATUS_NULL, EM_LEG_SENDED, EM_LEG_SENDREADY, EM_LEG_ORDERED, EM_LEG_TRADED, EM_LEG_ParTRADED, EM_LEG_CANCELREADY, EM_LEG_CANCELSENED, EM_LEG_ParCANCELED, EM_LEG_CANCELED };
	// 套利合约状态定义：0-待送 1-已报 2-部成 3-全成 4-部撤 5-全撤 6-平仓 9-null
	typedef enum ARBI_STATUS{ EM_9_9, EM_0_0, EM_1_0, EM_1_1, EM_2_0, EM_3_0, EM_4_0, EM_4_5, EM_2_1, EM_1_2, EM_2_2, EM_4_1, EM_5_0, EM_6_1, EM_6_0, EM_3_5, EM_3_1, EM_1_3,  EM_6_4, EM_6_5, EM_3_2, EM_2_3, EM_3_4, EM_3_6, EM_6_6, EM_3_3, EM_ARB_LONG_OPENED, EM_ARB_LONG_CLOSED, EM_ARB_SHORT_OPENED, EM_ARB_SHORT_CLOSED };
	typedef enum ARBI_COMMAND{ EM_CMD_NULL, EM_SEND_ORD,EM_CANCEL_ORD};
	typedef enum LEG_CONDITION { EM_COND_NULL=9,EM_OK, EM_WAIT, EM_CANCEL }; // init-可以马上送单 wait-等待超时送单 cancel-可以马上撤单 dead-失效，后续不进行任何操作
	typedef enum ARBI_BALANCE{ EM_BL_NULL, EM_BL_CUT, EM_BL_FILL };
	struct ArbOrdItem;
	class Arbitrage;
	struct PlannedOrderItem
	{
		std::string instId;
		std::string exchId;
		std::string ordsysId;
		CTradeSpi* pTrdSpi;
		long ordRef;
		bool isSpot; // 现货为true
		zc::LEG_TYPE leg; // 左右腿
		PlannedOrderItem* brotherLeg; // 另一腿
		fstech::CThostFtdcDepthMarketDataField* Tick;
		ArbOrdItem* arbiOrd; // 本腿来自哪个套利合约
		TRADE_DIR dir; // 买卖方向
		TRADE_OCFLAG ocFlag; // 开平标记
		int priceType; // 1 2 3 4
		int lot; // 送单手数
		int alot; // 未成交
		int elot; // 已成交
		int clot; // 已撤手数;
		double price; // 送单价
		time_t ordCancleSendTime;
		time_t ordCancledTime;
		time_t ordTradedTime; // 成交成功的时刻
		time_t ordOrderedTime; // 本单挂单成功的时刻
		time_t ordSendedTime;
		time_t ordCreatedTime; // 本单的创建时间,相当于计划下达的时间
		time_t curtime; // 当前时间
		int timeout; // 毫秒为单位，超时就撤
		LEG_CONDITION condition; // 0 - 不生效 1-送单条件满足 2-撤单条件满足 3-xxx  99-death单，不会再有其他任何处理，不建议重复使用一个存储位置
		LEG_STATUS status; // 单腿成交状态
		std::function<void(PlannedOrderItem* nextOrder)> orderedHook; // 送单成功后触发操作 -- 可能是送一个其他的单子
		std::function<void(PlannedOrderItem* nextOrder)> tradedHook; // 成交后触发操作 -- 成交完毕后就置 null
		std::function <void(PlannedOrderItem* nextOrder)> cancelHook; // 撤单成功后触发操作
		void setCloseOrd(int thid); // 已成交的开仓单，就地发平仓单，填写相应参数，之前部分成交剩下的全部改为整个订单
		void changeOrdPrc(int inPrc, int thid); // in为正，则将价格改激进，为负，改保守
		float GetMinPoint(int thid);
		float setPrice(int thid);
	};

	struct TradeRecItem
	{
		std::string date;
		std::string time;
		char dir; // 买，卖
		char oc; // 开，平
		int tradeUnit;
		float leftCommision;
		float rightCommision;
	};

	/*
	套利合约有左腿和右腿构成
	开多套利合约：买入左腿，卖出右腿
	平多套利合约：卖出左腿，买入右腿

	开空套利合约：卖出左腿，买入右腿
	平多套利合约：买入左腿，卖出右腿
	*/
	class ArbiInstrument
	{
	public:
		ArbiInstrument()
		{
			init();
		};

		void init()
		{
			leftTick.LastPrice = -1;
			leftTick.AskPrice1 = -1;
			leftTick.BidPrice1 = -1;

			rightTick.LastPrice = -1;
			rightTick.AskPrice1 = -1;
			rightTick.BidPrice1 = -1;

			shortBasisSpread4 = -1; // 左腿挂价，右腿挂价
			shortBasisSpread3 = -1; // 左腿挂价，右腿对价
			shortBasisSpread2 = -1; // 左腿对价，右腿挂价
			shortBasisSpread1 = -1; // 左腿对价，右腿对价

			longBasisSpread1 = -1; // 左腿对价，右腿对价
			longBasisSpread2 = -1; // 左腿对价，右腿挂价
			longBasisSpread3 = -1; // 左腿挂价，右腿对价
			longBasisSpread4 = -1; // 左腿挂价，右腿挂价
		}

		~ArbiInstrument(){};

		void SetInst(const char* leftid, const char* rightid, const int leftunit, const int rightunit)
		{
			setLeftTradeUnit(leftunit);
			setRightTradeUnit(rightunit);
			leftleg = leftid;
			rightleg = rightid;
			ArbiInstID = leftleg + "-" + rightleg;
		}

		void setLeftTradeUnit(int nl){ leftTradeUnit = nl; }
		int getLeftTradeUnit(){ return leftTradeUnit; }

		void setRightTradeUnit(int nr){ rightTradeUnit = nr; }
		int getRightTradeUnit(){ return rightTradeUnit; }

		float getSpreadUpper1(){ return spreadUpper1; }
		float getSpreadUpper2(){ return spreadUpper2; }
		void setSpreadUpper1(float in1){ spreadUpper1 = in1;}
		void setSpreadUpper2(float in2){ spreadUpper2 = in2;}

		float getSpreadLower1(){ return spreadLower1; }
		float getSpreadLower2(){ return spreadLower2; }
		void setSpreadLower1(float in1){ spreadLower1 = in1;}
		void setSpreadLower2(float in2){ spreadLower2 = in2;}

		void UpdateSpread()
		{
			shortBasisSpread4 = rightTick.AskPrice1 - leftTick.BidPrice1; // 左腿挂价，右腿挂价
			longBasisSpread4 = rightTick.BidPrice1 - leftTick.AskPrice1;  // 左腿挂价，右腿挂价

			shortBasisSpread3 = rightTick.BidPrice1 - leftTick.BidPrice1; // 左腿挂价，右腿对价
			longBasisSpread3 = rightTick.AskPrice1 - leftTick.AskPrice1;  // 左腿挂价，右腿对价

			shortBasisSpread2 = rightTick.AskPrice1 - leftTick.AskPrice1; // 左腿对价，右腿挂价
			longBasisSpread2 = rightTick.BidPrice1 - leftTick.BidPrice1;  // 左腿对价，右腿挂价

			shortBasisSpread1 = rightTick.BidPrice1 - leftTick.AskPrice1; // 左腿对价，右腿对价
			longBasisSpread1 = rightTick.AskPrice1 - leftTick.BidPrice1;  // 左腿对价，右腿对价
		}
	public:
		std::string leftleg;  // 合约id
		std::string rightleg; // 合约id
		std::string ArbiInstID; // 合约leftleg+rightleg
		LEG_TYPE spot; // 指定哪一腿是现货
		
		double leftMinPoint;
		double rightMinPoint;

		float shortBasisSpread4; // 左腿挂价，右腿挂价
		float shortBasisSpread3; // 左腿挂价，右腿对价
		float shortBasisSpread2; // 左腿对价，右腿挂价
		float shortBasisSpread1; // 左腿对价，右腿对价

		float longBasisSpread1; // 左腿对价，右腿对价
		float longBasisSpread2; // 左腿对价，右腿挂价
		float longBasisSpread3; // 左腿挂价，右腿对价
		float longBasisSpread4; // 左腿挂价，右腿挂价

		fstech::CThostFtdcDepthMarketDataField leftTick; // 左腿当前行情快照
		time_t leftUpdateTime;
		fstech::CThostFtdcDepthMarketDataField rightTick; // 右腿当前行情快照
		time_t rightUpdateTime;

	private:
		int leftTradeUnit; // 比如2手左腿
		int rightTradeUnit; // 比如5手右腿
		float spreadLower1; // 价差下限1
		float spreadLower2; // 价差下限2
		float spreadUpper1; // 价差上限1
		float spreadUpper2; // 价差上限2
	};

	struct InstumentInfo
	{
		std::string InstrumentId;
		double minpoint;
	};

	struct QryInstrumentFB
	{
		std::vector<InstumentInfo> Insts;
	};

	struct QryPositionFB
	{
		std::vector<fstech::CThostFtdcInvestorPositionField> posList;
	};

	// 套利进场阈值区间
	struct ParamRowItem4DbQry
	{
		std::string code;
		std::string groupname;
		float open_value;
		float close_value;
	};

	extern std::string tradedir(int dir);
};
#endif