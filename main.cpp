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
// 订阅行情
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

// 行情线程
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
		// 处理多个套利对
		for (auto it = zc::Arbitrage::ArbiTrades.begin(); it != zc::Arbitrage::ArbiTrades.end(); ++it)
		{
			it->UpdateArbiOrd(); // 刷新母单状态 生成子单订单，即下单计划下达
		}
		// 具体订单执行层
		// 条件单处理，只需要按照指令傻傻执行即可
		// 已经成交的，有成交触发动作的就执行成交触发动作
		// 送单失败的记录后重新发送
		// 超时未成的撤单重发
		// 条件单模式：定时发单，超时撤单，成交后触发新单
		while (InterlockedExchange64(&(zc::Arbitrage::spin_Locker_ordbook), TRUE)){ Sleep(0); }
		for (auto it = zc::Arbitrage::ordbook.begin(); it != zc::Arbitrage::ordbook.end(); ++it)
		{
			// 状态更新
			// zc::Arbitrage::UpdateSubOrdStatus();
			// 状态已经更新
			// 处理新单
			switch ((*it)->condition)
			{
			case zc::LEG_CONDITION::EM_OK: // 可以马上送单的
				// 待送单
				if (zc::LEG_STATUS::EM_LEG_SENDREADY == (*it)->status)
				{
					// Send后面必须设置status，否则会重复送单
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

					(*it)->status = zc::LEG_STATUS::EM_LEG_SENDED; // EM_LEG_SENDED暂态->EM_LEG_ORDERED
					// Send后面必须设置status，否则会重复送单
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
				// 等待
				//(*it)->curtime = zc::GetCurTime();
				/*if ((*it)->arbiOrd->sendDelay>0 && (*it)->curtime - (*it)->ordCreatedTime > (*it)->timeout)
				{
				// 等待时间到
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
	zc::RecieveInput("\ninput your cmd:  ", cmd, [](int& in)->bool{if (in < 0 || in>9)return false; else return true; });
}

void disppairs()
{
	std::cout << "Current Arbitrage Pairs:\n" << std::endl;
	int ns = zc::Arbitrage::ArbiTrades.size();
	for (int i = 0; i < ns; i++)
	{
		std::cout << "\t[" << i << "]" << zc::Arbitrage::ArbiTrades[i].ArbiInst.ArbiInstID << std::endl;
	}
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
				zc::Arbitrage::ArbiTrades[ind].sellshort(1, 30);
			}
			else
			{
				zc::Arbitrage::ArbiTrades[ind].sell(1, 30);
			}
		}

		break;
	case 3:
		std::cout << "Notice: Auto trading will be enabled!" << std::endl;
		zc::Arbitrage::AutoTradingEnabled = !zc::Arbitrage::AutoTradingEnabled;
		break;
	case 4:
		dispspread();
		break;
	case 5:
		std::cout << "Current Arbitrage Pairs:\n";
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
		std::cout << "Current Arbitrage Pairs:\n";
		disppairs();
		ns = zc::Arbitrage::ArbiTrades.size();
		zc::RecieveInput("\nwhich to be modified:  ", ind, [ns](int& in)->bool{return in < ns; });

		std::cout << "Lower1: " << zc::Arbitrage::ArbiTrades[ind].ArbiInst.getSpreadLower1() << std::endl;
		std::cout << "Upper1: " << zc::Arbitrage::ArbiTrades[ind].ArbiInst.getSpreadUpper1() << std::endl;
		std::cout << "Lower2: " << zc::Arbitrage::ArbiTrades[ind].ArbiInst.getSpreadLower2() << std::endl;
		std::cout << "Upper2: " << zc::Arbitrage::ArbiTrades[ind].ArbiInst.getSpreadUpper2() << std::endl;
		break;
	case 99:
		CThostFtdcUserPasswordUpdateField udpw;
		strncpy(udpw.UserID, "08000014", sizeof(TThostFtdcUserIDType));
		strncpy(udpw.OldPassword, "666888", sizeof(TThostFtdcPasswordType));
		strncpy(udpw.NewPassword, "501947", sizeof(TThostFtdcPasswordType));
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
	std::cout << "############黄金白银期现套利系统###########\n";
	CGetParam getParam;

	//创建交易api对象
	CThostFtdcTraderApi* pTradeApi = CThostFtdcTraderApi::CreateFtdcTraderApi("");
	//创建行情api对象
	CThostFtdcMdApi* pMdApi = CThostFtdcMdApi::CreateFtdcMdApi();

	//获取登陆参数
	CThostFtdcReqUserLoginField LoginField;
	memset(&LoginField, 0, sizeof(LoginField));
	//	getParam.GetLoginField(LoginField);
	//	strcpy(LoginField.UserID, "08000037");
	//	strcpy(LoginField.Password, "888888");
	//	strcpy(LoginField.BrokerID, "");
	strcpy(LoginField.UserID, "08000014");
	strcpy(LoginField.Password, "666888");
	//创建交易响应对象
	CTradeSpi tradeSpi(pTradeApi, &LoginField);
	strcpy(tradeSpi.InvestorID_Future, "08000014");
	strcpy(tradeSpi.InvestorID_Spot, "06000014");
	//创建行情响应对象
	CMdSpi mdSpi(pMdApi, &LoginField);

	//注册响应对象
	pTradeApi->RegisterSpi(&tradeSpi);
	pTradeApi->SubscribePrivateTopic(THOST_TERT_QUICK);
	pMdApi->RegisterSpi(&mdSpi);

	//获取ip
	char tradeIp[36];
	char quotIp[36];
	//	memset(tradeIp,0,sizeof(tradeIp));
	//	memset(quotIp,0,sizeof(quotIp));
	//	int i = getParam.GetIpField(tradeIp,quotIp);
	//	strcpy(tradeIp,"tcp://192.168.1.206:37776");
	//	strcpy(quotIp,"tcp://192.168.1.206:37777");
	//	strcpy(tradeIp, "tcp://140.206.81.6:27776");
	//	strcpy(quotIp, "tcp://140.206.81.6:27777");
	strcpy(tradeIp, "tcp://140.206.81.6:27776");
	strcpy(quotIp, "tcp://140.206.81.6:27777");
	//注册交易ip
	pTradeApi->RegisterFront(tradeIp);
	//注册行情ip
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
	std::string ip("192.168.1.201"), usr("root"), pswd("ct_1234"), lib("");
	g_pDb = new dbman(ip, dbport, usr, pswd, lib);
	g_pDb->init();

	zc::Arbitrage::initArbi(&mdSpi, &tradeSpi);

	Sleep(2000);
	std::thread tickThread(TickFunc, &mdSpi);

	std::thread ordThread(orderFunc, &tradeSpi);

	//登陆在OnConnect()函数里进行.
#ifdef _USE_MYSQL_
	MYSQL *conn;
	MYSQL *conn1;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int num_fields;
	int num_rows;
	int i;
	conn = mysql_init(NULL);
	if (conn == NULL)
	{
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	conn1 = mysql_init(NULL);
	if (conn1 == NULL)
	{
		printf("Error %u: %s\n", mysql_errno(conn1), mysql_error(conn1));
		exit(1);
	}
	if (mysql_real_connect(conn, "192.168.1.201", "root", "ct_1234", "trade", 3306, NULL, 0) == NULL)
	{
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	if (mysql_real_connect(conn1, "192.168.1.201", "root", "ct_1234", "auag", 3306, NULL, 0) == NULL)
	{
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
#endif

	time_t rawtime;
	time(&rawtime);
	char tbname[12] = "now_data";
	char tbnamehq[30] = "fs_";
	char   pblgtime[20];
	strftime(pblgtime, 20, "%Y%m%d", localtime(&rawtime));
	strcat(tbnamehq, pblgtime);
	cout << "strftime   " << pblgtime << endl;
#ifdef _USE_MYSQL_
	char sql[1000];
	if (!isindb(conn, tbname))
	{
		sprintf(sql, "CREATE TABLE %s (id INT AUTO_INCREMENT PRIMARY KEY,date_time DATETIME,code VARCHAR(12),lastprice DOUBLE,\
					 					 					 volume DOUBLE,bid1 DOUBLE,bidvolume1 DOUBLE,ask1 DOUBLE,askvolume1 DOUBLE)", tbname);
		mysql_query(conn, sql);
		sprintf(sql, "INSERT INTO %s (code) VALUES(%s)", tbname, "\'Ag(T+D)\'");
		mysql_query(conn, sql);
		sprintf(sql, "INSERT INTO %s (code) VALUES(%s)", tbname, "\'Au(T+D)\'");
		mysql_query(conn, sql);
		sprintf(sql, "INSERT INTO %s (code) VALUES(%s)", tbname, "\'Ag1712\'");
		mysql_query(conn, sql);
		sprintf(sql, "INSERT INTO %s (code) VALUES(%s)", tbname, "\'Au1712\'");
		mysql_query(conn, sql);
		sprintf(sql, "INSERT INTO %s (code) VALUES(%s)", tbname, "\'mAu(T+D)\'");
		mysql_query(conn, sql);
		/*
		if (!isindb(conn1, tbnamehq))
		{
		sprintf(sql, "CREATE TABLE %s (id INT AUTO_INCREMENT PRIMARY KEY,date_time DATETIME,InstrumentID VARCHAR(12),LastPrice DOUBLE,\
		Volume DOUBLE,Turnover DOUBLE,OpenInterest DOUBLE,BidPrice1 DOUBLE,BidVolume1 DOUBLE,AskPrice1 DOUBLE,AskVolume1 DOUBLE)", tbnamehq);
		mysql_query(conn1, sql);
		}
		ofstream outf;
		outf.open(".\\out\\0.csv", ios::out);
		outf << "TradingDay" << "," << "InstrumentID" << "," << "LastPrice" << "," << "PreSettlementPrice" << "," << \
		"PreClosePrice" << "," << "OpenPrice" << "," << "HighestPrice" << "," << "LowestPrice" << "," << "Volume" << "," << \
		"Turnover" << "," << "OpenInterest" << "," << "SettlementPrice" << "," << "UpdateTime" << "," << "UpdateMillisec" << "," << \
		"BidPrice1" << "," << "BidVolume1" << "," << "AskPrice1" << "," << "AskVolume1" << "," << "AveragePrice" << "," << \
		"ActionDay" << endl;*/
	}
#endif

	int fnum = 0;
	int rnum = 10000;
	string fname;
	string dtime, tem_str;
	const char *cdime;
	CThostFtdcInputOrderField InputOrder;
	memset(&InputOrder, 0, sizeof(InputOrder));
	strncpy(InputOrder.BrokerID, LoginField.BrokerID, sizeof(InputOrder.BrokerID));//会员号
	strncpy(InputOrder.UserID, LoginField.UserID, sizeof(InputOrder.UserID));//交易员

	int cmd;
	while (1)
	{
		dispMenu();
		getCmd(cmd);
		std::cout << "\n";
		if (9 == cmd)
		{
			continue;
		}
		if (0 == cmd)
		{
			std::cout << "exiting......\n";
			tradeSpi.quit();
			//ordThread.join();
			mdSpi.quit();
			//tickThread.join();

			std::cout << "exit......\n";
			return 0;
		}
		execute(cmd, mdSpi, tradeSpi);
		Sleep(3000);
#ifdef _USE_MYSQL_			
		if (strlen(mdSpi.mTradingDay) == 8)
		{
			//	cout << count << " " << mdSpi.mTradingDay << " " << mdSpi.mUpdateTime << " " << mdSpi.mInstrumentID << " " << \
																	mdSpi.mLastPrice << " " << mdSpi.mVolume << " " << mdSpi.mBidPrice1 << " " << mdSpi.mBidVolume1 << " " << \
																	mdSpi.mAskPrice1 << " " << mdSpi.mAskVolume1 << endl;
			tem_str = mdSpi.mTradingDay;
			dtime = tem_str.substr(0, 4) + '-' + tem_str.substr(4, 2) + '-' + tem_str.substr(6, 2) + ' ' + mdSpi.mUpdateTime;
			cdime = dtime.c_str();
			//			cout << dtime << " " << mdSpi.mInstrumentID << " " << cdime << endl;

			sprintf(sql, "UPDATE %s SET date_time = \'%s\',lastprice = %f,volume = %d,bid1 = %f,bidvolume1 = %d,ask1 = %f,askvolume1 = %d WHERE code = \'%s\' ", \
				tbname, cdime, mdSpi.mLastPrice, mdSpi.mVolume, mdSpi.mBidPrice1, mdSpi.mBidVolume1, mdSpi.mAskPrice1, \
				mdSpi.mAskVolume1, mdSpi.mInstrumentID);
			//			cout << sql << endl;
			mysql_query(conn, sql);
		}
#endif
	}
	return 0;
}

#ifdef _USE_MYSQL_
bool isindb(MYSQL *conn, char *tbname)
{
	bool status = false;
	MYSQL_RES *result;
	MYSQL_ROW row;
	mysql_query(conn, "show tables");
	result = mysql_store_result(conn);
	int num_fields = mysql_num_fields(result);
	int i;
	while ((row = mysql_fetch_row(result)))
	{
		for (i = 0; i < num_fields; i++)
		{
			if (strcmp(row[i], tbname) == 0)
				status = true;
		}
	}
	return status;
}
#endif

int ReqQrySettlementInfo(CThostFtdcTraderApi* pTradeApi)
{
	printf("结算结果确认...\n");

	fstech::CThostFtdcSettlementInfoConfirmField settlementInfoConfirm;
	memset(&settlementInfoConfirm, 0, sizeof(settlementInfoConfirm));
	return pTradeApi->ReqSettlementInfoConfirm(&settlementInfoConfirm, GetRequsetID());
}

int myReqOrderInsert(CThostFtdcTraderApi* pTradeApi, long iOrderRef, CThostFtdcInputOrderField InputOrder)
{
	cout << "下单: " << endl;
	sprintf(InputOrder.OrderRef, "%012ld", iOrderRef);//本地保单号
	cout << InputOrder.OrderRef << endl;
	InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Open;//开仓
	InputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	InputOrder.Direction = THOST_FTDC_D_Buy;
	InputOrder.LimitPrice = 279.0;
	InputOrder.VolumeTotalOriginal = 1;
	InputOrder.MinVolume = 1;
	InputOrder.VolumeTotalOriginal = 1;
	InputOrder.TimeCondition = THOST_FTDC_TC_GFD;//当日有效
	///成交量类型
	InputOrder.VolumeCondition = THOST_FTDC_VC_AV;//任何数量
	///最小成交量
	InputOrder.MinVolume = 1;
	///触发条件
	InputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;//立即
	///止损价
	InputOrder.StopPrice = 0.0;
	///强平原因
	InputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;//非强平
	///自动挂起标志
	InputOrder.IsAutoSuspend = 0;
	///业务单元
	sprintf(InputOrder.BusinessUnit, "%s", "");
	///请求编号
	InputOrder.RequestID = GetRequsetID();
	///用户强平标志
	InputOrder.UserForceClose = 0;
	///互换单标志
	InputOrder.IsSwapOrder = 0;
	int ret = pTradeApi->ReqOrderInsert(&InputOrder, InputOrder.RequestID);
	if (0 != ret)
	{
		printf("委托失败!\n");
	}
	return ret;
}

int ReqOrderInsert(CThostFtdcTraderApi* pTradeApi, long iOrderRef)
{
	printf("报单...\n");
	CThostFtdcInputOrderField InputOrder;
	memset(&InputOrder, 0, sizeof(InputOrder));

	CThostFtdcReqUserLoginField LoginField;
	memset(&LoginField, 0, sizeof(LoginField));
	CGetParam getParam;
	getParam.GetLoginField(LoginField);
	strncpy(InputOrder.BrokerID, LoginField.BrokerID, sizeof(InputOrder.BrokerID));//会员号
	strncpy(InputOrder.UserID, LoginField.UserID, sizeof(InputOrder.UserID));//交易员

	getParam.GetOrderInsertField(InputOrder);//资金账号,合约代码,价格,数量
	cout << "before: " << iOrderRef << endl;
	sprintf(InputOrder.OrderRef, "%012ld", iOrderRef);//本地保单号
	cout << "sprintf: " << InputOrder.OrderRef << endl;
	//InputOrder.Direction = THOST_FTDC_D_Buy;//买卖
	//InputOrder.CombHedgeFlag[0] = '4';//THOST_FTDC_HF_Speculation;//投机
	InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Close;//开平

	InputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	InputOrder.VolumeTotalOriginal = 1;
	InputOrder.MinVolume = 1;
	///有效期类型
	InputOrder.TimeCondition = THOST_FTDC_TC_GFD;//当日有效
	///GTD日期
	//sprintf(InputOrder.GTDDate, "%s", pTradeDate);
	///成交量类型
	InputOrder.VolumeCondition = THOST_FTDC_VC_AV;//任何数量
	///最小成交量
	InputOrder.MinVolume = 1;
	///触发条件
	InputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;//立即
	///止损价
	InputOrder.StopPrice = 0.0;
	///强平原因
	InputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;//非强平
	///自动挂起标志
	InputOrder.IsAutoSuspend = 0;
	///业务单元
	sprintf(InputOrder.BusinessUnit, "%s", "");
	///请求编号
	InputOrder.RequestID = GetRequsetID();
	///用户强平标志
	InputOrder.UserForceClose = 0;
	///互换单标志
	InputOrder.IsSwapOrder = 0;
	int ret = pTradeApi->ReqOrderInsert(&InputOrder, InputOrder.RequestID);
	if (0 != ret)
	{
		printf("委托失败!\n");
	}
	return ret;
}

int ReqOrderAction(CTradeSpi* pTradeSpi)
{
	pTradeSpi->iOrderAction += 1;
	return 0;
}

int ReqQryTrade(CThostFtdcTraderApi* pTradeApi)
{
	printf("成交查询...\n");
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
	printf("委托查询...\n");

	CThostFtdcQryOrderField QryOrder;
	memset(&QryOrder, 0, sizeof(QryOrder));
	//跟登陆时填写的保持一致,本程序所有请求中的brokerID都是"";
	strncpy(QryOrder.BrokerID, "", sizeof(QryOrder.BrokerID));
	//strncpy_s(QryOrder.InstrumentID,pInstrument,sizeof(QryOrder.InstrumentID));
	//strncpy_s(QryOrder.InvestorID,pInvestorID,sizeof(QryOrder.InvestorID));
	//strncpy_s(QryOrder.ExchangeID ,pExchang,sizeof(QryOrder.ExchangeID));
	//strncpy_s(QryOrder.InsertTimeStart,"",sizeof(QryOrder.InsertTimeStart));
	//strncpy_s(QryOrder.InsertTimeEnd,"",sizeof(QryOrder.InsertTimeEnd));
	return pTradeApi->ReqQryOrder(&QryOrder, GetRequsetID());
}

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
	printf("交易编码查询...\n");
	CThostFtdcQryTradingCodeField QryTradingCode;
	memset(&QryTradingCode, 0, sizeof(QryTradingCode));
	/*strncpy_s(QryTradingCode.BrokerID,pBrokerID,sizeof(QryTradingCode.BrokerID));
	strncpy_s(QryTradingCode.InvestorID,pInvestorID,sizeof(QryTradingCode.InvestorID));
	strncpy_s(QryTradingCode.ExchangeID,pExchang,sizeof(QryTradingCode.ExchangeID));*/
	return pTradeApi->ReqQryTradingCode(&QryTradingCode, GetRequsetID());
}

int ReqQryInstrumentMarginRate(CThostFtdcTraderApi* pTradeApi)
{
	///请求查询合约保证金率
	printf("请求查询合约保证金率...\n");
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
	//请求查询合约手续费率
	printf("请求查询合约手续费率...\n");
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