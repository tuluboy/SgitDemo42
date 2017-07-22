#include <stdio.h>
//#include <conio.h>
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "TradeSpi.h"
#include "MdSpi.h"
#include <functional>
#include "GetParam.h"
#include <iostream>
#include<fstream> 
#include <string>
#include "cirque.h"
#include <thread>
#include "zc_util.h"
#include "ArbOrdItem.h"
#include "dbman.h"
//#include "DtTm.h"
#include <iomanip>

#ifdef _USE_MYSQL_

#endif
#include <time.h>
using namespace std;
#ifdef WIN32
#include <windows.h>
void sgit_Sleep(float seconds)
{
	Sleep(seconds * 1000);
}
#else
#include "unistd.h"
void sgit_Sleep(int seconds)
{
	::sleep(seconds);
}
#endif // !WIN3
// ��������
char * pSubInstrumnet[] =
{
	"ag1712",
	"au1712",
	"Ag(T+D)",
	"Au(T+D)",
	"mAu(T+D)"
};

dbman* g_pDb = nullptr;

int requestID = 0;
int GetRequsetID(){ return ++requestID; }

int ReqQrySettlementInfo(CThostFtdcTraderApi* pTradeApi);
int ReqOrderInsert(CThostFtdcTraderApi* pTradeApi, long OrderRef);
int ReqOrderAction(CTradeSpi* pTradeSpi);
int ReqQryTrade(CThostFtdcTraderApi* pTradeApi);
int ReqQryOrder(CThostFtdcTraderApi* pTradeApi);
int ReqQryInvestorPosition(CThostFtdcTraderApi* pTradeApi);
int ReqQryTradingCode(CThostFtdcTraderApi* pTradeApi);
int ReqQryInstrumentMarginRate(CThostFtdcTraderApi* pTradeApi);
int ReqQryInstrumentCommissionRate(CThostFtdcTraderApi* pTradeApi);
int ReqQryPositionDetail(CThostFtdcTraderApi* pTradeApi);
int ReqQrySettlementConfirm(CThostFtdcTraderApi* pTradeApi);
int ReqQryNotice(CThostFtdcTraderApi* pTradeApi);
int ReqQryComPositionDetail(CThostFtdcTraderApi* pTradeApi);
int ReqQryTradeParam(CThostFtdcTraderApi* pTradeApi);
int Subquot(CThostFtdcMdApi* pMdApi);
int UnsubQuot(CThostFtdcMdApi* pMdApi);
#ifdef _USE_MYSQL_
bool isindb(MYSQL *conn, char *tbname);
#endif

int myReqOrderInsert(CThostFtdcTraderApi* pTradeApi, long iOrderRef, CThostFtdcInputOrderField InputOrder);

//-------------------------------------- Test -------------------------------------------------------//
#define xxx__TESTING__
void test_cirque()
{
	zc::cirque<int> cq;
	printf("cq.GetSize() = %d\n", cq.GetSize());
	int*pa = cq.read();
	if (!pa)printf("cq empty,read get null--ok!!!\n");
	cq.pop();

	cq.write(1);
	printf("cq.GetSize() = %d\n", cq.GetSize());
	cq.write(2);
	printf("cq.GetSize() = %d\n", cq.GetSize());
	cq.write(3);
	printf("cq.GetSize() = %d\n", cq.GetSize());

	int* b = cq.read();
	cq.pop();
	if (!b)printf("cq.read failed!\n");
	else printf("cq.read :%d\n", *b);
	printf("cq.GetSize() = %d\n", cq.GetSize());

	b = cq.read();
	cq.pop();
	printf("cq.read :%d\n", *b);
	printf("cq.GetSize() = %d\n", cq.GetSize());

	b = cq.read();
	cq.pop();
	if (!b)printf("cq.read failed!\n");
	else printf("cq.read :%d\n", *b);

	printf("cq.GetSize() = %d\n", cq.GetSize());

	b = cq.read();
	cq.pop();
	if (!b)printf("cq.read failed!\n");
	else printf("cq.read :%d\n", *b);
	printf("cq.GetSize() = %d\n", cq.GetSize());

	b = cq.read();
	cq.pop();
	if (!b)printf("cq.read failed!\n");
	else printf("cq.read :%d\n", *b);

	printf("cq.GetSize() = %d\n", cq.GetSize());

	for (int i = 0; i < 12; i++)
	{
		if (!cq.write(&i))
		{
			printf("cq full... write failed!\n");
		}
		printf("cq.GetSize() = %d\n", cq.GetSize());
	}
}

void test()
{
	test_cirque();
}

#define TS(tk,X) std::cout<<""#X":"<<tk.##X<<","<<std::endl

//-------------------------------------- Test -------------------------------------------------------//
const std::string& ToString(std::stringstream& ss, CThostFtdcDepthMarketDataField& tk)
{
	TS(tk, InstrumentID);
	ss << "TradingDay:" << tk.TradingDay << "," << "InstrumentID:" << tk.InstrumentID << "," << "LastPrice:" << tk.LastPrice << "," << "PreSettlementPrice:" << tk.PreSettlementPrice << "," << \
		"PreClosePrice:" << tk.PreClosePrice << "," << "OpenPrice:" << tk.OpenPrice << "," << "HighestPrice:" << tk.HighestPrice << "," << "LowestPrice:" << tk.LowestPrice << "," << "Volume:" << tk.Volume << "," << \
		"Turnover:" << tk.Turnover << "," << "OpenInterest:" << tk.OpenInterest << "," << "SettlementPrice:" << tk.SettlementPrice << "," << "UpdateTime:" << tk.UpdateTime << "," << "UpdateMillisec:" << tk.UpdateMillisec << "," << \
		"BidPrice1:" << tk.BidPrice1 << "," << "BidVolume1:" << tk.BidVolume1 << "," << "AskPrice1:" << tk.AskPrice1 << "," << "AskVolume1:" << tk.AskVolume1 << "," << "AveragePrice:" << tk.AveragePrice << "," << \
		"ActionDay:" << tk.ActionDay;
	return ss.str();
}

// �����߳�
void TickFunc(CMdSpi* pMdSpi = nullptr)
{
	if (!pMdSpi)
	{
		std::cout << "CMdSpi object null error!\n" << std::endl;
		Sleep(1000);
		return;
	}

	CThostFtdcDepthMarketDataField* pCurTick = nullptr;
	CThostFtdcDepthMarketDataField curTick;
	time_t curTime = 0;
	time_t dbUpdateTime = 0;

	std::stringstream ss;
	for (;;)
	{
		////		std::cout << "TickFunc running...\n";
		pCurTick = zc::Arbitrage::ticks.read();
		if (!pCurTick)
		{
			////			std::cout << "ticks que is empty!\n";
			curTime = zc::GetCurTime();
			if (curTime >= dbUpdateTime)
			{
				zc::Arbitrage::UpdateParams();
				dbUpdateTime = curTime + 30;
				//std::cout << "\nUpdateParams done......\n";
			}
			Sleep(100);
			continue;
		}
		curTick = *pCurTick;
		zc::Arbitrage::ticks.pop();
		//ToString(ss, curTick);
		//std::cout << ss.str() << std::endl;
		zc::Arbitrage::UpdateTick(curTick);
		if (111 != pMdSpi->running)break;
	}
}

// tradeSpi
void orderFunc(CTradeSpi* pTrdSpi = nullptr)
{
	if (!pTrdSpi)
	{
		std::cout << "CTradeSpi object null error!\n" << std::endl;
		Sleep(1000);
		return;
	}

	std::string oc;
	for (;;)
	{
		while (InterlockedExchange64(&(zc::Arbitrage::spin_Locker_ordbook), TRUE)){ Sleep(0); }
		// ������������
		for (auto it = zc::Arbitrage::ArbiTrades.begin(); it != zc::Arbitrage::ArbiTrades.end(); ++it)
		{
			it->UpdateArbiOrd(); // ˢ��ĸ��״̬ �����ӵ����������µ��ƻ��´�
		}
		// ���嶩��ִ�в�
		// ����������ֻ��Ҫ����ָ��ɵɵִ�м���
		// �Ѿ��ɽ��ģ��гɽ����������ľ�ִ�гɽ���������
		// �͵�ʧ�ܵļ�¼�����·���
		// ��ʱδ�ɵĳ����ط�
		// ������ģʽ����ʱ��������ʱ�������ɽ��󴥷��µ�
		//std::cout << "ready into InterlockedExchange64.. spin_Locker_ordbook ..\n";

		//std::cout << "already into InterlockedExchange64.. spin_Locker_ordbook ..\n";
		for (auto it = zc::Arbitrage::ordbook.begin(); it != zc::Arbitrage::ordbook.end(); ++it)
		{
			// ״̬����
			// zc::Arbitrage::UpdateSubOrdStatus();
			// ״̬�Ѿ�����
			// �����µ�
			switch ((*it)->condition)
			{
			case zc::LEG_CONDITION::EM_OK: // ���������͵���
				// ���͵�
				if (zc::LEG_STATUS::EM_LEG_SENDREADY == (*it)->status)
				{
					// Send�����������status��������ظ��͵�
					std::cout << "status:" << (*it)->status << " condi:" << (*it)->condition << " send issue: " << (*it)->instId << std::endl;;
					(*it)->condition = zc::LEG_CONDITION::EM_COND_NULL;
					(*it)->ordSendedTime = zc::GetCurTime();
					(*it)->ordRef = ++(*it)->pTrdSpi->OrderRef;
					if (zc::TRADE_OCFLAG::EM_Open == (*it)->ocFlag)
					{
						(*it)->pTrdSpi->SendOpen((*it)->instId.c_str(), (*it)->alot, (*it)->dir, (*it)->price, (*it)->isSpot);
						oc = "open";
					}
					else if (zc::TRADE_OCFLAG::EM_Close == (*it)->ocFlag)
					{
						(*it)->pTrdSpi->SendClose((*it)->instId.c_str(), (*it)->alot, (*it)->dir, (*it)->price, (*it)->isSpot);
						oc = "close";
					}
					else
					{
						std::cout << "send order ocFlag error!\n";
						assert(0);
					}

					(*it)->status = zc::LEG_STATUS::EM_LEG_SENDED; // EM_LEG_SENDED��̬->EM_LEG_ORDERED
					// Send�����������status��������ظ��͵�
					if (zc::LEG_TYPE::EM_LeftLeg == (*it)->leg)
					{
						std::cout << oc << " left leg sended..." << std::endl;
					}
					else if (zc::LEG_TYPE::EM_RightLeg == (*it)->leg)
					{
						std::cout << oc << " right leg sended..." << std::endl;
					}
					else
					{
						std::cout << "leg error..." << std::endl;
					}
				}
				break;
			case zc::LEG_CONDITION::EM_WAIT:
				// �ȴ�
				//(*it)->curtime = zc::GetCurTime();
				/*if ((*it)->arbiOrd->sendDelay>0 && (*it)->curtime - (*it)->ordCreatedTime > (*it)->timeout)
				{
				// �ȴ�ʱ�䵽
				std::cout << "waiting order timeout\n";
				(*it)->condition = zc::LEG_CONDITION::EM_OK;
				(*it)->status = zc::LEG_STATUS::EM_LEG_SENDREADY;
				}*/
				break;
			case zc::LEG_CONDITION::EM_CANCEL:
				// it->arbiOrd->arbit->pTrdSpi->m_pReqApi->ReqOrderAction();
				if (zc::LEG_STATUS::EM_LEG_CANCELREADY == (*it)->status || zc::LEG_STATUS::EM_LEG_ParTRADED == (*it)->status)
				{
					(*it)->ordCancleSendTime = zc::GetCurTime();
					(*it)->pTrdSpi->CancelOrd(*it);
					(*it)->status = zc::LEG_STATUS::EM_LEG_CANCELSENED;
				}
				break;
			}//switch
		}//for
		InterlockedExchange64(&(zc::Arbitrage::spin_Locker_ordbook), FALSE);
		//std::cout << "out InterlockedExchange64.. spin_Locker_ordbook ..\n";
		if (111 != pTrdSpi->running)break;
	}
}

void dispMenu()
{
	std::cout << "\n\n########################################################\n";
	std::cout << "\t            M E N U\n";
	std::cout << "\t1. list pairs\n";
	std::cout << "\t2. open an ArbiOrd manualy\n";
	std::cout << "\t3. auto trade toggle\n";
	std::cout << "\t4. get spread\n";
	std::cout << "\t5. set params\n";
	std::cout << "\t6. get params\n";
	std::cout << "\t9. show menu\n";
	std::cout << "\t0. quit\n";
	std::cout << "\t99. change password\n";
	std::cout << "########################################################\n";
}

void getCmd(int& cmd)
{
	zc::RecieveInput("\ninput your cmd:  ", cmd, [](int& in)->bool{if (in < 0 || in>99){ dispMenu(); return false; } else return true; });
}

void disppairs()
{
	std::cout << "Current Arbitrage Pairs:\n" << std::endl;
	int ns = zc::Arbitrage::ArbiTrades.size();
	for (int i = 0; i < ns; i++)
	{
		std::cout << "\t[" << i << "]" << zc::Arbitrage::ArbiTrades[i].ArbiInst.ArbiInstID << std::endl;
	}
	std::cout << std::endl;
}

void dispspread()
{
	for (auto it = zc::Arbitrage::ArbiTrades.begin(); it != zc::Arbitrage::ArbiTrades.end(); ++it)
	{
		std::cout << it->ArbiInst.ArbiInstID << " longSpread1:" << it->ArbiInst.longBasisSpread1 << " shortSpread1:" << it->ArbiInst.shortBasisSpread1 << std::endl;
	}
}

void execute(const int cmdnum, const CMdSpi in_mdspi, const CTradeSpi in_trdspi)
{
	//if (cmdnum > 9 || cmdnum < 0)return;
	int ind = -1;
	float botlev;
	float toplev;
	std::string input;
	std::string dir;
	std::string ocflg;
	int pricetype; // 1 2 3 4
	int ns;
	int unit;
	switch (cmdnum)
	{
	case 1:
		disppairs();
		for (auto it = zc::Arbitrage::ArbiTrades.begin(); it != zc::Arbitrage::ArbiTrades.end(); ++it)
		{
			std::cout << it->ArbiInst.ArbiInstID << "  arbiPos long:" << it->ArbiPosLong << " arbiPos short:" << it->ArbiPosShort << std::endl;
		}
		break;
	case 2:
		disppairs();
		dispspread();
		ns = zc::Arbitrage::ArbiTrades.size();
		zc::RecieveInput("\nwhich to be trade:  ", ind, [ns](int& in)->bool{return in < ns; });
		zc::RecieveInput("\nlong or short: ", dir, [](std::string& in)->bool{return true; });
		zc::RecieveInput("\nopen or close: ", ocflg, [](std::string& in)->bool{return true; });
		zc::RecieveInput("\nprice type(1-4): ", pricetype, [](int& in)->bool{return true; });
		zc::RecieveInput("\ntrade unit: ", unit, [](int& in)->bool{return true; });
		zc::Arbitrage::ArbiTrades[ind].priceType = pricetype;
		if (dir == "long")
		{
			if (ocflg == "open")
			{
				zc::Arbitrage::ArbiTrades[ind].buy(unit, 30);
			}
			else
			{
				zc::Arbitrage::ArbiTrades[ind].buy2cover(unit, 30);
			}
		}
		else
		{
			if (ocflg == "open")
			{
				zc::Arbitrage::ArbiTrades[ind].sellshort(unit, 30);
			}
			else
			{
				zc::Arbitrage::ArbiTrades[ind].sell(unit, 30);
			}
		}

		break;
	case 3:
		disppairs();
		ns = zc::Arbitrage::ArbiTrades.size();
		zc::RecieveInput("\nwhich to be enable/disable AutoTrading:  ", ind, [ns](int& in)->bool{return in < ns; });
		if (false == zc::Arbitrage::ArbiTrades[ind].getAutoTrade())
		{
			std::cout << "Notice: Auto trading will be enabled!" << std::endl;
			zc::Arbitrage::ArbiTrades[ind].SetAutoTrade();
		}
		else
		{
			std::cout << "Notice: Auto trading will be disabled!" << std::endl;
			zc::Arbitrage::ArbiTrades[ind].StopAutoTrade();
		}

		break;
	case 4:
		dispspread();
		break;
	case 5:
		disppairs();
		ns = zc::Arbitrage::ArbiTrades.size();
		zc::RecieveInput("\nwhich to be modified:  ", ind, [ns](int& in)->bool{return in < ns; });
		zc::RecieveInput("\nlong line:", botlev, [](float& in)->bool{return true; });
		zc::RecieveInput("\nshort line:", toplev, [](float& in)->bool{return true; });
		zc::Arbitrage::ArbiTrades[ind].ArbiInst.setSpreadLower1(botlev);
		zc::Arbitrage::ArbiTrades[ind].ArbiInst.setSpreadUpper1(toplev);
		zc::Arbitrage::ArbiTrades[ind].ArbiInst.setSpreadLower2(botlev + 1);
		zc::Arbitrage::ArbiTrades[ind].ArbiInst.setSpreadUpper2(toplev + 1);
		break;
	case 6:
		disppairs();
		ns = zc::Arbitrage::ArbiTrades.size();
		//zc::RecieveInput("\nwhich to be showed:  ", ind, [ns](int& in)->bool{return in < ns; });
		std::cout << std::left << std::setw(18) << "ArbiInstID" << std::right << std::setw(10) << "Lower1" << std::right << std::setw(10) << "Upper1" << std::right << std::setw(10) << "Lower2" << std::right << std::setw(10) << "Upper2" << std::endl;
		for (int i = 0; i < ns; i++)
		{
			std::cout << std::left << std::setw(18) << zc::Arbitrage::ArbiTrades[i].ArbiInst.ArbiInstID;
			std::cout << std::right << std::setw(10) << zc::Arbitrage::ArbiTrades[i].ArbiInst.getSpreadLower1();
			std::cout << std::right << std::setw(10) << zc::Arbitrage::ArbiTrades[i].ArbiInst.getSpreadUpper1();
			std::cout << std::right << std::setw(10) << zc::Arbitrage::ArbiTrades[i].ArbiInst.getSpreadLower2();
			std::cout << std::right << std::setw(10) << zc::Arbitrage::ArbiTrades[i].ArbiInst.getSpreadUpper2() << std::endl;
		}
		break;
	case 9:
		dispMenu();
		break;
	case 99:
		CThostFtdcUserPasswordUpdateField udpw;
		strncpy(udpw.UserID, "08000014", sizeof(TThostFtdcUserIDType));
		strncpy(udpw.OldPassword, "501947", sizeof(TThostFtdcPasswordType));
		strncpy(udpw.NewPassword, "666888", sizeof(TThostFtdcPasswordType));
		udpw.BrokerID[0] = '\0';
		in_trdspi.m_pReqApi->ReqUserPasswordUpdate(&udpw, GetRequsetID());
		Sleep(3000);
		break;
	default:
		return;
	}
}

int main(int argc, char** argv)
{
#ifdef __TESTING__
	test();
	return 0;
#endif
	try
	{
		std::cout << "############�ƽ������������ϵͳ###########\n";
		//��������api����
		CThostFtdcTraderApi* pTradeApi = CThostFtdcTraderApi::CreateFtdcTraderApi("");
		//��������api����
		CThostFtdcMdApi* pMdApi = CThostFtdcMdApi::CreateFtdcMdApi();

		//��ȡ��½����
		CThostFtdcReqUserLoginField LoginField;
		memset(&LoginField, 0, sizeof(LoginField));
		//	getParam.GetLoginField(LoginField);
		//	strcpy(LoginField.UserID, "08000037");
		//	strcpy(LoginField.Password, "888888");
		//	strcpy(LoginField.BrokerID, "");
		std::string RunningType;
		zc::RecieveInput("\nVirtual trading or Real trading[V/Real]:  ", RunningType, [](std::string& in)->bool{if (in == "V" || in == "v" || in == "Real")return true; else return false; });
		std::string FuserID, pswd, TuserID;
		char tradeIp[36];
		char quotIp[36];
		if (RunningType == "Real")
		{
			strcpy(tradeIp, "tcp://140.207.169.150:20022");
			strcpy(quotIp, "tcp://140.207.169.150:20023");
			//strcpy(tradeIp, "192.168.20.13:7776");
			//strcpy(quotIp, "192.168.20.13:7777");
			FuserID = "12200393";
			TuserID = "165766";
			pswd = "norway%1";
		}
		else
		{
			strcpy(tradeIp, "tcp://140.206.81.6:27776");
			strcpy(quotIp, "tcp://140.206.81.6:27777");
			FuserID = "08000014";
			TuserID = "06000014";
			pswd = "888888";
		}

		strcpy(LoginField.UserID, FuserID.c_str());
		strcpy(LoginField.Password, pswd.c_str());
		//����������Ӧ����
		CTradeSpi tradeSpi(pTradeApi, &LoginField);
		strcpy(tradeSpi.InvestorID_Future, FuserID.c_str());
		strcpy(tradeSpi.InvestorID_Spot, TuserID.c_str());
		//����������Ӧ����
		CMdSpi mdSpi(pMdApi, &LoginField);

		//ע����Ӧ����
		pTradeApi->RegisterSpi(&tradeSpi);
		pTradeApi->SubscribePrivateTopic(THOST_TERT_QUICK);
		pMdApi->RegisterSpi(&mdSpi);

		//ע�ύ��ip
		pTradeApi->RegisterFront(tradeIp);
		//ע������ip
		pMdApi->RegisterFront(quotIp);

		//init
		std::cout << "traderApi init...\n";
		pTradeApi->Init();

		//Sleep(3000);
		std::cout << "mdApi init ...\n";
		pMdApi->Init();

		Sleep(5000);
		//tradeSpi.QryInvestorId();
		//Sleep(2000);
		int dbport = 3306;
		std::string dbip("192.168.1.201"), dbusr("root"), dbpswd("ct_1234"), lib("");
		g_pDb = new dbman(dbip, dbport, dbusr, dbpswd, lib);
		g_pDb->init();

		zc::Arbitrage::initArbi(&mdSpi, &tradeSpi);

		Sleep(2000);
		std::thread tickThread(TickFunc, &mdSpi);

		std::thread ordThread(orderFunc, &tradeSpi);

		int fnum = 0;
		int rnum = 10000;
		string fname;
		string dtime, tem_str;
		const char *cdime;
		CThostFtdcInputOrderField InputOrder;
		memset(&InputOrder, 0, sizeof(InputOrder));
		strncpy(InputOrder.BrokerID, LoginField.BrokerID, sizeof(InputOrder.BrokerID));//��Ա��
		strncpy(InputOrder.UserID, LoginField.UserID, sizeof(InputOrder.UserID));//����Ա
		dispMenu();
		int cmd;
		while (1)
		{
			getCmd(cmd);
			std::cout << "\n";
			if (0 == cmd)
			{
				std::cout << "exiting......\n";
				tradeSpi.quit();
				ordThread.join();
				mdSpi.quit();
				tickThread.join();
				std::cout << "exit......\n";
				Sleep(2000);
				return 0;
			}
			execute(cmd, mdSpi, tradeSpi);
			Sleep(1000);
		}
		delete g_pDb;
		g_pDb = nullptr;
	}
	catch (...)
	{
		std::cout << "catch error!\n";
	}
	return 0;
}

int ReqQrySettlementInfo(CThostFtdcTraderApi* pTradeApi)
{
	printf("������ȷ��...\n");

	fstech::CThostFtdcSettlementInfoConfirmField settlementInfoConfirm;
	memset(&settlementInfoConfirm, 0, sizeof(settlementInfoConfirm));
	return pTradeApi->ReqSettlementInfoConfirm(&settlementInfoConfirm, GetRequsetID());
}

int myReqOrderInsert(CThostFtdcTraderApi* pTradeApi, long iOrderRef, CThostFtdcInputOrderField InputOrder)
{
	cout << "�µ�: " << endl;
	sprintf(InputOrder.OrderRef, "%012ld", iOrderRef);//���ر�����
	cout << InputOrder.OrderRef << endl;
	InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Open;//����
	InputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	InputOrder.Direction = THOST_FTDC_D_Buy;
	InputOrder.LimitPrice = 279.0;
	InputOrder.VolumeTotalOriginal = 1;
	InputOrder.MinVolume = 1;
	InputOrder.VolumeTotalOriginal = 1;
	InputOrder.TimeCondition = THOST_FTDC_TC_GFD;//������Ч
	///�ɽ�������
	InputOrder.VolumeCondition = THOST_FTDC_VC_AV;//�κ�����
	///��С�ɽ���
	InputOrder.MinVolume = 1;
	///��������
	InputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;//����
	///ֹ���
	InputOrder.StopPrice = 0.0;
	///ǿƽԭ��
	InputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;//��ǿƽ
	///�Զ������־
	InputOrder.IsAutoSuspend = 0;
	///ҵ��Ԫ
	sprintf(InputOrder.BusinessUnit, "%s", "");
	///������
	InputOrder.RequestID = GetRequsetID();
	///�û�ǿƽ��־
	InputOrder.UserForceClose = 0;
	///��������־
	InputOrder.IsSwapOrder = 0;
	int ret = pTradeApi->ReqOrderInsert(&InputOrder, InputOrder.RequestID);
	if (0 != ret)
	{
		printf("ί��ʧ��!\n");
	}
	return ret;
}

int ReqOrderInsert(CThostFtdcTraderApi* pTradeApi, long iOrderRef)
{
	printf("����...\n");
	CThostFtdcInputOrderField InputOrder;
	memset(&InputOrder, 0, sizeof(InputOrder));

	CThostFtdcReqUserLoginField LoginField;
	memset(&LoginField, 0, sizeof(LoginField));
	CGetParam getParam;
	getParam.GetLoginField(LoginField);
	strncpy(InputOrder.BrokerID, LoginField.BrokerID, sizeof(InputOrder.BrokerID));//��Ա��
	strncpy(InputOrder.UserID, LoginField.UserID, sizeof(InputOrder.UserID));//����Ա

	getParam.GetOrderInsertField(InputOrder);//�ʽ��˺�,��Լ����,�۸�,����
	cout << "before: " << iOrderRef << endl;
	sprintf(InputOrder.OrderRef, "%012ld", iOrderRef);//���ر�����
	cout << "sprintf: " << InputOrder.OrderRef << endl;
	//InputOrder.Direction = THOST_FTDC_D_Buy;//����
	//InputOrder.CombHedgeFlag[0] = '4';//THOST_FTDC_HF_Speculation;//Ͷ��
	InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Close;//��ƽ

	InputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	InputOrder.VolumeTotalOriginal = 1;
	InputOrder.MinVolume = 1;
	///��Ч������
	InputOrder.TimeCondition = THOST_FTDC_TC_GFD;//������Ч
	///GTD����
	//sprintf(InputOrder.GTDDate, "%s", pTradeDate);
	///�ɽ�������
	InputOrder.VolumeCondition = THOST_FTDC_VC_AV;//�κ�����
	///��С�ɽ���
	InputOrder.MinVolume = 1;
	///��������
	InputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;//����
	///ֹ���
	InputOrder.StopPrice = 0.0;
	///ǿƽԭ��
	InputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;//��ǿƽ
	///�Զ������־
	InputOrder.IsAutoSuspend = 0;
	///ҵ��Ԫ
	sprintf(InputOrder.BusinessUnit, "%s", "");
	///������
	InputOrder.RequestID = GetRequsetID();
	///�û�ǿƽ��־
	InputOrder.UserForceClose = 0;
	///��������־
	InputOrder.IsSwapOrder = 0;
	int ret = pTradeApi->ReqOrderInsert(&InputOrder, InputOrder.RequestID);
	if (0 != ret)
	{
		printf("ί��ʧ��!\n");
	}
	return ret;
}

/*
int ReqOrderAction(CTradeSpi* pTradeSpi)
{
pTradeSpi->iOrderAction += 1;
return 0;
}

int ReqQryTrade(CThostFtdcTraderApi* pTradeApi)
{
printf("�ɽ���ѯ...\n");
CThostFtdcQryTradeField QryTrade;
memset(&QryTrade, 0, sizeof(QryTrade));
//strncpy_s(QryTrade.BrokerID,pBrokerID,sizeof(QryTrade.BrokerID));
//strncpy_s(QryTrade.InvestorID ,pInvestorID,sizeof(QryTrade.InvestorID));
//strncpy_s(QryTrade.InstrumentID,"IF1509",sizeof(QryTrade.InstrumentID));
//strncpy_s(QryTrade.ExchangeID,pExchang,sizeof(QryTrade.ExchangeID));
////strncpy_s(QryTrade.TradeID,pTradeDate,sizeof(QryTrade.TradeID));
//strncpy_s(QryTrade.TradeTimeStart,"",sizeof(QryTrade.TradeTimeStart));
//strncpy_s(QryTrade.TradeTimeEnd,"",sizeof(QryTrade.TradeTimeEnd));
return pTradeApi->ReqQryTrade(&QryTrade, GetRequsetID());
}


int ReqQryOrder(CThostFtdcTraderApi* pTradeApi)
{
printf("ί�в�ѯ...\n");

CThostFtdcQryOrderField QryOrder;
memset(&QryOrder, 0, sizeof(QryOrder));
//����½ʱ��д�ı���һ��,���������������е�brokerID����"";
strncpy(QryOrder.BrokerID, "", sizeof(QryOrder.BrokerID));
//strncpy_s(QryOrder.InstrumentID,pInstrument,sizeof(QryOrder.InstrumentID));
//strncpy_s(QryOrder.InvestorID,pInvestorID,sizeof(QryOrder.InvestorID));
//strncpy_s(QryOrder.ExchangeID ,pExchang,sizeof(QryOrder.ExchangeID));
//strncpy_s(QryOrder.InsertTimeStart,"",sizeof(QryOrder.InsertTimeStart));
//strncpy_s(QryOrder.InsertTimeEnd,"",sizeof(QryOrder.InsertTimeEnd));
return pTradeApi->ReqQryOrder(&QryOrder, GetRequsetID());
}
*/

int ReqQryInvestorPosition(CThostFtdcTraderApi* pTradeApi)
{
	CThostFtdcQryInvestorPositionField QryInvestorPosition;
	memset(&QryInvestorPosition, 0, sizeof(QryInvestorPosition));
	/*strncpy_s(QryInvestorPosition.InstrumentID,"",sizeof(QryInvestorPosition.InstrumentID));
	strncpy_s(QryInvestorPosition.InvestorID,pInvestorID,sizeof(QryInvestorPosition.InvestorID));
	strncpy_s(QryInvestorPosition.BrokerID,pBrokerID,sizeof(QryInvestorPosition.BrokerID));*/
	return pTradeApi->ReqQryInvestorPosition(&QryInvestorPosition, GetRequsetID());
}

int ReqQryTradingCode(CThostFtdcTraderApi* pTradeApi)
{
	printf("���ױ����ѯ...\n");
	CThostFtdcQryTradingCodeField QryTradingCode;
	memset(&QryTradingCode, 0, sizeof(QryTradingCode));
	/*strncpy_s(QryTradingCode.BrokerID,pBrokerID,sizeof(QryTradingCode.BrokerID));
	strncpy_s(QryTradingCode.InvestorID,pInvestorID,sizeof(QryTradingCode.InvestorID));
	strncpy_s(QryTradingCode.ExchangeID,pExchang,sizeof(QryTradingCode.ExchangeID));*/
	return pTradeApi->ReqQryTradingCode(&QryTradingCode, GetRequsetID());
}

int ReqQryInstrumentMarginRate(CThostFtdcTraderApi* pTradeApi)
{
	///�����ѯ��Լ��֤����
	printf("�����ѯ��Լ��֤����...\n");
	CThostFtdcQryInstrumentMarginRateField QryInstrumentMarginRate;
	memset(&QryInstrumentMarginRate, 0, sizeof(CThostFtdcQryInstrumentMarginRateField));
	strncpy(QryInstrumentMarginRate.InvestorID, "0800003", sizeof("0800003"));
	/*strncpy_s(QryInstrumentMarginRate.BrokerID,pBrokerID,sizeof(QryInstrumentMarginRate.BrokerID));
	strncpy_s(QryInstrumentMarginRate.InvestorID,pInvestorID,sizeof(QryInstrumentMarginRate.InvestorID));
	strncpy_s(QryInstrumentMarginRate.InstrumentID,pInstrument,sizeof(QryInstrumentMarginRate.InstrumentID));
	QryInstrumentMarginRate.HedgeFlag = THOST_FTDC_HF_Speculation;*/
	return pTradeApi->ReqQryInstrumentMarginRate(&QryInstrumentMarginRate, GetRequsetID());
}

int ReqQryInstrumentCommissionRate(CThostFtdcTraderApi* pTradeApi)
{
	//�����ѯ��Լ��������
	printf("�����ѯ��Լ��������...\n");
	CThostFtdcQryInstrumentCommissionRateField QryInstrumentCommissionRate;
	memset(&QryInstrumentCommissionRate, 0, sizeof(CThostFtdcQryInstrumentCommissionRateField));
	strncpy(QryInstrumentCommissionRate.InvestorID, "0800003", sizeof("0800003"));
	/*strncpy_s(QryInstrumentCommissionRate.BrokerID,pBrokerID,sizeof(QryInstrumentCommissionRate.BrokerID));
	strncpy_s(QryInstrumentCommissionRate.InvestorID,pInvestorID,sizeof(QryInstrumentCommissionRate.InvestorID));
	strncpy_s(QryInstrumentCommissionRate.InstrumentID,pInstrument,sizeof(QryInstrumentCommissionRate.InstrumentID));*/
	return pTradeApi->ReqQryInstrumentCommissionRate(&QryInstrumentCommissionRate, GetRequsetID());
}

int ReqQryPositionDetail(CThostFtdcTraderApi* pTradeApi)
{
	CThostFtdcQryInvestorPositionDetailField req;
	memset(&req, 0, sizeof(req));
	return pTradeApi->ReqQryInvestorPositionDetail(&req, GetRequsetID());
}

int ReqQrySettlementConfirm(CThostFtdcTraderApi* pTradeApi)
{
	CThostFtdcQrySettlementInfoConfirmField req;
	memset(&req, 0, sizeof(req));
	return pTradeApi->ReqQrySettlementInfoConfirm(&req, GetRequsetID());
}

int ReqQryNotice(CThostFtdcTraderApi* pTradeApi)
{
	CThostFtdcQryNoticeField req;
	memset(&req, 0, sizeof(req));
	return pTradeApi->ReqQryNotice(&req, GetRequsetID());
}

int ReqQryComPositionDetail(CThostFtdcTraderApi* pTradeApi)
{
	CThostFtdcQryInvestorPositionCombineDetailField req;
	memset(&req, 0, sizeof(req));
	return pTradeApi->ReqQryInvestorPositionCombineDetail(&req, GetRequsetID());
}

int ReqQryTradeParam(CThostFtdcTraderApi* pTradeApi)
{
	CThostFtdcQryBrokerTradingParamsField req;
	memset(&req, 0, sizeof(req));
	return pTradeApi->ReqQryBrokerTradingParams(&req, GetRequsetID());
}

int UnsubQuot(CThostFtdcMdApi* pMdApi)
{
	char * pInstrument[] = {
		"ag1712",
		"au1712",
		"Ag(T+D)",
		"Au(T+D)",
		"mAu(T+D)"
	};
	return pMdApi->UnSubscribeMarketData(pInstrument, 2);
}

namespace zc
{
	InstumentInfo& InstruInfo(char* iss)
	{
		static InstumentInfo a;
		return a;
	}
	std::string tradedir(int dir)
	{
		if (dir == zc::TRADE_DIR::EM_Long)return "long";
		else return "short";
	}
}