#ifndef _SGITMDSPI_H_DQK_20160602
#define _SGITMDSPI_H_DQK_20160602

#include "../s_include/SgitFtdcMdApi.h"

#ifdef WIN32
#ifndef _DEBUG
#pragma comment(lib,"../lib/sgitquotapi.lib")
#else
#pragma comment(lib,"../lib/sgitquotapi.lib")
#endif
#endif

#include "Arbitrage.h"

using namespace fstech;

class CMdSpi
	:public  CThostFtdcMdSpi
{
public:
	CMdSpi(CThostFtdcMdApi * pMdApi, CThostFtdcReqUserLoginField* pReqLogon);
	~CMdSpi();
	void quit();
public:
	int logon(CThostFtdcReqUserLoginField* pReqLogon);
	virtual void OnFrontConnected();
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) ;
	virtual void OnRtnDeferDeliveryQuot(CThostDeferDeliveryQuot* pQuot);

	///订阅行情应答
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) ;

	///取消订阅行情应答
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

private:
	CThostFtdcReqUserLoginField m_logonField;
	CThostFtdcMdApi* m_pApi;
	int tickscount;
public:
	int running;
};

#endif