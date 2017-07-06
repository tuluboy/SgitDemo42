#include "MdSpi.h"
#include "string.h"
#include "../s_log/log.h"
#include <iostream>
#include <fstream> 
#include "cirque.h"
#include <assert.h>
using namespace std;

extern char * pSubInstrumnet[5];

CMdSpi::CMdSpi(CThostFtdcMdApi * pMdApi, CThostFtdcReqUserLoginField* pReqLogon)
{
	m_pApi = pMdApi;
	memcpy(&m_logonField, pReqLogon, sizeof(m_logonField));
	running = 111;
	tickscount = 0;
}

CMdSpi::~CMdSpi()
{

}

int CMdSpi::logon(CThostFtdcReqUserLoginField* pReqLogon)
{
	if (NULL == pReqLogon)
		return -1;
	memcpy(&m_logonField, pReqLogon, sizeof(m_logonField));
	return 0;
}

void CMdSpi::OnFrontConnected()
{
	std::cout << "行情连接成功...\n";
	std::cout << "行情登陆...\n";
	m_pApi->ReqUserLogin(&m_logonField, 0);
	dqk_log::CLogApi log;
	log << "行情登录:" << m_logonField.UserID << "\n";
}

void CMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	// bConnected = false;
	dqk_log::CLogApi LOG;
	std::cout << "行情登录请求响应-OnRspUserLogin-" << nRequestID << ",bIsLast-" << bIsLast << "\n";
	if (NULL != pRspInfo)
	{
		if (0 == pRspInfo->ErrorID)
		{
			printf("行情登录成功.\n");
			printf("订阅行情...\n");
			int ret = m_pApi->SubscribeMarketData(pSubInstrumnet, sizeof(pSubInstrumnet) / sizeof(char*));
			if (ret != 0)
			{
				std::cout << "发送订阅行情指令失败...\n";
			}
		}
		else
		{
			printf("pRspInfo msg%d - %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
		std::cout << "CMdSpi::OnRspUserLogin:errcode-" << pRspInfo->ErrorID << ",msg-" << pRspInfo->ErrorMsg << "\n";
	}
}

void CMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	//	 dqk_log::CLogApi LOG;
	//	 LOG<<"OnRtnDepthMarketData\n";
	//	 cout << "chengyj:　"<<pDepthMarketData->InstrumentID << " " << pDepthMarketData->LastPrice << endl;
	//if ((++tickscount) % 10 == 1)std::cout << "tick data ok.....\n";
	if (!zc::Arbitrage::ticks.write(pDepthMarketData))
	{
		std::cout << "ticks buf overflow......\n";
	}
	////printf("ticks.size： %d\n", ticks.GetSize());
}

void CMdSpi::OnRtnDeferDeliveryQuot(CThostDeferDeliveryQuot* pQuot)
{
	dqk_log::CLogApi LOG;
	LOG << "OnRtnDeferDeliveryQuot-" << pQuot << "\n";
	if (NULL != pQuot)
	{
		LOG << pQuot->InstrumentID << ","
			<< pQuot->AskVolume << ","
			<< pQuot->BidVolume << ","
			<< pQuot->MidAskVolume << ","
			<< pQuot->MidBidVolume << "\n"
			;
	}

}

///订阅行情应答
void CMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	dqk_log::CLogApi LOG;
	std::cout << "OnRspSubMarketData-" << nRequestID << ",blast-" << bIsLast << "\n";
	if (NULL != pRspInfo)
	{
		LOG << "pRspInfo:errorid=" << pRspInfo->ErrorID <<",errorMsg=" << pRspInfo->ErrorMsg << "\n";
		std::cout << "pRspInfo:errorid=" << pRspInfo->ErrorID << ",errorMsg=" << pRspInfo->ErrorMsg << "\n";
	}
	if (NULL != pSpecificInstrument)
	{
		LOG << "pSpecificInstrument:" << pSpecificInstrument->InstrumentID << "\n";
		std::cout << "pSpecificInstrument:" << pSpecificInstrument->InstrumentID << "\n";
	}
	std::cout << "行情订阅成功...\n";
}

///取消订阅行情应答
void CMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	dqk_log::CLogApi LOG;
	LOG << "OnRspUnSubMarketData-" << nRequestID << ",blast-" << bIsLast << "\n";
	if (NULL != pRspInfo)
	{
		LOG << "pRspInfo:errorid=" << pRspInfo->ErrorID <<
			",errorMsg=" << pRspInfo->ErrorMsg << "\n"
			;
	}
	if (NULL != pSpecificInstrument)
		LOG << "pSpecificInstrument:" << pSpecificInstrument->InstrumentID << "\n";
}

void CMdSpi::quit()
{
	running = 0;

}