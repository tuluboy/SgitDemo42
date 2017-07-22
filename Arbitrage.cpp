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
	std::vector<Arbitrage> Arbitrage::ArbiTrades; // ����ͬʱ���׶��������
	std::vector<TradeRecItem> Arbitrage::tradeRecord;
	std::vector<PlannedOrderItem*> Arbitrage::ordbook; // �͵��ƻ���
	LONGLONG Arbitrage::spin_Locker_ordbook = 0; // ordbook����
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
		// ��ֲ֣�����Լ���նԳ�����ϳ�Ϊ��������
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
			std::cout << "InstrumentID��" << it->InstrumentID << "  direction: " << (it->PosiDirection == '1' ? "��" : "��") << "  pos: " << it->TodayPosition << "  open cost: " << it->OpenCost << std::endl;
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
		// ���������ȵĳֲִճ������ԣ�
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
		ArbiTrades[0].setOpenBalance(ARBI_BALANCE::EM_BL_FILL); // ���ֲ������ֵ�ƽ
		ArbiTrades[0].setCloseBalance(ARBI_BALANCE::EM_BL_CUT); // ���ֲ��ü��ֵ�ƽ

		ArbiTrades.push_back(arb);
		ArbiTrades[1].Init("Au(T+D)", "au1712", 1, 1);
		ArbiTrades[1].ArbiInst.setSpreadLower1(-9999);
		ArbiTrades[1].ArbiInst.setSpreadUpper1(9998);
		ArbiTrades[1].ArbiInst.setSpreadLower1(-9998);
		ArbiTrades[1].ArbiInst.setSpreadUpper1(9999);
		ArbiTrades[1].sendFirst = LEG_TYPE::EM_RightLeg;
		ArbiTrades[1].setSpot(LEG_TYPE::EM_LeftLeg);
		ArbiTrades[1].setOpenBalance(ARBI_BALANCE::EM_BL_FILL); // ���ֲ������ֵ�ƽ
		ArbiTrades[1].setCloseBalance(ARBI_BALANCE::EM_BL_CUT); // ���ֲ��ü��ֵ�ƽ

		ArbiTrades.push_back(arb);
		ArbiTrades[2].Init("mAu(T+D)", "au1712", 10, 1);
		ArbiTrades[2].ArbiInst.setSpreadLower1(-9999);
		ArbiTrades[2].ArbiInst.setSpreadUpper1(9998);
		ArbiTrades[2].ArbiInst.setSpreadLower1(-9998);
		ArbiTrades[2].ArbiInst.setSpreadUpper1(9999);
		ArbiTrades[2].sendFirst = LEG_TYPE::EM_RightLeg;
		ArbiTrades[2].setSpot(LEG_TYPE::EM_LeftLeg);
		ArbiTrades[2].setOpenBalance(ARBI_BALANCE::EM_BL_FILL); // ���ֲ������ֵ�ƽ
		ArbiTrades[2].setCloseBalance(ARBI_BALANCE::EM_BL_CUT); // ���ֲ��ü��ֵ�ƽ

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
		// ��ѯ�ֲ֣����������
		getArbiPos(pTrd);
	}

	// ��̬��������orderFunc������ѯ����״̬�����¶���״̬
	void Arbitrage::UpdateSubOrdStatus()
	{
		// �Ѿ�ִ�е��Ӷ���״̬��ȡ��������TraderSpi���������
		// ���ϣ�ֱ����TraderSpi����orderbook
	}

	void Arbitrage::UpdateTick(CThostFtdcDepthMarketDataField& tick)
	{
		// �ַ�tick������������Լ
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

	// ���������߼�����
	void Arbitrage::DoTrade()
	{
		if (doTradeCount++ % 100 == 0) std::cout << ArbiInst.ArbiInstID<< " Auto trade is running\n";
		// ���ȶ�Ҫ�����ݲ�ִ�н���
		if (ArbiInst.leftTick.LastPrice < 0 || ArbiInst.rightTick.LastPrice < 0) return;
		int curtime = DtTm::Parse(ArbiInst.leftTick.UpdateTime);
		
		bool openTimeCond = true || (curtime < 91600 || curtime>210000);

		if (curtime > 190000&& curtime < 210005) // �罻���ճ�ʼ��
		{
			// �½����ճ�ʼ��
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
			// ƽ��
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
		// �����߼�
		// �������ԣ�ִ��1��۲����
		// 9:16:00֮��ֻƽ�ֲ�����
		if (openTimeCond && ArbiInst.shortBasisSpread1 > ArbiInst.getSpreadUpper2())
		{
			if (0 == SendFlag)
			{
				std::cout << "����ͬʱ����2����������,ֱ����2��...\n";
				sellshort(2,60); // ֱ����2��
				SendFlag = 2;
				CloseFlag = 0;
			}
			else if (1 == SendFlag)
			{
				std::cout << "���㿪������2������1��...\n";
				sellshort(1, 60);
				SendFlag = 2;
			}
		}
		else if (openTimeCond && ArbiInst.shortBasisSpread1 > ArbiInst.getSpreadUpper1())
		{
			// Ŀǰֻ������ if (ArbiPos > 0) sell(ArbiPos);
			if (0 == SendFlag)
			{
				std::cout << "���㿪������1����1��...\n";
				sellshort(1, 60);
				SendFlag = 1;
				CloseFlag = 0;
			}
		}
		else if (ArbiPosShort < 0 && ArbiInst.longBasisSpread1 < ArbiInst.getSpreadLower1())
		{
			if (0 == CloseFlag)
			{
				std::cout << "��������2��ƽ������,ֱ����2��ƽ��...\n";
				buy2cover(-ArbiPosShort, 60); // ȫƽ��
				CloseFlag = 2;
				SendFlag = 0;
			}
			else if (1 == CloseFlag)
			{
				std::cout << "����ƽ������1������1��...\n";
				buy2cover(1, 60);
				CloseFlag = 2;
			}
		}
		else if (ArbiPosShort < 0 && ArbiInst.longBasisSpread1 < ArbiInst.getSpreadLower2())
		{
			if (0 == CloseFlag)
			{
				std::cout << "����ƽ������1����1��...\n";
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
		arbOrders.push_back(ArbOrdItem()); // ��Ҫ����
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

		UpdateFirst(newOrd); // ȷ��first second �� left right��ӳ���ϵ
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
		newOrd._timeout = 50; // 5�벻�ɾͳ���

		if (LEG_TYPE::EM_LeftLeg == sendFirst)newOrd.setLeftFirst(); // initArbi�б�����
		else if (LEG_TYPE::EM_RightLeg == sendFirst)newOrd.setRightFirst();
		else if (LEG_TYPE::EM_BothLegs == sendFirst)newOrd.setBothFirst();
		
		std::cout << "right ask: " << ArbiInst.rightTick.AskPrice1 << " right bid: " << ArbiInst.rightTick.BidPrice1 << std::endl;
		std::cout << "left ask: " << ArbiInst.leftTick.AskPrice1 << " left bid: " << ArbiInst.leftTick.BidPrice1 << std::endl;
		std::cout << "longSpread1:" << ArbiInst.longBasisSpread1 << " shortSpread1:" << ArbiInst.shortBasisSpread1 << std::endl;
		std::cout << "*****************************************************************\n";
		//InterlockedExchange64(&spin_Locker_arbOrders, FALSE);
		InterlockedExchange64(&spin_Locker_ordbook, FALSE);
	}

	// ��ʱ����,unit-���׵�λ��
	// priceType-�۸����ͣ�1 2 3 4
	// �۸����ʹ�����ִ��ģʽ������1�����ȶԼ����ȶԼۣ��µ�ʱҲ�����¶Լۣ������¶Լ�
	// ����ʽִ��vs�첽ִ��
	// ����ʽִ��----�ȴ����ȶ�ִ������ٷ��أ��µ�ʱ����һ���ȴ�ʱ��
	// �첽ִ��----����µ��ƻ����ֱ�ӷ��أ����������Ƿ�ɽ���ϣ����ɼ���̰߳��µ��ƻ�����
	int Arbitrage::buy(int unit, int nTimeOut)
	{
		// �������ģʽ������-����
		// �������--�������ȣ���������
		// ���Ȼ�Ծ
		/*while (InterlockedExchange64(&spinLock_arbOrders, TRUE))
		{
		Sleep(0);
		}tickFunc�̵߳��ã�*/
		Send(unit, zc::TRADE_DIR::EM_Long, zc::TRADE_OCFLAG::EM_Open, nTimeOut);
		std::cout << "buy a pair at:" << ArbiInst.longBasisSpread1 << " " << unit << std::endl;
		return 0;
	}

	// �ȴ��Ҽۿ��࣬���ȶ��ȴ���ֱ��ĳ�ֻ���ﵽ�޼۾�ת�뼴ʱ����
	int Arbitrage::buy(int unit, float basisSpread, int price) // �û���Ҽۣ�����֮�󰴼۸������͵�
	{

		return 0;
	}

	// ��ʱƽ��
	int Arbitrage::sell(int unit, int nTimeOut)
	{
		if (ArbiPosLong>0)Send(unit, zc::TRADE_DIR::EM_Short, zc::TRADE_OCFLAG::EM_Close, nTimeOut);
		else std::cout << "ƽ���λ����\n";
		return 0;
	}

	// �ȴ��Ҽ�ƽ�࣬���ȶ��ȴ�
	int Arbitrage::sell(int unit, float basisSpread, int price)
	{
		return 0;
	}

	int Arbitrage::sellshort(int unit, int nTimeOut) // ����
	{
		Send(unit, zc::TRADE_DIR::EM_Short, zc::TRADE_OCFLAG::EM_Open, nTimeOut);
		std::cout << "short a pair at:" << ArbiInst.shortBasisSpread1 << " " << unit << std::endl;
		return 0;
	}

	int Arbitrage::sellshort(int unit, float basisSpread, int price)
	{
		return 0;
	}

	int Arbitrage::buy2cover(int unit, int nTimeOut) // ƽ��
	{
		if (ArbiPosShort < 0)Send(unit, zc::TRADE_DIR::EM_Long, zc::TRADE_OCFLAG::EM_Close, nTimeOut);
		else std::cout << "ƽ�ղ�λ���㣡\n";
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

	// dir:�����ȵķ���
	float Arbitrage::getLeftPrice(int prcType, TRADE_DIR dir)
	{
		switch (prcType)
		{
		case 1:// ���ȶԼۣ����ȶԼ�
		case 2:// ���ȶԼۣ����ȹҼ�
			return TRADE_DIR::EM_Long == dir ? ArbiInst.leftTick.AskPrice1 : ArbiInst.leftTick.BidPrice1;
			break;
		case 3:// ���ȹҼۣ����ȶԼ�
		case 4:// ���ȹҼۣ����ȹҼ�
			return TRADE_DIR::EM_Long == dir ? ArbiInst.leftTick.BidPrice1 : ArbiInst.leftTick.AskPrice1;
			break;
		}
	}

	float Arbitrage::getRightPrice(int prcType, TRADE_DIR dir)
	{
		switch (prcType)
		{
		case 1:// ���ȶԼۣ����ȶԼ�
		case 3:// ���ȹҼۣ����ȶԼ�
			return TRADE_DIR::EM_Long == dir ? ArbiInst.rightTick.AskPrice1 : ArbiInst.rightTick.BidPrice1;
			break;
		case 2:// ���ȶԼۣ����ȹҼ�
		case 4:// ���ȹҼۣ����ȹҼ�
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
			curFirst = GetCurFirst(); // ����ʵ�����ȷ����ǰ����������
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

	// ��ӡ��ת״̬
#define DbgLog std::cout << "\ntrans to status:"<<Arbi_Status2Str(it->status)<<std::endl

	zc::ARBI_STATUS Arbitrage::GetArbiStatus(zc::ArbOrdItem& it)
	{
		return (TRADE_DIR::EM_Long == it.dir ? (TRADE_OCFLAG::EM_Open == it.oc ? ARBI_STATUS::EM_ARB_LONG_OPENED : ARBI_STATUS::EM_ARB_LONG_CLOSED) : (TRADE_OCFLAG::EM_Open == it.oc ? ARBI_STATUS::EM_ARB_SHORT_OPENED : ARBI_STATUS::EM_ARB_SHORT_CLOSED));
	}

	// ˢ������ĸ����״̬
	void Arbitrage::UpdateArbiOrd()
	{
		//std::cout << "Into UpdateArbiOrd...\n";
		// ������Լ״̬���壺0-���� 1-�ѱ� 2-���� 3-ȫ�� 4-���� 5-ȫ�� 6-ƽ��
		// ��ѯ�����ȳɽ�״̬�����ݴ�Arbitrage::ordbook�л�ȡ
		// ĸ��״̬��δ�ͣ�����δ�ɣ��ҳ���ȳ�������ȳ��ȫ��
		// ��������ʱ�����ֳɽ���ȫ���ɽ�
		// �������͵�������
		// ����ϵͳʱ��
		// ���ȴ���-����ģʽ-��һ��δ�ɣ���δ�ɣ�ƽ�ѳ�
		// ���ȴ���-����ģʽ-��һ��δ�ɣ���δ�ɣ��Ӽ�����

		time_t curtime = GetCurTime();
		while (InterlockedExchange64(&(spin_Locker_arbOrders), TRUE)){ Sleep(0); }
		//std::cout << "Into InterlockedExchange64..spin_Locker_arbOrders....\n";
		for (auto it = arbOrders.begin(); it != arbOrders.end(); ++it)
		{
			//std::cout << Arbi_Status2Str(it->status);
			switch (it->status)
			{
			case ARBI_STATUS::EM_0_0:
				// ��ʼ̬���͵�
				std::cout << "status init: EM_0_0......\n"; 
				//UpdateFirst(*it);  �µ�ʱ�Ѿ�ȷ������
				// �¼�����
				if (0 == it->sendDelay)
				{
					// ���Ϸ�
					if (LEG_TYPE::EM_BothLegs == sendFirst)
					{
						// ����ͬʱ��
						it->status = zc::ARBI_STATUS::EM_1_1;
						DbgLog;
						break;
					}
					else
					{
						// ����
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
					// ��ʱ����ʱ�䵽
					it->first->condition = LEG_CONDITION::EM_OK;
					it->second->condition = LEG_CONDITION::EM_WAIT;
					it->status = zc::ARBI_STATUS::EM_1_0;
					DbgLog;
					break;
				}
				break;
			case ARBI_STATUS::EM_1_1:
				// �ȼ򻯰汾ֱ�����ȳɽ�
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
						// ���� EM_!_1;
					}
					DbgLog;
					break;
				}
				else if (LEG_STATUS::EM_LEG_TRADED == it->first->status && LEG_STATUS::EM_LEG_TRADED != it->second->status)
				{
					// ���ȳɽ�����δ��
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
					// ����δ�ɺ��ȳɽ�
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
				// ��EM_0_0��ת�����������һ����ʱ��Χ��û�е��Ҫ�����쳣
				if (LEG_STATUS::EM_LEG_SENDED == it->first->status)
				{
					if (curtime - it->first->ordSendedTime > it->first->timeout)
					{
						// ���ַϵ�
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
					// �ҵ���ʱδ�ɽ�
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

				// �������ͣ����Ƿ����ͳ�
				if (zc::LEG_STATUS::EM_LEG_ParTRADED == it->first->status)
				{
					// ���ֳɽ�
					it->status = EM_2_0;
					DbgLog;
					break;
				}
				else if (LEG_STATUS::EM_LEG_TRADED == it->first->status)
				{
					// ���ȵĵ����Ѿ���д����ordbook����һ��ֱ���޸�״̬����
					it->second->condition = LEG_CONDITION::EM_OK;
					it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->status = EM_3_1; // EM_3_0 EM_3_1�ϲ�
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

				// ��ʱδ��
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
						// ���ַϵ���Ҫ�������ӣ����ⷴ�������Ⱥ�ƽ�֣���ɴ���С��������Լ���������ʧ
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						std::cout << "sended order timeout error....\n";
						it->second->condition = LEG_CONDITION::EM_COND_NULL; // Ҫ��Ҫ��һ������ָ�
						//it->first->condition = LEG_CONDITION::EM_OK;
						it->status = ARBI_STATUS::EM_3_5; // �ϵ�Ҳ��Ϊ�Ǳ�������ϵͳ�Զ�����
						DbgLog;
					}
					break;
				}

				if (LEG_STATUS::EM_LEG_ORDERED == it->second->status)
				{
					// �ҵ���ʱδ�ɽ�
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
					// ���ȴ���-����ģʽ-��һ��δ�ɣ���δ�ɣ�ƽ�ѳ�
					// ���ȴ���-����ģʽ-��һ��δ�ɣ���δ�ɣ��Ӽ�����
					// �ڶ��ȳ�ʱδ�ɽ��������ɹ���ƽ����һ��
					if (TRADE_OCFLAG::EM_Open == it->oc)
					{
						it->first->setCloseOrd(); // �͵�תƽ��
						it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
						it->first->condition = LEG_CONDITION::EM_OK;
						it->status = ARBI_STATUS::EM_6_5;
					}
					else // ƽ��ʱ�ü���ģʽ
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
					// �ڶ����͵�ʧ�ܣ�����Ҫ�쳣��Ҫ��һ��
					// ��ƽ����һ��
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
				// ��ʱ�¼�
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
					// �����ȶ�ƽ��
					it->first->condition = LEG_CONDITION::EM_OK; // ׼��ƽ��
					it->first->setCloseOrd(); // �͵ظ�Ϊƽ�ֵ�
					it->first->status = LEG_STATUS::EM_LEG_SENDREADY;

					it->second->condition = LEG_CONDITION::EM_OK;
					it->second->setCloseOrd(); // �͵ظ�Ϊƽ�ֵ�
					it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->status = ARBI_STATUS::EM_6_6;
#endif
					// ���ڶ���ʣ�࣬ʣ����׷��
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
					it->second->setCloseOrd(); // �͵ظ�Ϊƽ�ֵ�
					it->status = ARBI_STATUS::EM_6_6;
					DbgLog;
				}
				break;
			case ARBI_STATUS::EM_6_5:
				if (LEG_STATUS::EM_LEG_TRADED == it->first->status)
				{
					// �ɽ���ϣ���������
					it->status = ARBI_STATUS::EM_9_9;
					DbgLog;
				}
				else if (LEG_STATUS::EM_LEG_ORDERED == it->first->status)
				{
					if (curtime - it->first->ordOrderedTime > it->first->timeout)
					{
						// �ҵ���ʱ���ɽ�����������
						it->first->status = LEG_STATUS::EM_LEG_CANCELREADY;
						it->first->condition = LEG_CONDITION::EM_CANCEL;
						// ����״̬ת�ƣ��͵صȴ������ɹ������͵�
					}
				}
				else if (LEG_STATUS::EM_LEG_CANCELED == it->first->status)
				{
					// �����ɹ������·���
					it->first->changeOrdPrc(10);
					it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->first->condition = LEG_CONDITION::EM_OK;
					// ����״̬ת��
				}
				break;
			case ARBI_STATUS::EM_4_0:
				if (LEG_STATUS::EM_LEG_TRADED == it->first->status) // ����ʧ��
				{
					it->status = ARBI_STATUS::EM_3_1;
					it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
					it->second->condition = LEG_CONDITION::EM_OK;
					DbgLog;
					break;
				}

				if (LEG_STATUS::EM_LEG_ParCANCELED == it->first->status)
				{
					// ���Ȳ��ֳɽ�������ʣ�ಿ�֣����Ȳ�ƽ��Ĳ�λ�����ִ��÷�ʽ�����ֵ�ƽ�ͼ��ֵ�ƽ
					// ĸ������ʱ�����ֵ�ƽ
					if (TRADE_OCFLAG::EM_Open == it->first->ocFlag)
					{
						if (ARBI_BALANCE::EM_BL_CUT == it->arbit->openBalanceMethod)
						{
							// ���ֵ�ƽ
							it->first->setCloseOrd(); // �͵�תƽ�ֵ�
							it->first->condition = LEG_CONDITION::EM_OK; // LEG_STATUS::EM_LEG_CANCELED    LEG_CONDITION::EM_CLOSE
							it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
							it->status = ARBI_STATUS::EM_6_0;
						}
						else if (ARBI_BALANCE::EM_BL_FILL == it->arbit->openBalanceMethod)
						{
							// ���ֵ�ƽ����ƽ�����͵�
							it->first->changeOrdPrc(10);
							it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
							it->first->condition = LEG_CONDITION::EM_OK;
							it->status = ARBI_STATUS::EM_1_0;
						}
						DbgLog;
					}
					else if (TRADE_OCFLAG::EM_Close == it->first->ocFlag)// ĸ��ƽ��ʱ׷��
					{
						if (ARBI_BALANCE::EM_BL_FILL == it->arbit->openBalanceMethod)
						{
							// ׷��������������
							it->first->changeOrdPrc(10);
							it->first->status = LEG_STATUS::EM_LEG_SENDREADY;
							it->first->condition = LEG_CONDITION::EM_OK;

							it->second->status = LEG_STATUS::EM_LEG_SENDREADY;
							it->second->condition = LEG_CONDITION::EM_OK;
							it->status = ARBI_STATUS::EM_3_1;
						}
						else
						{
							std::cout << "���ֵ�ƽ����\n";
						}
						DbgLog;
					}
					else assert(0);

				}
				else if (LEG_STATUS::EM_LEG_CANCELED == it->first->status) // Դ��EM_6_0�ĵ��ط�
				{
					it->status = ARBI_STATUS::EM_6_0;
					DbgLog;
				}
				break;
			case ARBI_STATUS::EM_6_0:
				if (LEG_STATUS::EM_LEG_ORDERED == it->first->status)
				{
					// �ѱ����ȴ��ɽ�
					if (curtime - it->first->ordOrderedTime > it->first->timeout)
					{
						// ��ʱû�ɽ����ĵ�(��������)���ػ�EM_4_0
						it->first->changeOrdPrc(10); // ���۸�ļ���һ���Ա�ɽ�
						it->first->condition = LEG_CONDITION::EM_CANCEL;
						it->first->status = LEG_STATUS::EM_LEG_CANCELREADY;
						it->status = ARBI_STATUS::EM_4_0; // EM_6_0����һ��״ֻ̬������EM_4_0
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
				// ����ҳ� �����λ
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
						// ������ʱû���յ��ر�
						std::cout << "cancel order timeout error....and recancel it once\n";
						it->first->condition = LEG_CONDITION::EM_CANCEL;
						it->first->status = LEG_STATUS::EM_LEG_CANCELREADY; // �ٳ�һ��
						it->second->condition = LEG_CONDITION::EM_COND_NULL;
						it->status = ARBI_STATUS::EM_9_9;
						DbgLog;
					}
					break;
				}

				if (LEG_STATUS::EM_LEG_TRADED == it->first->status)
				{
					// ����ʧ�ܣ�ʵ�ʳɽ���
					it->status = ARBI_STATUS::EM_3_0;
					// �����͵ڶ���
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
						// ��ʱû��ȫ��
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

	// ȳ�ȵ��ĳ�����Ҫ��δ���Ȳ�ƽ���ѳ���
	// ����δ�ɣ�ȫ��
	// ���ȶ��ɣ������ڳ���
	void Arbitrage::CancelArbi(ArbOrdItem& arbiOrd)
	{
		//
	}

	// ����ȳ����--�������Ѿ��ɽ��������ѱ�δ�ɽ�
	// �ڳ�ʱ��Χ��--��������
	// ��ʱ��ȡ��ĸ��
	void Arbitrage::procLeftLame(ArbOrdItem& arbiOrd)
	{
	}

	// ȳ�ȳ�ʱ
	// ȳ�ȳ���
	void Arbitrage::procRightLame(ArbOrdItem& arbiOrd)
	{

	}

	// ��ĸ��ִ���ͳ�
	void Arbitrage::procInitSend(ArbOrdItem& arbiOrd)
	{
#if 0
		if (LEG_TYPE::EM_Auto == arbiOrd.first)
		{

		}
		else if (LEG_TYPE::EM_LeftLeg == arbiOrd.first)
		{
			// �͵�
			if (TRADE_DIR::EM_Long == arbiOrd.left->dir)
			{
				// �����Ⱥ�˳��
				// ��ĸ���д洢�Ķ�����Ϣ������orderbook
				while (InterlockedExchange64(&spin_Locker_ordbook, TRUE)){ Sleep(0); }
				arbiOrd.left->condition = LEG_CONDITION::EM_OK; // ֱ���͵�
				arbiOrd.right->condition = LEG_CONDITION::EM_WAIT; // �ȴ�
				arbiOrd.right->timeout = -1; // ��-1��ʾ����ȴ�
				InterlockedExchange64(&spin_Locker_ordbook, FALSE);
			}
		}
		else if (LEG_TYPE::EM_RightLeg == arbiOrd.first)
		{
			// �͵�
			if (TRADE_DIR::EM_Long == arbiOrd.right->dir)
			{
				// �����Ⱥ�˳��
				// ��ĸ���д洢�Ķ�����Ϣ������orderbook
				while (InterlockedExchange64(&spin_Locker_ordbook, TRUE)){ Sleep(0); }
				arbiOrd.left->condition = LEG_CONDITION::EM_WAIT; // ֱ���͵�
				arbiOrd.right->condition = LEG_CONDITION::EM_OK; // �ȴ�
				InterlockedExchange64(&spin_Locker_ordbook, FALSE);
			}
		}
		else if (LEG_TYPE::EM_NoWaitBtwnLegs == arbiOrd.first) // ͬʱ�͵�
		{
			while (InterlockedExchange64(&spin_Locker_ordbook, TRUE)){ Sleep(0); }
			arbiOrd.left->condition = LEG_CONDITION::EM_OK; // ֱ���͵�
			arbiOrd.right->condition = LEG_CONDITION::EM_OK; // �ȴ�
			InterlockedExchange64(&spin_Locker_ordbook, FALSE);
		}
#endif
	}

	void Arbitrage::ProcTimeOut(ArbOrdItem& arbiOrd)
	{

	}

	// ����ĸ�����ӵ�����ĸ��ʱ��ͬʱ�ͳ�������ֻ�����ò�ͬ��״̬���ѣ�ProcSubOrdֻ���޸�״̬����
	// ֻ�г���������Ҫ��������Ŀ
	// ����orderbook����ָ����£�
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

	// ���ѳɽ����򲿷ֳɽ�����Ϊƽ�ֵ�
	void PlannedOrderItem::setCloseOrd()
	{
		lot = elot; // ȫ����ȫƽ��������ƽ
		elot = 0;
		clot = 0;
		alot = lot;
		ordRef = -1;
		dir = (dir == TRADE_DIR::EM_Long ? TRADE_DIR::EM_Short : TRADE_DIR::EM_Long);
		ocFlag = (ocFlag == TRADE_OCFLAG::EM_Open ? TRADE_OCFLAG::EM_Close : TRADE_OCFLAG::EM_Open); // ����ƽ��ʱ����������ƽ���ȣ�ʹ����ƽ��
		setPrice();
	}

	// ����ǰ���úÿ�ƽ�ͷ���
	float PlannedOrderItem::setPrice()
	{
		if (LEG_TYPE::EM_LeftLeg == leg)
		{
			switch (priceType)
			{
			case 1:// ���ȶԼۣ����ȶԼ�
			case 2:// ���ȶԼۣ����ȹҼ�
				price = (TRADE_DIR::EM_Long == dir ? Tick->AskPrice1 : Tick->BidPrice1);
				break;
			case 3:// ���ȹҼۣ����ȶԼ�
			case 4:// ���ȹҼۣ����ȹҼ�
				price = (TRADE_DIR::EM_Long == dir ? Tick->BidPrice1 : Tick->AskPrice1);
				break;
			}
		}
		else if (LEG_TYPE::EM_RightLeg == leg)
		{
			switch (priceType)
			{
			case 1:// ���ȶԼۣ����ȶԼ�
			case 3:// ���ȹҼۣ����ȶԼ�
				price = TRADE_DIR::EM_Long == dir ? Tick->AskPrice1 : Tick->BidPrice1;
				break;
			case 2:// ���ȶԼۣ����ȹҼ�
			case 4:// ���ȹҼۣ����ȹҼ�
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
		// ��ѯ���ݿ⣬��ȡ�۲�������ֵ
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

		// ִ�в�ѯ
		q.doQry();
		if (q.nRows) // ������
		{
			std::string secId;
			for (int i = 0; i < q.nRows; i++)
			{
				secId = q.allRowsData[i].code; // �洢�����ֻ�,�ֻ�����Ψһ��
				auto foundit = std::find_if(zc::Arbitrage::ArbiTrades.begin(), zc::Arbitrage::ArbiTrades.end(), [&secId](Arbitrage& in){return (secId == in.ArbiInst.leftleg || secId == in.ArbiInst.rightleg); });
				if (foundit != zc::Arbitrage::ArbiTrades.end())
				{
					foundit->tradeParam = q.allRowsData[i]; // �ֻ�����Ψһ��
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