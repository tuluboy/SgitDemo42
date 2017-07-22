#include <Windows.h>
#include "Arbitrage.h"
#include <assert.h>
#include "common.h"
#include "TradeSpi.h"
#include <iostream>
#include "ArbOrdItem.h"
#include <algorithm>
#include "dbman.h"
#include "DtTm.h"

extern char * pSubInstrumnet[5];
extern int GetRequsetID();
extern dbman* g_pDb;
namespace zc
{
	extern InstumentInfo& InstruInfo(char* iss);
	// staticc
	cirque<CThostFtdcDepthMarketDataField> Arbitrage::ticks(1000, [](CThostFtdcDepthMarketDataField* des, const CThostFtdcDepthMarketDataField* src){ *des = *src; });
	std::vector<Arbitrage> Arbitrage::ArbiTrades; // 可以同时交易多个套利对
	std::vector<TradeRecItem> Arbitrage::tradeRecord;
	std::vector<PlannedOrderItem*> Arbitrage::ordbook; // 送单计划表
	LONGLONG Arbitrage::spin_Locker_ordbook = 0; // ordbook的锁
	LONGLONG Arbitrage::spin_Locker_arbOrders = 0;
	QryInstrumentFB Arbitrage::qrInstFB;
	QryPositionFB Arbitrage::qrPosFB;
	Arbitrage::Arbitrage(CMdSpi* pMd, CTradeSpi* pTrd) :pMdSpi(pMd), pTrdSpi(pTrd)
	{
		ArbiPosLong = 0; ArbiPosShort = 0;
		SendFlag = 0;
		CloseFlag = 99;
		AutoTradingEnabled = false;
		priceType = 1;
	}

	void Arbitrage::Init(const char* leftId, const char* rightId, int leftunit, int rightunit)
	{
		ArbiInst.SetInst(leftId, rightId, leftunit, rightunit);
		
	}

	int Arbitrage::getArbiPos(CTradeSpi* pTrd)
	{
		// 查持仓，将合约按照对冲比例合成为套利对数
		qrPosFB.posList.clear();
		pTrd->SetQryFB(static_cast<void*>(&qrPosFB));
		CThostFtdcQryInvestorPositionField qrPos;
		memset(&qrPos, 0, sizeof(qrPos));

		//qrPos.BrokerID[0] = '\0';
		//strncpy(qrPos.InvestorID, pTrd->m_loginField.UserID, sizeof(TThostFtdcInvestorIDType));
		int ret = pTrd->m_pReqApi->ReqQryInvestorPosition(&qrPos, GetRequsetID());
		if (ret != 0)
		{
			std::cout << "ReqQryInvestorPosition error!\n";
		}
		Sleep(3000);
		if (qrPosFB.posList.size() < 1)
		{
			std::cout << "qry pos empty!\n";
			return -1;
		}
		for (auto it = qrPosFB.posList.begin(); it != qrPosFB.posList.end(); ++it)
		{
			std::cout << "InstrumentID：" << it->InstrumentID << "  direction: " << (it->PosiDirection == '1' ? "空" : "多") << "  pos: " << it->TodayPosition << "  open cost: " << it->OpenCost << std::endl;
		}

		bool run = true;
		int lastsum = 999999;
		int cursum = 0;
		while (run)
		{
			for (auto jt = ArbiTrades.begin(); jt != ArbiTrades.end(); ++jt)
			{
				fstech::CThostFtdcInvestorPositionField *left(nullptr), *right(nullptr);
				for (auto it = qrPosFB.posList.begin(); it != qrPosFB.posList.end(); ++it)
				{
					if (jt->ArbiInst.leftleg == it->InstrumentID)
					{
						left = &(*it);
					}
					if (jt->ArbiInst.rightleg == it->InstrumentID)
					{
						right = &(*it);
					}
				}
				if (!left || !right)continue;
				if (left->TodayPosition / jt->ArbiInst.getLeftTradeUnit() <= right->TodayPosition / jt->ArbiInst.getRightTradeUnit())
				{
					if (left->TodayPosition / jt->ArbiInst.getLeftTradeUnit() >= 1)
					{
						jt->leftPos = jt->ArbiInst.getLeftTradeUnit();
						jt->rightPos = jt->ArbiInst.getRightTradeUnit();
						left->TodayPosition -= jt->ArbiInst.getLeftTradeUnit();
						right->TodayPosition -= jt->ArbiInst.getRightTradeUnit();
						if (left->PosiDirection == '1')jt->leftPos = -jt->leftPos;
						if (right->PosiDirection == '1')jt->rightPos = -jt->rightPos;
					}
					else continue;
				}
				else
				{
					if (right->TodayPosition / jt->ArbiInst.getRightTradeUnit() >= 1)
					{
						jt->rightPos = jt->ArbiInst.getRightTradeUnit();
						jt->leftPos = jt->ArbiInst.getLeftTradeUnit();
						right->TodayPosition -= jt->ArbiInst.getRightTradeUnit();
						left->TodayPosition -= jt->ArbiInst.getLeftTradeUnit();
						if (left->PosiDirection == '1')jt->leftPos = -jt->leftPos;
						if (right->PosiDirection == '1')jt->rightPos = -jt->rightPos;
					}
					else continue;
				}
				assert(jt->leftPos*jt->rightPos < 0);
				if (jt->leftPos < 0)jt->ArbiPosLong += jt->rightPos / jt->ArbiInst.getRightTradeUnit();
				else jt->ArbiPosShort += jt->rightPos / jt->ArbiInst.getRightTradeUnit();
				std::cout << jt->ArbiInst.ArbiInstID << " ArbiPosLong: " << jt->ArbiPosLong << "  ArbiPosShort: " << jt->ArbiPosShort << std::endl;
				if (jt->ArbiPosLong > 0 || jt->ArbiPosShort < 0) jt->CloseFlag = 0;
			}//for

			for (auto kt = qrPosFB.posList.begin(); kt != qrPosFB.posList.end(); ++kt)
			{
				cursum += kt->TodayPosition;
			}
			if (lastsum > cursum)
			{
				lastsum = cursum;
				cursum = 0;
			}
			if (lastsum == cursum)
			{
				run = false;
			}
		}
		// 怎样将各腿的持仓凑成套利对？
		return 0;
	}

	void Arbitrage::initArbi(CMdSpi* pMd, CTradeSpi* pTrd)
	{
		CThostFtdcQryInstrumentField qryInst;
		memset(&qryInst, 0, sizeof(qryInst));
		pTrd->SetQryFB(static_cast<void*>(&qrInstFB));
		for (int i = 0; i < 5; i++)
		{
			strcpy(qryInst.InstrumentID, pSubInstrumnet[i]);
			pTrd->m_pReqApi->ReqQryInstrument(&qryInst, GetRequsetID());
			Sleep(1500);
		}
		//qrInstFB.Insts.push_back({ "mAu(T+D)", 0.01 });
		if (qrInstFB.Insts.size() < 4)
		{
			std::cout << "qry instrument info error!\n";
			assert(0);
			return;
		}
		else
		{
			std::cout << "query instrumen info ok!...\n";
			for (auto it = qrInstFB.Insts.begin(); it != qrInstFB.Insts.end(); ++it)
			{
				std::cout << "Instrument: " << it->InstrumentId << "  minpoint: " << it->minpoint << std::endl;
				if (abs(it->minpoint-0.01) < 0.001)
				{
					it->minpoint = 0.01;
				}
				if (abs(it->minpoint - 0.05) < 0.001)
				{
					it->minpoint = 0.05;
				}
			}
		}

		Arbitrage arb(pMd, pTrd);
		ArbiTrades.push_back(arb);
		ArbiTrades[0].Init("Ag(T+D)", "ag1712", 15, 1);
		ArbiTrades[0].ArbiInst.setSpreadLower1(-9999);
		ArbiTrades[0].ArbiInst.setSpreadUpper1(9998);
		ArbiTrades[0].ArbiInst.setSpreadLower2(-9998);
		ArbiTrades[0].ArbiInst.setSpreadUpper2(9999);

		ArbiTrades[0].sendFirst = LEG_TYPE::EM_RightLeg;
		ArbiTrades[0].setSpot(LEG_TYPE::EM_LeftLeg);
		ArbiTrades[0].setOpenBalance(ARBI_BALANCE::EM_BL_FILL); // 开仓采用增仓调平
		ArbiTrades[0].setCloseBalance(ARBI_BALANCE::EM_BL_CUT); // 开仓采用减仓调平

		ArbiTrades.push_back(arb);
		ArbiTrades[1].Init("Au(T+D)", "au1712", 1, 1);
		ArbiTrades[1].ArbiInst.setSpreadLower1(-9999);
		ArbiTrades[1].ArbiInst.setSpreadUpper1(9998);
		ArbiTrades[1].ArbiInst.setSpreadLower1(-9998);
		ArbiTrades[1].ArbiInst.setSpreadUpper1(9999);
		ArbiTrades[1].sendFirst = LEG_TYPE::EM_RightLeg;
		ArbiTrades[1].setSpot(LEG_TYPE::EM_LeftLeg);
		ArbiTrades[1].setOpenBalance(ARBI_BALANCE::EM_BL_FILL); // 开仓采用增仓调平
		ArbiTrades[1].setCloseBalance(ARBI_BALANCE::EM_BL_CUT); // 开仓采用减仓调平

		ArbiTrades.push_back(arb);
		ArbiTrades[2].Init("mAu(T+D)", "au1712", 10, 1);
		ArbiTrades[2].ArbiInst.setSpreadLower1(-9999);
		ArbiTrades[2].ArbiInst.setSpreadUpper1(9998);
		ArbiTrades[2].ArbiInst.setSpreadLower1(-9998);
		ArbiTrades[2].ArbiInst.setSpreadUpper1(9999);
		ArbiTrades[2].sendFirst = LEG_TYPE::EM_RightLeg;
		ArbiTrades[2].setSpot(LEG_TYPE::EM_LeftLeg);
		ArbiTrades[2].setOpenBalance(ARBI_BALANCE::EM_BL_FILL); // 开仓采用增仓调平
		ArbiTrades[2].setCloseBalance(ARBI_BALANCE::EM_BL_CUT); // 开仓采用减仓调平

		for (auto it = ArbiTrades.begin(); it != ArbiTrades.end(); ++it)
		{
			for (auto jt = qrInstFB.Insts.begin(); jt != qrInstFB.Insts.end(); ++jt)
			{
				if (jt->InstrumentId == it->ArbiInst.leftleg)
				{
					it->ArbiInst.leftMinPoint = jt->minpoint;
				}
				else if (jt->InstrumentId == it->ArbiInst.rightleg)
				{
					it->ArbiInst.rightMinPoint = jt->minpoint;
				}
			}
		}
		// 查询持仓，组成套利对
		getArbiPos(pTrd);
	}

	// 静态函数，被orderFunc调用轮询订单状态，更新订单状态
	void Arbitrage::UpdateSubOrdStatus()
	{
		// 已经执行的子订单状态获取？？？？TraderSpi对象负责更新
		// 作废，直接由TraderSpi更新orderbook
	}

	void Arbitrage::UpdateTick(CThostFtdcDepthMarketDataField& tick)
	{
		// 分发tick到各个套利合约
		if ('A' == tick.InstrumentID[0])
		{
			if ('g' == tick.InstrumentID[1])
			{
				ArbiTrades[0].UpdateLeftTick(tick);
			}
			else
			{
				// u
				ArbiTrades[1].UpdateLeftTick(tick);
			}
		}
		else if ('a' == tick.InstrumentID[0])
		{
			if ('g' == tick.InstrumentID[1])
			{
				ArbiTrades[0].UpdateRightTick(tick);
			}
			else
			{
				// u
				ArbiTrades[1].UpdateRightTick(tick);
				ArbiTrades[2].UpdateRightTick(tick);
			}
		}
		else
		{
			ArbiTrades[2].UpdateLeftTick(tick);
		}
	}

	Arbitrage::~Arbitrage()
	{
	}

	void Arbitrage::getAutoFirst()
	{
	}

	void Arbitrage::setSpot(zc::LEG_TYPE leg)
	{
		ArbiInst.spot = leg;
	}

	void Arbitrage::UpdateLeftTick(fstech::CThostFtdcDepthMarketDataField& tick)
	{
		ArbiInst.leftTick = tick;
		ArbiInst.UpdateSpread();
		if (zc::Arbitrage::AutoTradingEnabled)DoTrade();
	}

	void Arbitrage::UpdateRightTick(fstech::CThostFtdcDepthMarketDataField& tick)
	{
		ArbiInst.rightTick = tick;
		ArbiInst.UpdateSpread();
		if (zc::Arbitrage::AutoTradingEnabled)DoTrade();
	}

	// 触发交易逻辑计算
	void Arbitrage::DoTrade()
	{
		if (doTradeCount++ % 100 == 0) std::cout << ArbiInst.ArbiInstID<< " Auto trade is running\n";
		// 两腿都要有数据才执行交易
		if (ArbiInst.leftTick.LastPrice < 0 || ArbiInst.rightTick.LastPrice < 0) return;
		int curtime = DtTm::Parse(ArbiInst.leftTick.UpdateTime);
		
		bool openTimeCond = true || (curtime < 91600 || curtime>210000);

		if (curtime > 190000&& curtime < 210005) // 跨交易日初始化
		{
			// 新交易日初始化
			ArbiInst.leftTick.LastPrice = -1;
			ArbiInst.leftTick.AskPrice1 = -1;
			ArbiInst.leftTick.BidPrice1 = -1;

			ArbiInst.rightTick.LastPrice = -1;
			ArbiInst.rightTick.AskPrice1 = -1;
			ArbiInst.rightTick.BidPrice1 = -1;

			SendFlag = 0;
			CloseFlag = 99;
		}
		else if (curtime > 145900 && curtime < 160000)
		{
			// 平仓
			if (ArbiPosLong > 0)
			{
				sellshort(ArbiPosLong, 30);
			}
			if (ArbiPosShort < 0)
			{
				buy2cover(-ArbiPosShort, 30);
			}
			return;
		}
		else if (curtime > 143000 && curtime <= 145900)
		{
			if (ArbiPosLong > 0)
			{
				if (ArbiInst.shortBasisSpread1 - avgLongEntrySpread > 0.7)
				{
					sell(ArbiPosLong, 60);
				}
			}
			if (ArbiPosShort < 0)
			{
				if (avgShortEntrySpread - ArbiInst.longBasisSpread1 > 0.7)
				{
					buy2cover(-ArbiPosShort, 60);
				}
			}
			return;
		}
		// 套利逻辑
		// 积极策略，执行1类价差策略
		// 9:16:00之后只平仓不开仓
		if (openTimeCond && ArbiInst.shortBasisSpread1 > ArbiInst.getSpreadUpper2())
		{
			if (0 == SendFlag)
			{
				std::cout << "跳空同时满足2个开仓条件,直接送2手...\n";
				sellshort(2,60); // 直接送2手
				SendFlag = 2;
				CloseFlag = 0;
			}
			else if (1 == SendFlag)
			{
				std::cout << "满足开仓条件2，再送1手...\n";
				sellshort(1, 60);
				SendFlag = 2;
			}
		}
		else if (openTimeCond && ArbiInst.shortBasisSpread1 > ArbiInst.getSpreadUpper1())
		{
			// 目前只有正套 if (ArbiPos > 0) sell(ArbiPos);
			if (0 == SendFlag)
			{
				std::cout << "满足开仓条件1，送1手...\n";
				sellshort(1, 60);
				SendFlag = 1;
				CloseFlag = 0;
			}
		}
		else if (ArbiPosShort < 0 && ArbiInst.longBasisSpread1 < ArbiInst.getSpreadLower1())
		{
			if (0 == CloseFlag)
			{
				std::cout << "跳空满足2个平仓条件,直接送2手平仓...\n";
				buy2cover(-ArbiPosShort, 60); // 全平掉
				CloseFlag = 2;
				SendFlag = 0;
			}
			else if (1 == CloseFlag)
			{
				std::cout << "满足平仓条件1，再送1手...\n";
				buy2cover(1, 60);
				CloseFlag = 2;
			}
		}
		else if (ArbiPosShort < 0 && ArbiInst.longBasisSpread1 < ArbiInst.getSpreadLower2())
		{
			if (0 == CloseFlag)
			{
				std::cout << "满足平仓条件1，送1手...\n";
				buy2cover(1, 60);
				CloseFlag = 1;
				SendFlag = 0;
			}
		}
	}

	void Arbitrage::Send(int unit, TRADE_DIR arb_dir, TRADE_OCFLAG arb_oc, int nTimeOut)
	{
		while (InterlockedExchange64(&spin_Locker_ordbook, TRUE))
		{
			//std::cout << "Send blocked In spin_Locker_ordbook............\n";
			Sleep(0);
		}
		//while (InterlockedExchange64(&spin_Locker_arbOrders, TRUE)){ Sleep(0); }
		arbOrders.push_back(ArbOrdItem()); // 需要加锁
		ArbOrdItem& newOrd = arbOrders.back();
		newOrd.dir = arb_dir;
		ordbook.push_back(new PlannedOrderItem);
		newOrd.left = ordbook.back();
		ordbook.push_back(new PlannedOrderItem);
		newOrd.right = ordbook.back();
		time_t curTime = zc::GetCurTime();
		newOrd.left->ordCreatedTime = curTime;
		newOrd.right->ordCreatedTime = curTime;
		newOrd.left->priceType = priceType;
		newOrd.right->priceType = priceType;

		newOrd.left->instId = ArbiInst.leftleg;
		newOrd.right->instId = ArbiInst.rightleg;

		newOrd.left->leg = LEG_TYPE::EM_LeftLeg;
		newOrd.right->leg = LEG_TYPE::EM_RightLeg;

		newOrd.left->isSpot = (ArbiInst.spot == LEG_TYPE::EM_LeftLeg);
		newOrd.right->isSpot = (ArbiInst.spot == LEG_TYPE::EM_RightLeg);

		newOrd.left->timeout = nTimeOut;
		newOrd.right->timeout = nTimeOut;

		newOrd.left->condition = LEG_CONDITION::EM_COND_NULL;
		newOrd.right->condition = LEG_CONDITION::EM_COND_NULL;

		newOrd.left->pTrdSpi = zc::Arbitrage::pTrdSpi;
		newOrd.right->pTrdSpi = zc::Arbitrage::pTrdSpi;
		newOrd.left->brotherLeg = newOrd.right;
		newOrd.right->brotherLeg = newOrd.left;
		newOrd.left->arbiOrd = &newOrd;
		newOrd.right->arbiOrd = &newOrd;
		newOrd.left->Tick = &ArbiInst.leftTick;
		newOrd.right->Tick = &ArbiInst.rightTick;

		newOrd.arbit = this;

		newOrd.status = ARBI_STATUS::EM_0_0;

		UpdateFirst(newOrd); // 确定first second 和 left right的映射关系
		switch (arb_dir)
		{
		case zc::TRADE_DIR::EM_Long:
			newOrd.left->dir = zc::TRADE_DIR::EM_Short;
			newOrd.right->dir = zc::TRADE_DIR::EM_Long;
			break;
		case zc::TRADE_DIR::EM_Short:
			newOrd.left->dir = zc::TRADE_DIR::EM_Long;
			newOrd.right->dir = zc::TRADE_DIR::EM_Short;
			break;
		}
		newOrd.left->ocFlag = arb_oc;
		newOrd.right->ocFlag = arb_oc;

		newOrd.left->setPrice(); // = getLeftPrice(priceType, TRADE_DIR::EM_Short);
		newOrd.right->setPrice(); //= getRightPrice(priceType, TRADE_DIR::EM_Long);
		newOrd.left->price = zc::round2minpoint(newOrd.left->price, ArbiInst.leftMinPoint);
		newOrd.right->price = zc::round2minpoint(newOrd.right->price, ArbiInst.rightMinPoint);

		newOrd.left->lot = unit * ArbiInst.getLeftTradeUnit();
		newOrd.left->alot = newOrd.left->lot;
		newOrd.left->elot = 0;
		newOrd.left->clot = 0;

		newOrd.right->lot = unit * ArbiInst.getRightTradeUnit();
		newOrd.right->alot = newOrd.right->lot;
		newOrd.right->elot = 0;
		newOrd.right->clot = 0;

		newOrd.sendDelay = 0;
		newOrd._timeout = 50; // 5秒不成就撤单

		if (LEG_TYPE::EM_LeftLeg == sendFirst)newOrd.setLeftFirst(); // initArbi中被配置
		else if (LEG_TYPE::EM_RightLeg == sendFirst)newOrd.setRightFirst();
		else if (LEG_TYPE::EM_BothLegs == sendFirst)newOrd.setBothFirst();
		
		std::cout << "right ask: " << ArbiInst.rightTick.AskPrice1 << " right bid: " << ArbiInst.rightTick.BidPrice1 << std::endl;
		std::cout << "left ask: " << ArbiInst.leftTick.AskPrice1 << " left bid: " << ArbiInst.leftTick.BidPrice1 << std::endl;
		std::cout << "longSpread1:" << ArbiInst.longBasisSpread1 << " shortSpread1:" << ArbiInst.shortBasisSpread1 << std::endl;
		std::cout << "*****************************************************************\n";
		//InterlockedExchange64(&spin_Locker_arbOrders, FALSE);
		InterlockedExchange64(&spin_Locker_ordbook, FALSE);
	}

	// 即时开多,unit-交易单位，
	// priceType-价格类型：1 2 3 4
	// 价格类型代表了执行模式，比如1：左腿对价右腿对价，下单时也左腿下对价，右腿下对价
	// 事物式执行vs异步执行
	// 事物式执行----等待两腿都执行完毕再返回，下单时给定一个等待时间
	// 异步执行----填充下单计划表后直接返回，具体两腿是否成交完毕，交由监控线程按下单计划表处理
	int Arbitrage::buy(int unit, int nTimeOut)
	{
		// 基差计算模式：右腿-左腿
		// 做多基差--买入右腿，卖出左腿
		// 右腿活跃
		/*while (InterlockedExchange64(&spinLock_arbOrders, TRUE))
		{
		Sleep(0);
		}tickFunc线程调用，*/
		Send(unit, zc::TRADE_DIR::EM_Long, zc::TRADE_OCFLAG::EM_Open, nTimeOut);
		std::cout << "buy a pair at:" << ArbiInst.longBasisSpread1 << " " << unit << std::endl;
		return 0;
	}

	// 等待挂价开多，两腿都等待，直到某种基差达到限价就转入即时开单
	int Arbitrage::buy(int unit, float basisSpread, int price) // 用基差挂价，到了之后按价格类型送单
	{

		return 0;
	}

	// 即时平多
	int Arbitrage::sell(int unit, int nTimeOut)
	{
		if (ArbiPosLong>0)Send(unit, zc::TRADE_DIR::EM_Short, zc::TRADE_OCFLAG::EM_Close, nTimeOut);
		else std::cout << "平多仓位不足\n";
		return 0;
	}

	// 等待挂价平多，两腿都等待
	int Arbitrage::sell(int unit, float basisSpread, int price)
	{
		return 0;
	}

	int Arbitrage::sellshort(int unit, int nTimeOut) // 开空
	{
		Send(unit, zc::TRADE_DIR::EM_Short, zc::TRADE_OCFLAG::EM_Open, nTimeOut);
		std::cout << "short a pair at:" << ArbiInst.shortBasisSpread1 << " " << unit << std::endl;
		return 0;
	}

	int Arbitrage::sellshort(int unit, float basisSpread, int price)
	{
		return 0;
	}

	int Arbitrage::buy2cover(int unit, int nTimeOut) // 平空
	{
		if (ArbiPosShort < 0)Send(unit, zc::TRADE_DIR::EM_Long, zc::TRADE_OCFLAG::EM_Close, nTimeOut);
		else std::cout << "平空仓位不足！\n";
		std::cout << "close a short pair at:" << ArbiInst.longBasisSpread1 << " " << unit << std::endl;
		return 0;
	}

	int Arbitrage::buy2cover(int unit, float basisSpread, int price)
	{
		return 0;
	}

	// private
	int Arbitrage::buyLeft(PlannedOrderItem& t)
	{

		return 0;
	}

	int Arbitrage::sellShortLeft(PlannedOrderItem& t)
	{
		return 0;
	}

	// dir:具体腿的方向
	float Arbitrage::getLeftPrice(int prcType, TRADE_DIR dir)
	{
		switch (prcType)
		{
		case 1:// 左腿对价，右腿对价
		case 2:// 左腿对价，右腿挂价
			return TRADE_DIR::EM_Long == dir ? ArbiInst.leftTick.AskPrice1 : ArbiInst.leftTick.BidPrice1;
			break;
		case 3:// 左腿挂价，右腿对价
		case 4:// 左腿挂价，右腿挂价
			return TRADE_DIR::EM_Long == dir ? ArbiInst.leftTick.BidPrice1 : ArbiInst.leftTick.AskPrice1;
			break;
		}
	}

	float Arbitrage::getRightPrice(int prcType, TRADE_DIR dir)
	{
		switch (prcType)
		{
		case 1:// 左腿对价，右腿对价
		case 3:// 左腿挂价，右腿对价
			return TRADE_DIR::EM_Long == dir ? ArbiInst.rightTick.AskPrice1 : ArbiInst.rightTick.BidPrice1;
			break;
		case 2:// 左腿对价，右腿挂价
		case 4:// 左腿挂价，右腿挂价
			return TRADE_DIR::EM_Long == dir ? ArbiInst.rightTick.BidPrice1 : ArbiInst.rightTick.AskPrice1;
			break;
		}
		return -1;
	}

	zc::LEG_TYPE Arbitrage::GetCurFirst()
	{
		return zc::LEG_TYPE::EM_RightLeg;
	}

	void Arbitrage::UpdateFirst(ArbOrdItem& newOrd)
	{
		zc::LEG_TYPE curFirst;
		if (zc::LEG_TYPE::EM_Auto == sendFirst)
		{
			curFirst = GetCurFirst(); // 根据实际情况确定当前先送哪条腿
			if (zc::LEG_TYPE::EM_LeftLeg == curFirst)newOrd.setLeftFirst();
			else if (zc::LEG_TYPE::EM_RightLeg == curFirst)newOrd.setRightFirst();
			else std::cout << "get auto first leg error!\n";
		}
	}

#define ARBISWT(x) case ARBI_STATUS::##x: res = #x;break
	// ARBI_STATUS
	std::string Arbi_Status2Str(int st)
	{
		std::string res;
		switch (st)
		{//EM_9_9, EM_0_0, EM_1_0, EM_1_1, EM_2_0, EM_3_0, EM_4_0, EM_4_5, EM_2_1, EM_1_2, EM_2_2, 
			//EM_4_1, EM_5_0, EM_6_1, EM_6_0, EM_3_5, EM_3_1, EM_1_3,  EM_6_4, EM_6_5, EM_3_2, EM_2_3, EM_3_4, EM_3_6,
			//EM_6_6, EM_3_3, EM_ARB_LONG_OPENED, EM_ARB_LONG_CLOSED, EM_ARB_SHORT_OPENED, EM_ARB_SHORT_CLOSED
			ARBISWT(EM_9_9);
			ARBISWT(EM_0_0);
			ARBISWT(EM_1_0);
			ARBISWT(EM_1_1);
			ARBISWT(EM_2_0);
			ARBISWT(EM_3_0);
			ARBISWT(EM_4_0);
			ARBISWT(EM_4_5);
			ARBISWT(EM_2_1);
			ARBISWT(EM_1_2);
			ARBISWT(EM_2_2);
			ARBISWT(EM_4_1);
			ARBISWT(EM_5_0);
			ARBISWT(EM_6_1);
			ARBISWT(EM_6_0);
			ARBISWT(EM_3_5);
			ARBISWT(EM_3_1);
			ARBISWT(EM_1_3);
			ARBISWT(EM_6_4);
			ARBISWT(EM_6_5);
			ARBISWT(EM_3_2);
			ARBISWT(EM_2_3);
			ARBISWT(EM_3_4);
			ARBISWT(EM_3_6);
			ARBISWT(EM_6_6);
			ARBISWT(EM_3_3);
		}
		return res;
	}

	// 打印跳转状态
#define DbgLog std::cout << "\ntrans to status:"<<Arbi_Status2Str(it->status)<<std::endl

	zc::ARBI_STATUS Arbitrage::GetArbiStatus(zc::ArbOrdItem& it)
	{
		return (TRADE_DIR::EM_Long == it.dir ? (TRADE_OCFLAG::EM_Open == it.oc ? ARBI_STATUS::EM_ARB_LONG_OPENED : ARBI_STATUS::EM_ARB_LONG_CLOSED) : (TRADE_OCFLAG::EM_Open == it.oc ? ARBI_STATUS::EM_ARB_SHORT_OPENED : ARBI_STATUS::EM_ARB_SHORT_CLOSED));
	}

	// 刷新套利母单的状态
	void Arbitrage::UpdateArbiOrd()
	{
		//std::cout << "Into UpdateArbiOrd...\n";
		// 套利合约状态定义：0-待送 1-已报 2-部成 3-全成 4-部撤 5-全撤 6-平仓
		// 查询左右腿成交状态，数据从Arbitrage::ordbook中获取
		// 母单状态：未送，已送未成，右成左瘸，左成右瘸，全成
		// 条件：超时，部分成交，全部成交
		// 动作：送单，撤单
		// 更新系统时间
		// 单腿处理-消极模式-若一腿未成，撤未成，平已成
		// 单腿处理-积极模式-若一腿未成，撤未成，加价再送

		time_t curtime = GetCurTime();
		while (InterlockedExchange64(&(spin_Locker_arbOrders), TRUE)){ Sleep(0); }
		//std::cout << "Into InterlockedExchange64..spin_Locker_arbOrders....\n";
		for (auto it = arbOrders.begin(); it != arbOrders.end(); ++it)
		{
			//std::cout << Arbi_Status2Str(it->status);
			switch (it->status)
			{
			case ARBI_STATUS::EM_0_0:
				// 初始态，送单
				std::cout << "status init: EM_0_0......\n"; 
				//UpdateFirst(*it);  下单时已经确定好了
				// 事件处理
				if (0 == it->sendDelay)
				{
					// 马上发
					if (LEG_TYPE::EM_BothLegs == sendFirst)
					{
						// 两腿同时发
						it->status = zc::ARBI_STATUS::EM_1_1;
						DbgLog;
						break;
					}
					else
					{
						// 单腿
						it->first->condition = LEG_CONDITION::EM_OK;
						it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
						it->second->condition = LEG_CONDITION::EM_WAIT;
						it->second->status = LEG_STATUS::EM_LEG_STATUS_NULL;
						it->status = zc::ARBI_STATUS::EM_1_0;
						DbgLog;
						break;
					}
				}
				else if (curtime - it->time0 > it->sendDelay)
				{
					// 延时单的时间到
					it->first->condition = LEG_CONDITION::EM_OK;
					it->second->condition = LEG_CONDITION::EM_WAIT;
					it->status = zc::ARBI_STATUS::EM_1_0;
					DbgLog;
					break;
				}
				break;
			case ARBI_STATUS::EM_1_1:
				// 先简化版本直接两腿成交
				if (LEG_STATUS::EM_LEG_TRADED == it->first->status  && LEG_STATUS::EM_LEG_TRADED == it->first->status)
				{
					it->status = ARBI_STATUS::EM_3_3;
					DbgLog;
					break;
				}
				else if (LEG_STATUS::EM_LEG_TRADED != it->first->status && LEG_STATUS::EM_LEG_TRADED != it->second->status)
				{
					if (LEG_STATUS::EM_LEG_ParTRADED == it->first->status && LEG_STATUS::EM_LEG_ParTRADED != it->second->status)
					{
						it->status = ARBI_STATUS::EM_2_1;
					}
					else if (LEG_STATUS::EM_LEG_ParTRADED != it->first->status && LEG_STATUS::EM_LEG_ParTRADED == it->second->status)
					{
						it->status = ARBI_STATUS::EM_1_2;
					}
					else if (LEG_STATUS::EM_LEG_ParTRADED == it->first->status && LEG_STATUS::EM_LEG_ParTRADED == it->second->status)
					{
						it->status = ARBI_STATUS::EM_2_2;
					}
					else
					{
						// 还是 EM_!_1;
					}
					DbgLog;
					break;
				}
				else if (LEG_STATUS::EM_LEG_TRADED == it->first->status && LEG_STATUS::EM_LEG_TRADED != it->second->status)
				{
					// 先腿成交后腿未成
					if (LEG_STATUS::EM_LEG_ParTRADED == it->second->status)
					{
						it->status = ARBI_STATUS::EM_3_2;
					}
					else
					{
						it->status = ARBI_STATUS::EM_3_1;
					}
				}
				else
				{
					// 先腿未成后腿成交
					if (LEG_STATUS::EM_LEG_ParTRADED == it->first->status)
					{
						it->status = ARBI_STATUS::EM_2_3;
					}
					else
					{
						it->status = ARBI_STATUS::EM_1_3;
					}
				}
				DbgLog;
				break;
			case ARBI_STATUS::EM_1_0:
				// 从EM_0_0跳转而来，如果在一定超时范围内没有到达，要处理异常
				if (LEG_STATUS::EM_LEG_SENDED == it->first->status)
				{
					if (curtime - it->first->ordSendedTime > it->first->timeout)
					{
						// 出现废单
						std::cout << "sended order timeout error....\n";
						it->first->condition = LEG_CONDITION::EM_COND_NULL;
						it->second->condition = LEG_CONDITION::EM_COND_NULL;
						it->status = ARBI_STATUS::EM_9_9;
						DbgLog;
					}
					break;
				}

				if (LEG_STATUS::EM_LEG_ORDERED == it->first->status)
				{
					// 挂单超时未成交
					if (curtime - it->first->ordOrderedTime > it->first->timeout)
					{
						it->first->condition = LEG_CONDITION::EM_CANCEL;
						it->first->status = LEG_STATUS::EM_LEG_CANCELREADY;
						it->second->condition = LEG_CONDITION::EM_COND_NULL;
						it->second->status = LEG_STATUS::EM_LEG_STATUS_NULL;
						it->status = ARBI_STATUS::EM_5_0;
						DbgLog;
					}
					break;
				}

				// 单腿已送，查是否已送成
				if (zc::LEG_STATUS::EM_LEG_ParTRADED == it->first->status)
				{
					// 部分成交
					it->status = EM_2_0;
					DbgLog;
					break;
				}
				else if (LEG_STATUS::EM_LEG_TRADED == it->first->status)
				{
					// 两腿的单子已经都写入了ordbook，另一腿直接修改状态即可
					it->second->condition = LEG_CONDITION::EM_OK;
					it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->status = EM_3_1; // EM_3_0 EM_3_1合并
					DbgLog;
					break;
				}
				break;
			case ARBI_STATUS::EM_2_0:
				if (LEG_STATUS::EM_LEG_TRADED == it->first->status)
				{
					it->second->condition = LEG_CONDITION::EM_OK;
					it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->status = ARBI_STATUS::EM_3_1;
					DbgLog;
					break;
				}

				// 超时未成
				if (LEG_STATUS::EM_LEG_ParTRADED == it->first->status)
				{
					if (curtime - it->first->ordOrderedTime > it->first->timeout)
					{
						it->first->condition = LEG_CONDITION::EM_CANCEL;
						it->first->status = LEG_STATUS::EM_LEG_CANCELREADY;
						it->status = ARBI_STATUS::EM_4_0;
						DbgLog;
					}
					break;
				}
				break;
			case ARBI_STATUS::EM_3_1:
				if (LEG_STATUS::EM_LEG_SENDED == it->second->status)
				{
					if (curtime - it->second->ordSendedTime > it->second->timeout)
					{
						// 出现废单，要引起重视，避免反复开单腿后平仓，造成大量小额亏损交易以及手续费损失
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						it->second->condition = LEG_CONDITION::EM_COND_NULL; // 要不要送一个撤单指令？
						//it->first->condition = LEG_CONDITION::EM_OK;
						it->status = ARBI_STATUS::EM_3_5; // 废单也认为是被撤单，系统自动撤单
						DbgLog;
					}
					break;
				}

				if (LEG_STATUS::EM_LEG_ORDERED == it->second->status)
				{
					// 挂单超时未成交
					if (curtime - it->second->ordOrderedTime > it->second->timeout)
					{
						it->second->condition = LEG_CONDITION::EM_CANCEL;
						it->second->status = LEG_STATUS::EM_LEG_CANCELREADY;
						it->status = ARBI_STATUS::EM_3_5;
						DbgLog;
					}
					break;
				}

				if (LEG_STATUS::EM_LEG_ParTRADED == it->second->status)
				{
					it->status = ARBI_STATUS::EM_3_2;
					DbgLog;
					break;
				}
				else if (LEG_STATUS::EM_LEG_TRADED == it->second->status)
				{
					it->status = ARBI_STATUS::EM_3_3;
					DbgLog;
					break;
				}
				break;
			case ARBI_STATUS::EM_3_5:
				if (LEG_STATUS::EM_LEG_CANCELED == it->second->status)
				{
					// 单腿处理-消极模式-若一腿未成，撤未成，平已成
					// 单腿处理-积极模式-若一腿未成，撤未成，加价再送
					// 第二腿超时未成交，撤单成功，平掉第一腿
					if (TRADE_OCFLAG::EM_Open == it->oc)
					{
						it->first->setCloseOrd(); // 就地转平仓
						it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
						it->first->condition = LEG_CONDITION::EM_OK;
						it->status = ARBI_STATUS::EM_6_5;
					}
					else // 平仓时用激进模式
					{
						it->second->changeOrdPrc(10);
						it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
						it->second->condition = LEG_CONDITION::EM_OK;
						it->status = ARBI_STATUS::EM_3_1;
					}
					DbgLog;
				}
				else if (LEG_STATUS::EM_LEG_SENDED == it->second->status)
				{
					// 第二腿送单失败，是重要异常，要查一下
					// 先平掉第一腿
					if (TRADE_OCFLAG::EM_Open == it->oc)
					{
						it->first->setCloseOrd();
						it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
						it->first->condition = LEG_CONDITION::EM_OK;
						it->status = ARBI_STATUS::EM_6_5;
					}
					else if(TRADE_OCFLAG::EM_Close == it->oc)
					{
						it->second->changeOrdPrc(10);
						it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
						it->second->condition = LEG_CONDITION::EM_OK;
						it->status = ARBI_STATUS::EM_3_1;
					}
					else
					{
						assert(0);
					}
				}
				break;
			case ARBI_STATUS::EM_3_2:
				// 超时事件
				if (LEG_STATUS::EM_LEG_ParTRADED == it->second->status)
				{
					if (curtime - it->second->ordOrderedTime > it->second->timeout)
					{
						it->second->condition = LEG_CONDITION::EM_CANCEL;
						it->second->status = LEG_STATUS::EM_LEG_CANCELREADY;

						it->status = ARBI_STATUS::EM_3_4;
						DbgLog;
					}
					break;
				}

				if (LEG_STATUS::EM_LEG_TRADED == it->second->status)
				{
					it->status = ARBI_STATUS::EM_3_3;
					DbgLog;
					break;
				}

				break;
			case ARBI_STATUS::EM_3_4:
				if (LEG_STATUS::EM_LEG_CANCELED == it->second->status)
				{
#if 0
					// 左右腿都平仓
					it->first->condition = LEG_CONDITION::EM_OK; // 准备平仓
					it->first->setCloseOrd(); // 就地改为平仓单
					it->first->status = LEG_STATUS::EM_LEG_SENDREADY;

					it->second->condition = LEG_CONDITION::EM_OK;
					it->second->setCloseOrd(); // 就地改为平仓单
					it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->status = ARBI_STATUS::EM_6_6;
#endif
					// 撤第二腿剩余，剩余量追价
					it->second->lot = it->second->clot;
					it->second->alot = it->second->lot;
					it->second->clot = 0;
					it->second->elot = 0;
					it->second->changeOrdPrc(10);
					it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->second->condition = LEG_CONDITION::EM_OK;
					it->status = ARBI_STATUS::EM_3_1;
					DbgLog;
				}
				else
				{
					std::cout << "cancel timeout error....";
					DbgLog;
					it->status = ARBI_STATUS::EM_9_9;
				}
				break;

			case ARBI_STATUS::EM_6_4:
				if (LEG_STATUS::EM_LEG_TRADED == it->first->status && LEG_STATUS::EM_LEG_CANCELED == it->second->status)
				{
					it->second->condition = LEG_CONDITION::EM_OK;
					it->second->setCloseOrd(); // 就地改为平仓单
					it->status = ARBI_STATUS::EM_6_6;
					DbgLog;
				}
				break;
			case ARBI_STATUS::EM_6_5:
				if (LEG_STATUS::EM_LEG_TRADED == it->first->status)
				{
					// 成交完毕，本单结束
					it->status = ARBI_STATUS::EM_9_9;
					DbgLog;
				}
				else if (LEG_STATUS::EM_LEG_ORDERED == it->first->status)
				{
					if (curtime - it->first->ordOrderedTime > it->first->timeout)
					{
						// 挂单超时不成交，撤单重送
						it->first->status = LEG_STATUS::EM_LEG_CANCELREADY;
						it->first->condition = LEG_CONDITION::EM_CANCEL;
						// 不做状态转移，就地等待撤单成功后再送单
					}
				}
				else if (LEG_STATUS::EM_LEG_CANCELED == it->first->status)
				{
					// 撤单成功，重新发送
					it->first->changeOrdPrc(10);
					it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->first->condition = LEG_CONDITION::EM_OK;
					// 不做状态转移
				}
				break;
			case ARBI_STATUS::EM_4_0:
				if (LEG_STATUS::EM_LEG_TRADED == it->first->status) // 撤单失败
				{
					it->status = ARBI_STATUS::EM_3_1;
					it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->second->condition = LEG_CONDITION::EM_OK;
					DbgLog;
					break;
				}

				if (LEG_STATUS::EM_LEG_ParCANCELED == it->first->status)
				{
					// 单腿部分成交，撤掉剩余部分，两腿不平衡的仓位有两种处置方式：增仓调平和减仓调平
					// 母单开仓时，增仓调平
					if (TRADE_OCFLAG::EM_Open == it->first->ocFlag)
					{
						if (ARBI_BALANCE::EM_BL_CUT == it->arbit->openBalanceMethod)
						{
							// 减仓调平
							it->first->setCloseOrd(); // 就地转平仓单
							it->first->condition = LEG_CONDITION::EM_OK; // LEG_STATUS::EM_LEG_CANCELED    LEG_CONDITION::EM_CLOSE
							it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
							it->status = ARBI_STATUS::EM_6_0;
						}
						else if (ARBI_BALANCE::EM_BL_FILL == it->arbit->openBalanceMethod)
						{
							// 增仓调平，左补平，右送单
							it->first->changeOrdPrc(10);
							it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
							it->first->condition = LEG_CONDITION::EM_OK;
							it->status = ARBI_STATUS::EM_1_0;
						}
						DbgLog;
					}
					else if (TRADE_OCFLAG::EM_Close == it->first->ocFlag)// 母单平仓时追补
					{
						if (ARBI_BALANCE::EM_BL_FILL == it->arbit->openBalanceMethod)
						{
							// 追补后正常送右腿
							it->first->changeOrdPrc(10);
							it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
							it->first->condition = LEG_CONDITION::EM_OK;

							it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
							it->second->condition = LEG_CONDITION::EM_OK;
							it->status = ARBI_STATUS::EM_3_1;
						}
						else
						{
							std::cout << "减仓调平错误。\n";
						}
						DbgLog;
					}
					else assert(0);

				}
				else if (LEG_STATUS::EM_LEG_CANCELED == it->first->status) // 源自EM_6_0改单重发
				{
					it->status = ARBI_STATUS::EM_6_0;
					DbgLog;
				}
				break;
			case ARBI_STATUS::EM_6_0:
				if (LEG_STATUS::EM_LEG_ORDERED == it->first->status)
				{
					// 已报，等待成交
					if (curtime - it->first->ordOrderedTime > it->first->timeout)
					{
						// 超时没成交，改单(撤单重送)，重回EM_4_0
						it->first->changeOrdPrc(10); // 将价格改激进一点以便成交
						it->first->condition = LEG_CONDITION::EM_CANCEL;
						it->first->status = LEG_STATUS::EM_LEG_CANCELREADY;
						it->status = ARBI_STATUS::EM_4_0; // EM_6_0的上一个状态只可能是EM_4_0
						DbgLog;
					}
					break;
				}
				else if (LEG_STATUS::EM_LEG_TRADED == it->first->status)
				{
					it->first->condition = LEG_CONDITION::EM_COND_NULL;
					it->first->status = LEG_STATUS::EM_LEG_STATUS_NULL;
					it->status = ARBI_STATUS::EM_9_9;
					DbgLog;
				}
				break;
			case ARBI_STATUS::EM_6_6:
				if (LEG_STATUS::EM_LEG_TRADED == it->first->status && LEG_STATUS::EM_LEG_TRADED == it->second->status)
				{
					it->status = ARBI_STATUS::EM_9_9;
					DbgLog;
				}
				break;
			case ARBI_STATUS::EM_3_3:
				// 左成右成 计入仓位
				if (TRADE_DIR::EM_Long == it->dir)
				{
					if (TRADE_OCFLAG::EM_Open == it->right->ocFlag)
					{
						leftPos += it->left->lot;
						rightPos += it->right->lot;
						ArbiPosLong += (it->left->lot / ArbiInst.getLeftTradeUnit());
					}
					else if (TRADE_OCFLAG::EM_Close == it->right->ocFlag)
					{
						leftPos -= it->left->lot;
						rightPos -= it->right->lot;
						ArbiPosShort += (it->left->lot / ArbiInst.getLeftTradeUnit());
					}
					else assert(0);
				}
				else if (TRADE_DIR::EM_Short == it->dir)
				{
					if (TRADE_OCFLAG::EM_Open == it->right->ocFlag)
					{
						leftPos += it->left->lot;
						rightPos += it->right->lot;
						ArbiPosShort -= (it->left->lot / ArbiInst.getLeftTradeUnit());
					}
					else if (TRADE_OCFLAG::EM_Close == it->right->ocFlag)
					{
						leftPos -= it->left->lot;
						rightPos -= it->right->lot;
						ArbiPosLong -= (it->left->lot / ArbiInst.getLeftTradeUnit());
					}
					else assert(0);
				}
				else
				{
					assert(0);
				}
				std::cout << "cur ArbiPosLong:" << ArbiPosLong << std::endl;
				std::cout << "cur ArbiPosShort:" << ArbiPosShort << std::endl;
				it->status = GetArbiStatus(*it);
				break;
			case ARBI_STATUS::EM_5_0:
				if (LEG_STATUS::EM_LEG_CANCELSENED == it->first->status)
				{
					if (curtime - it->first->ordCancleSendTime > it->first->timeout)
					{
						// 撤单超时没有收到回报
						std::cout << "cancel order timeout error....and recancel it once\n";
						it->first->condition = LEG_CONDITION::EM_CANCEL;
						it->first->status = LEG_STATUS::EM_LEG_CANCELREADY; // 再撤一次
						it->second->condition = LEG_CONDITION::EM_COND_NULL;
						it->status = ARBI_STATUS::EM_9_9;
						DbgLog;
					}
					break;
				}

				if (LEG_STATUS::EM_LEG_TRADED == it->first->status)
				{
					// 撤单失败，实际成交了
					it->status = ARBI_STATUS::EM_3_0;
					// 重新送第二腿
					it->second->condition = LEG_CONDITION::EM_OK;
					it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->status = ARBI_STATUS::EM_3_1;
					DbgLog;
					break;
				}

				if (LEG_STATUS::EM_LEG_ParTRADED == it->first->status || LEG_STATUS::EM_LEG_ParCANCELED == it->first->status)
				{
					if (curtime - it->first->ordOrderedTime > it->first->timeout)
					{
						// 超时没有全成
						it->status = ARBI_STATUS::EM_4_0;
						it->second->condition = LEG_CONDITION::EM_COND_NULL;
						it->second->status = LEG_STATUS::EM_LEG_STATUS_NULL;
						DbgLog;
					}
					break;
				}
				if (LEG_STATUS::EM_LEG_CANCELED == it->first->status)
				{
					it->status = ARBI_STATUS::EM_9_9;
					DbgLog;
				}
				break;
			}//switch
		}// for
		InterlockedExchange64(&(spin_Locker_arbOrders), FALSE);
		//std::cout << "out of UpdateArbiOrd...\n";
	}

	// 瘸腿单的撤销：要撤未成腿并平掉已成腿
	// 若都未成，全撤
	// 两腿都成，不存在撤单
	void Arbitrage::CancelArbi(ArbOrdItem& arbiOrd)
	{
		//
	}

	// 左腿瘸处理--即右腿已经成交，左腿已报未成交
	// 在超时范围内--不做处理
	// 超时了取消母单
	void Arbitrage::procLeftLame(ArbOrdItem& arbiOrd)
	{
	}

	// 瘸腿超时
	// 瘸腿超价
	void Arbitrage::procRightLame(ArbOrdItem& arbiOrd)
	{

	}

	// 新母单执行送出
	void Arbitrage::procInitSend(ArbOrdItem& arbiOrd)
	{
#if 0
		if (LEG_TYPE::EM_Auto == arbiOrd.first)
		{

		}
		else if (LEG_TYPE::EM_LeftLeg == arbiOrd.first)
		{
			// 送单
			if (TRADE_DIR::EM_Long == arbiOrd.left->dir)
			{
				// 按照先后顺序
				// 将母单中存储的订单信息拷贝到orderbook
				while (InterlockedExchange64(&spin_Locker_ordbook, TRUE)){ Sleep(0); }
				arbiOrd.left->condition = LEG_CONDITION::EM_OK; // 直接送单
				arbiOrd.right->condition = LEG_CONDITION::EM_WAIT; // 等待
				arbiOrd.right->timeout = -1; // 置-1表示无穷等待
				InterlockedExchange64(&spin_Locker_ordbook, FALSE);
			}
		}
		else if (LEG_TYPE::EM_RightLeg == arbiOrd.first)
		{
			// 送单
			if (TRADE_DIR::EM_Long == arbiOrd.right->dir)
			{
				// 按照先后顺序
				// 将母单中存储的订单信息拷贝到orderbook
				while (InterlockedExchange64(&spin_Locker_ordbook, TRUE)){ Sleep(0); }
				arbiOrd.left->condition = LEG_CONDITION::EM_WAIT; // 直接送单
				arbiOrd.right->condition = LEG_CONDITION::EM_OK; // 等待
				InterlockedExchange64(&spin_Locker_ordbook, FALSE);
			}
		}
		else if (LEG_TYPE::EM_NoWaitBtwnLegs == arbiOrd.first) // 同时送单
		{
			while (InterlockedExchange64(&spin_Locker_ordbook, TRUE)){ Sleep(0); }
			arbiOrd.left->condition = LEG_CONDITION::EM_OK; // 直接送单
			arbiOrd.right->condition = LEG_CONDITION::EM_OK; // 等待
			InterlockedExchange64(&spin_Locker_ordbook, FALSE);
		}
#endif
	}

	void Arbitrage::ProcTimeOut(ArbOrdItem& arbiOrd)
	{

	}

	// 套利母单的子单在送母单时就同时送出，两腿只是设置不同的状态而已，ProcSubOrd只是修改状态而已
	// 只有撤单重送需要另增加条目
	// 操作orderbook进行指令更新，
	void Arbitrage::ProcSubOrd()
	{
		for (auto arbiOrd = arbOrders.begin(); arbiOrd != arbOrders.end(); ++arbiOrd)
		{

		}
	}

	void PlannedOrderItem::changeOrdPrc(int in)
	{
		if (TRADE_DIR::EM_Long == dir)
		{
			//price += in*zc::InstruInfo(Tick->InstrumentID).minpoint;
			price += in * GetMinPoint();
		}
		else if (TRADE_DIR::EM_Short == dir)
		{
			price -= in * GetMinPoint();
		}
	}

	float PlannedOrderItem::GetMinPoint()
	{
		std::string instId = Tick->InstrumentID;
		if (instId.length() < 4)std::cout << "GetMinPoint : Tick instrument error!\n";
		auto tb = arbiOrd->arbit->qrInstFB.Insts;
		auto ifound = std::find_if(tb.begin(), tb.end(), [&instId](zc::InstumentInfo& in)->bool{return in.InstrumentId == instId; });
		if (ifound != tb.end()) return ifound->minpoint;
		else return -1;
	}

	// 将已成交单或部分成交单改为平仓单
	void PlannedOrderItem::setCloseOrd()
	{
		lot = elot; // 全成则全平，部成则部平
		elot = 0;
		clot = 0;
		alot = lot;
		ordRef = -1;
		dir = (dir == TRADE_DIR::EM_Long ? TRADE_DIR::EM_Short : TRADE_DIR::EM_Long);
		ocFlag = (ocFlag == TRADE_OCFLAG::EM_Open ? TRADE_OCFLAG::EM_Close : TRADE_OCFLAG::EM_Open); // 若是平仓时，则会买回已平的腿，使左右平衡
		setPrice();
	}

	// 调用前设置好开平和方向
	float PlannedOrderItem::setPrice()
	{
		if (LEG_TYPE::EM_LeftLeg == leg)
		{
			switch (priceType)
			{
			case 1:// 左腿对价，右腿对价
			case 2:// 左腿对价，右腿挂价
				price = (TRADE_DIR::EM_Long == dir ? Tick->AskPrice1 : Tick->BidPrice1);
				break;
			case 3:// 左腿挂价，右腿对价
			case 4:// 左腿挂价，右腿挂价
				price = (TRADE_DIR::EM_Long == dir ? Tick->BidPrice1 : Tick->AskPrice1);
				break;
			}
		}
		else if (LEG_TYPE::EM_RightLeg == leg)
		{
			switch (priceType)
			{
			case 1:// 左腿对价，右腿对价
			case 3:// 左腿挂价，右腿对价
				price = TRADE_DIR::EM_Long == dir ? Tick->AskPrice1 : Tick->BidPrice1;
				break;
			case 2:// 左腿对价，右腿挂价
			case 4:// 左腿挂价，右腿挂价
				price = TRADE_DIR::EM_Long == dir ? Tick->BidPrice1 : Tick->AskPrice1;
				break;
			}
		}
		else
		{
			std::cout << "setOpenPrice error! leg error...\n";
			return -1;
		}
		return price;
	}

	void Arbitrage::UpdateParams()
	{
		// 查询数据库，获取价差上下限值
		std::string sqlstr("select code,groupname,open_value,close_value from auag.parameter");
		dbQuery<ParamRowItem4DbQry> q(g_pDb->conn, sqlstr, [](dbQuery<ParamRowItem4DbQry>* pq, MYSQL_ROW& row, int col){
			switch (col)
			{
			case 0:
				pq->allRowsData[pq->curRow].code = row[col];
				break;
			case 1:
				pq->allRowsData[pq->curRow].groupname = row[col];
				break;
			case 2:
				pq->allRowsData[pq->curRow].open_value = atof(row[col]);
				break;
			case 3:
				pq->allRowsData[pq->curRow].close_value = atof(row[col]);
				break;
			}
			return;
		});

		// 执行查询
		q.doQry();
		if (q.nRows) // 有数据
		{
			std::string secId;
			for (int i = 0; i < q.nRows; i++)
			{
				secId = q.allRowsData[i].code; // 存储的是现货,现货腿是唯一的
				auto foundit = std::find_if(zc::Arbitrage::ArbiTrades.begin(), zc::Arbitrage::ArbiTrades.end(), [&secId](Arbitrage& in){return (secId == in.ArbiInst.leftleg || secId == in.ArbiInst.rightleg); });
				if (foundit != zc::Arbitrage::ArbiTrades.end())
				{
					foundit->tradeParam = q.allRowsData[i]; // 现货腿是唯一的
					if ("first" == foundit->tradeParam.groupname)
					{
						foundit->ArbiInst.setSpreadLower1(foundit->tradeParam.close_value);
						foundit->ArbiInst.setSpreadUpper1(foundit->tradeParam.open_value);
					}
					if ("second" == foundit->tradeParam.groupname)
					{
						foundit->ArbiInst.setSpreadLower2(foundit->tradeParam.close_value);
						foundit->ArbiInst.setSpreadUpper2(foundit->tradeParam.open_value);
					}

				}
				else
				{
					std::cout << "error! UpdateParams security not found.....\n";
				}
			}
		}
	}
}