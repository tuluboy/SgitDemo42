#include "SysLog.h"

#include "TradeSpi.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
//#include "../s_log/log.h"
#include "GetParam.h"
#include "Arbitrage.h"
#include <assert.h>
#include <algorithm>
#include "common.h"
#include <processthreadsapi.h>

extern char * pSubInstrumnet[7];
extern int GetRequsetID();
CTradeSpi::CTradeSpi(CThostFtdcTraderApi* pReqApi, CThostFtdcReqUserLoginField* pLoginField)
{
	iOrderAction = 0;
	m_pReqApi = pReqApi;
	if (0 != pLoginField)
		memcpy(&m_loginField, pLoginField, sizeof(m_loginField));
	else
		memset(&m_loginField, 0, sizeof(m_loginField));
	bFirstLogin = false;
	running = 111;
}


CTradeSpi::~CTradeSpi(void)
{
}

void CTradeSpi::OnFrontConnected(){
	int thid = GetCurrentThreadId();
	std::cout << "traderspi 连接成功...\n";
	LOG("traderspi 连接成功...\n");
	std::cout << "traderspi 登陆...\n";
	LOG("traderspi 登陆...\n");
	m_pReqApi->ReqUserLogin(&m_loginField, 0);
	LOG("登录:" << m_loginField.UserID << "\n");
};

///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
///@param nReason 错误原因
///        0x1001 网络读失败
///        0x1002 网络写失败
///        0x2001 接收心跳超时
///        0x2002 发送心跳失败
///        0x2003 收到错误报文
void CTradeSpi::OnFrontDisconnected(int nReason){
	int thid = GetCurrentThreadId();
	LOG( "OnFrontDisconnected" << "\n"<< "CTradeSpi::OnFrontDisconnected" << "\n");
	bConnected = false;
};

///心跳超时警告。当长时间未收到报文时，该方法被调用。
///@param nTimeLapse 距离上次接收报文的时间
void CTradeSpi::OnHeartBeatWarning(int nTimeLapse){};

///客户端认证响应
void CTradeSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};


///登录请求响应
void CTradeSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	bConnected = false;
	LOG("登录请求响应-OnRspUserLogin-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		if (0 == pRspInfo->ErrorID)
		{
			OrderRef = atol(pRspUserLogin->MaxOrderRef);
			LOG("交易登录成功 --" << "MaxOrderRef returned as value: " << OrderRef << "\n" );
			bConnected = true;
			if (false == bFirstLogin)
			{
				bFirstLogin = true;
				//查询交易编码
				//查询合约
			}
		}
		else
		{
			LOG("交易登录失败！\n");
			LOG("CTradeSpi::pRspInfo msg: " << pRspInfo->ErrorID << "  " << pRspInfo->ErrorMsg );
		}
		LOG("CTradeSpi::OnRspUserLogin:errcode-" << pRspInfo->ErrorID << ",msg-" << pRspInfo->ErrorMsg << "\n" );
	}

	if (NULL != pRspUserLogin)
	{
		//strncpy_s(CSystemData::tradingDay,pRspUserLogin->TradingDay,sizeof(CSystemData::tradingDay));
		//dqk_log::CLogApi g_Log << thid;
		LOG("pRspUserLogin:TradingDay-" << pRspUserLogin->TradingDay
			<< ",LoginTime-" << pRspUserLogin->LoginTime
			<< ",BrokerID-" << pRspUserLogin->BrokerID
			<< ",UserID-" << pRspUserLogin->UserID
			<< ",SystemName-" << pRspUserLogin->SystemName
			<< ",FrontID-" << pRspUserLogin->FrontID
			<< ",SessionID-" << pRspUserLogin->SessionID
			<< ",MaxOrderRef-" << pRspUserLogin->MaxOrderRef
			<< ",SHFETime-" << pRspUserLogin->SHFETime
			<< ",DCETime-" << pRspUserLogin->DCETime
			<< ",CZCETime-" << pRspUserLogin->CZCETime
			<< ",FFEXTime-" << pRspUserLogin->FFEXTime
			<< ",INETime-" << pRspUserLogin->INETime << "\n" );
	}
};

///登出请求响应
void CTradeSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	bConnected = false;
	LOG("登出请求响应-OnRspUserLogout" << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("pRspInfo:errcode-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n" );
	}
	if (NULL != pUserLogout)
	{
		LOG("pUserLogout:BrokerID-" << pUserLogout->BrokerID << ",UserID-" << pUserLogout->UserID << "\n" );
	}
};

///用户口令更新请求响应
void CTradeSpi::OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();

	//dqk_log::CLogApi g_Log << thid;
	LOG("用户口令更新请求响应-" << nRequestID << "\n" << "用户口令更新请求响应 - OnRspUserPasswordUpdate bIsLast - " << bIsLast << "\n" );
	if (NULL != pRspInfo)
	{
		LOG("MSG:errcode-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n" );
	}
	if (NULL != pUserPasswordUpdate)
	{
		LOG("pUserPasswordUpdate:BrokerID-" << pUserPasswordUpdate->BrokerID
			<< ",UserID-" << pUserPasswordUpdate->UserID
			<< ",OldPassword-" << pUserPasswordUpdate->OldPassword
			<< ",NewPassword-" << pUserPasswordUpdate->NewPassword
			<< ",NewPassword-" << pUserPasswordUpdate->NewPassword << "\n" );
	}
};


///资金账户口令更新请求响应
void CTradeSpi::OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
};


///报单录入请求响应
void CTradeSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	//printf("OnRspOrderInsert报单录入请求响应-%d.\n", nRequestID);
	//dqk_log::CLogApi g_Log << thid;
	//std::cout << "报单录入请求响应-OnRspOrderInsert-" << nRequestID << ",bIsLast-" << bIsLast << ",bIsLast-" << bIsLast << "\n";

	if (NULL != pRspInfo)
	{
		//printf("报单录入请求响应-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		//std::cout << "pRspInfo:msgCode-" << pRspInfo->ErrorID << ",msg-" << pRspInfo->ErrorMsg << "\n";
		if (0 == pRspInfo->ErrorID && 0 < iOrderAction)
		{
			CThostFtdcInputOrderActionField inputOrderAction;
			memset(&inputOrderAction, 0, sizeof(inputOrderAction));
			strcpy(inputOrderAction.UserID, pInputOrder->UserID);
			strcpy(inputOrderAction.InvestorID, pInputOrder->InvestorID);
			strcpy(inputOrderAction.InstrumentID, pInputOrder->InstrumentID);
			//本地报单号撤单
			strcpy(inputOrderAction.OrderRef, pInputOrder->OrderRef);
			//系统报单号撤单
			strcpy(inputOrderAction.OrderSysID, pInputOrder->OrderSysID);
			strcpy(inputOrderAction.ExchangeID, pInputOrder->ExchangeID);
			if (0 != m_pReqApi->ReqOrderAction(&inputOrderAction, 0))
				LOG("撤单发送失败" << "\n" );

		}
	}
	if (NULL != pInputOrder)
	{

		LOG("OnRspOrderInsert:BrokerID-" << pInputOrder->BrokerID
			<<",ExchangeID-"<<pInputOrder->ExchangeID
			//<< ",InvestorID-" << pInputOrder->InvestorID
			<< ",InstrumentID-" << pInputOrder->InstrumentID
			<< ",OrderRef-" << pInputOrder->OrderRef
			//<< ",UserID-" << pInputOrder->UserID
			//<< ",OrderPriceType-" << pInputOrder->OrderPriceType
			<< ",Direction-" << pInputOrder->Direction
			<< ",CombOffsetFlag-" << pInputOrder->CombOffsetFlag
			//<< ",CombHedgeFlag-" << pInputOrder->CombHedgeFlag
			<< ",LimitPrice-" << pInputOrder->LimitPrice
			<< ",VolumeTotalOriginal-" << pInputOrder->VolumeTotalOriginal
			//<< ",TimeCondition-" << pInputOrder->TimeCondition
			//<< ",GTDDate-" << pInputOrder->GTDDate
			//<< ",VolumeCondition-" << pInputOrder->VolumeCondition
			//<< ",MinVolume-" << pInputOrder->MinVolume
			//<< ",ContingentCondition-" << pInputOrder->ContingentCondition
			//<< ",StopPrice-" << pInputOrder->StopPrice
			//<< ",ForceCloseReason-" << pInputOrder->ForceCloseReason
			//<< ",IsAutoSuspend-" << pInputOrder->IsAutoSuspend
			//<< ",BusinessUnit-" << pInputOrder->BusinessUnit
			//<< ",RequestID-" << pInputOrder->RequestID
			//<< ",UserForceClose-" << pInputOrder->UserForceClose
			//<< ",IsSwapOrder-" << pInputOrder->IsSwapOrder
			<< "\n" );
	}

};

///预埋单录入请求响应
void CTradeSpi::OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///预埋撤单录入请求响应
void CTradeSpi::OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///报单操作请求响应
void CTradeSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	//dqk_log::CLogApi g_Log << thid;
	LOG("报单操作请求响应-OnRspOrderAction-" << nRequestID << ",bIsLast-" << bIsLast << "\n");

	if (NULL != pRspInfo)
	{
		LOG("报单操作请求响应 pRspInfo:msgCode-" << pRspInfo->ErrorID << ",msg-" << pRspInfo->ErrorMsg << "\n");

	}
	if (NULL != pInputOrderAction)
	{
		LOG("OnRspOrderAction-pInputOrderAction:BrokerID-" << pInputOrderAction->BrokerID
			//<< ",InvestorID-" << pInputOrderAction->InvestorID
			//<< ",OrderActionRef-" << pInputOrderAction->OrderActionRef
			<< ",OrderRef-" << pInputOrderAction->OrderRef
			//<< ",RequestID-" << pInputOrderAction->RequestID
			//<< ",FrontID-" << pInputOrderAction->FrontID
			//<< ",SessionID-" << pInputOrderAction->SessionID
			<< ",ExchangeID-" << pInputOrderAction->ExchangeID
			<< ",OrderSysID-" << pInputOrderAction->OrderSysID
			<< ",ActionFlag-" << pInputOrderAction->ActionFlag
			<< ",LimitPrice-" << pInputOrderAction->LimitPrice
			<< ",VolumeChange-" << pInputOrderAction->VolumeChange
			//<< ",UserID-" << pInputOrderAction->UserID
			<< ",InstrumentID-" << pInputOrderAction->InstrumentID << "\n" );
	}
	zc::PlannedOrderItem* po = getLocalOrder(pInputOrderAction->OrderRef);
	if (po)
	{
		if (THOST_FTDC_AF_Delete == pInputOrderAction->ActionFlag && pInputOrderAction->VolumeChange > 0)
		{
			LOG("撤单被接受！" << "已撤数量" << pInputOrderAction->VolumeChange << "lot！\n" );
			po->clot += pInputOrderAction->VolumeChange;
			if (po->clot == po->lot) // 全撤
			{
				po->status = zc::LEG_STATUS::EM_LEG_CANCELED;
			}
			else if (po->clot < po->lot)
			{
				po->status = zc::LEG_STATUS::EM_LEG_ParCANCELED;
			}
			else
			{
				LOG("撤单数错误！已撤数量重复计算。。。\n");
			}
		}
	}
	else
	{
		LOG("撤单：没有找到本地对应订单\n" );
	}
};

///查询最大报单数量响应
void CTradeSpi::OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("查询最大报单数量响应-OnRspOrderAction-" << nRequestID << ",bIsLast-" << bIsLast << "\n");

	if (NULL != pRspInfo)
	{
		LOG("查询最大报单数量响应 pRspInfo:msgCode-" << pRspInfo->ErrorID << ",msg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pQueryMaxOrderVolume)
	{
		LOG("pQueryMaxOrderVolume:BrokerID-" << pQueryMaxOrderVolume->BrokerID
			<< ",InvestorID-" << pQueryMaxOrderVolume->InvestorID
			<< ",InstrumentID-" << pQueryMaxOrderVolume->InstrumentID
			<< ",Direction-" << pQueryMaxOrderVolume->Direction
			<< ",OffsetFlag-" << pQueryMaxOrderVolume->OffsetFlag
			<< ",HedgeFlag-" << pQueryMaxOrderVolume->HedgeFlag
			<< ",MaxVolume-" << pQueryMaxOrderVolume->MaxVolume << "\n");
	}
};

///投资者结算结果确认响应
void CTradeSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("投资者结算结果确认响应-OnRspSettlementInfoConfirm:结算结果确认." << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("结算结果确认 pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pSettlementInfoConfirm)
	{
		LOG("CThostFtdcSettlementInfoConfirmField:BrokerID-" << pSettlementInfoConfirm->BrokerID
			<< ",ConfirmDate-" << pSettlementInfoConfirm->ConfirmDate
			<< ",ConfirmTime-" << pSettlementInfoConfirm->ConfirmTime
			<< ",InvestorID-" << pSettlementInfoConfirm->InvestorID << "\n");
	}
};

///删除预埋单响应
void CTradeSpi::OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///删除预埋撤单响应
void CTradeSpi::OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///执行宣告录入请求响应
void CTradeSpi::OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///执行宣告操作请求响应
void CTradeSpi::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///询价录入请求响应
void CTradeSpi::OnRspForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///报价录入请求响应
void CTradeSpi::OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///报价操作请求响应
void CTradeSpi::OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///批量报单操作请求响应
void CTradeSpi::OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField *pInputBatchOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///申请组合录入请求响应
void CTradeSpi::OnRspCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询报单响应
void CTradeSpi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("请求查询报单响应-OnRspQryOrder-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pOrder)
	{
		LOG("OnRspQryOrder:BrokerID-" << pOrder->BrokerID
			<< ",InvestorID-" << pOrder->InvestorID
			<< ",InstrumentID-" << pOrder->InstrumentID
			<< ",OrderRef-" << pOrder->OrderRef
			<< ",UserID-" << pOrder->UserID
			<< ",OrderPriceType-" << pOrder->OrderPriceType
			<< ",Direction-" << pOrder->Direction
			<< ",CombOffsetFlag-" << pOrder->CombOffsetFlag
			<< ",CombHedgeFlag-" << pOrder->CombHedgeFlag
			<< ",LimitPrice-" << pOrder->LimitPrice
			<< ",VolumeTotalOriginal-" << pOrder->VolumeTotalOriginal
			<< ",TimeCondition-" << pOrder->TimeCondition
			<< ",GTDDate-" << pOrder->GTDDate
			<< ",VolumeCondition-" << pOrder->VolumeCondition
			<< ",MinVolume-" << pOrder->MinVolume
			<< ",ContingentCondition-	" << pOrder->ContingentCondition
			<< ",StopPrice-" << pOrder->StopPrice
			<< ",ForceCloseReason-" << pOrder->ForceCloseReason
			<< ",IsAutoSuspend-" << pOrder->IsAutoSuspend
			<< ",BusinessUnit-" << pOrder->BusinessUnit
			<< ",RequestID-" << pOrder->RequestID
			<< ",OrderLocalID-" << pOrder->OrderLocalID
			<< ",ExchangeID-" << pOrder->ExchangeID
			<< ",ParticipantID-" << pOrder->ParticipantID
			<< ",ClientID-" << pOrder->ClientID
			<< ",ExchangeInstID-" << pOrder->ExchangeInstID
			<< ",TraderID-" << pOrder->TraderID
			<< ",InstallID-" << pOrder->InstallID
			<< ",OrderSubmitStatus-" << pOrder->OrderSubmitStatus
			<< ",NotifySequence-" << pOrder->NotifySequence
			<< ",TradingDay-" << pOrder->TradingDay
			<< ",SettlementID-" << pOrder->SettlementID
			<< ",OrderSysID-" << pOrder->OrderSysID
			<< ",OrderSource-" << pOrder->OrderSource
			<< ",OrderStatus-" << pOrder->OrderStatus
			<< ",OrderType-" << pOrder->OrderType
			<< ",VolumeTraded-" << pOrder->VolumeTraded
			<< ",VolumeTotal-" << pOrder->VolumeTotal
			<< ",InsertDate-" << pOrder->InsertDate
			<< ",InsertTime-" << pOrder->InsertTime
			<< ",ActiveTime-" << pOrder->ActiveTime
			<< ",SuspendTime-" << pOrder->SuspendTime
			<< ",UpdateTime-" << pOrder->UpdateTime
			<< ",CancelTime-" << pOrder->CancelTime
			<< ",ActiveTraderID-" << pOrder->ActiveTraderID
			<< ",ClearingPartID-" << pOrder->ClearingPartID
			<< ",SequenceNo-" << pOrder->SequenceNo
			<< ",FrontID-" << pOrder->FrontID
			<< ",SessionID-" << pOrder->SessionID
			<< ",UserProductInfo-" << pOrder->UserProductInfo
			<< ",StatusMsg-" << pOrder->StatusMsg
			<< ",UserForceClose-" << pOrder->UserForceClose
			<< ",ActiveUserID-" << pOrder->ActiveUserID
			<< ",BrokerOrderSeq-" << pOrder->BrokerOrderSeq
			<< ",RelativeOrderSysID-" << pOrder->RelativeOrderSysID
			<< ",ZCETotalTradedVolume-" << pOrder->ZCETotalTradedVolume
			<< ",IsSwapOrder-" << pOrder->IsSwapOrder << "\n" );
	}
};

///请求查询成交响应
void CTradeSpi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("请求查询成交响应-OnRspQryTrade-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("请求查询成交响应: pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pTrade)
	{
		LOG("pTrade:BrokerID-" << pTrade->BrokerID
			<< ",InvestorID-" << pTrade->InvestorID
			<< ",InstrumentID-" << pTrade->InstrumentID
			<< ",OrderRef-" << pTrade->OrderRef
			<< ",UserID-" << pTrade->UserID
			<< ",ExchangeID-" << pTrade->ExchangeID
			<< ",TradeID-" << pTrade->TradeID
			<< ",Direction-" << pTrade->Direction
			<< ",OrderSysID-" << pTrade->OrderSysID
			<< ",ParticipantID-" << pTrade->ParticipantID
			<< ",ClientID-" << pTrade->ClientID
			<< ",TradingRole-" << pTrade->TradingRole
			<< ",ExchangeInstID-" << pTrade->ExchangeInstID
			<< ",OffsetFlag-" << pTrade->OffsetFlag
			<< ",HedgeFlag-" << pTrade->HedgeFlag
			<< ",Price-" << pTrade->Price
			<< ",Volume-" << pTrade->Volume
			<< ",TradeDate-" << pTrade->TradeDate
			<< ",TradeTime-" << pTrade->TradeTime
			<< ",TradeType-" << pTrade->TradeType
			<< ",PriceSource-" << pTrade->PriceSource
			<< ",TraderID-" << pTrade->TraderID
			<< ",OrderLocalID-" << pTrade->OrderLocalID
			<< ",ClearingPartID-" << pTrade->ClearingPartID
			<< ",BusinessUnit-" << pTrade->BusinessUnit
			<< ",SequenceNo-" << pTrade->SequenceNo
			<< ",TradingDay-" << pTrade->TradingDay
			<< ",SettlementID-" << pTrade->SettlementID
			<< ",BrokerOrderSeq-" << pTrade->BrokerOrderSeq
			<< ",TradeSource-" << pTrade->TradeSource << "\n" );
	}
};

///请求查询投资者持仓响应
void CTradeSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("请求查询投资者持仓响应-OnRspQryInvestorPosition-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pInvestorPosition)
	{
		LOG("pInvestorPosition:InstrumentID-" << pInvestorPosition->InstrumentID
			<< ",BrokerID-" << pInvestorPosition->BrokerID
			<< ",InvestorID-" << pInvestorPosition->InvestorID
			<< ",PosiDirection-" << pInvestorPosition->PosiDirection
			<< ",HedgeFlag-" << pInvestorPosition->HedgeFlag
			<< ",PositionDate-" << pInvestorPosition->PositionDate
			<< ",YdPosition-" << pInvestorPosition->YdPosition
			<< ",Position-" << pInvestorPosition->Position
			<< ",TodayPosition-" << pInvestorPosition->TodayPosition
			<< ",LongFrozen-" << pInvestorPosition->LongFrozen
			<< ",ShortFrozen-" << pInvestorPosition->ShortFrozen
			<< ",OpenVolume-" << pInvestorPosition->OpenVolume
			<< ",CloseVolume-" << pInvestorPosition->CloseVolume
			<< ",OpenAmount-" << pInvestorPosition->OpenAmount
			<< ",LongFrozenAmount-" << pInvestorPosition->LongFrozenAmount
			<< ",ShortFrozenAmount-" << pInvestorPosition->ShortFrozenAmount
			<< ",CloseAmount-" << pInvestorPosition->CloseAmount
			<< ",PositionCost-" << pInvestorPosition->PositionCost
			<< ",PreMargin-" << pInvestorPosition->PreMargin
			<< ",UseMargin-" << pInvestorPosition->UseMargin
			<< ",FrozenMargin-" << pInvestorPosition->FrozenMargin
			<< ",FrozenCash-" << pInvestorPosition->FrozenCash
			<< ",FrozenCommission-" << pInvestorPosition->FrozenCommission
			<< ",CashIn-" << pInvestorPosition->CashIn
			<< ",Commission-" << pInvestorPosition->Commission
			<< ",CloseProfit-" << pInvestorPosition->CloseProfit
			<< ",PositionProfit-" << pInvestorPosition->PositionProfit
			<< ",PreSettlementPrice-" << pInvestorPosition->PreSettlementPrice
			<< ",SettlementPrice-" << pInvestorPosition->SettlementPrice
			<< ",TradingDay-" << pInvestorPosition->TradingDay
			<< ",SettlementID-" << pInvestorPosition->SettlementID
			<< ",OpenCost-" << pInvestorPosition->OpenCost
			<< ",ExchangeMargin-" << pInvestorPosition->ExchangeMargin
			<< ",CombPosition-" << pInvestorPosition->CombPosition
			<< ",CombLongFrozen-" << pInvestorPosition->CombLongFrozen
			<< ",CombShortFrozen-" << pInvestorPosition->CombShortFrozen
			<< ",CloseProfitByDate-" << pInvestorPosition->CloseProfitByDate
			<< ",CloseProfitByTrade-" << pInvestorPosition->CloseProfitByTrade
			<< ",MarginRateByMoney-" << pInvestorPosition->MarginRateByMoney
			<< ",MarginRateByVolume-" << pInvestorPosition->MarginRateByVolume
			<< ",StrikeFrozen-" << pInvestorPosition->StrikeFrozen
			<< ",StrikeFrozenAmount-" << pInvestorPosition->StrikeFrozenAmount
			<< ",AbandonFrozen-" << pInvestorPosition->AbandonFrozen << "\n" );
		zc::QryPositionFB* qryPos = static_cast<zc::QryPositionFB*>(queryFeedBack);
		pInvestorPosition->TodayPosition -= (pInvestorPosition->LongFrozen + pInvestorPosition->ShortFrozen);
		if (pInvestorPosition->TodayPosition > 0)
		{
			qryPos->posList.push_back(*pInvestorPosition);
		}
	}

};

bool bAccount = false;
///请求查询资金账户响应
void CTradeSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	if (!bAccount)
		bAccount = true;
	else
		return;
	//printf("请求查询资金账户响应-%d\n",nRequestID);
	//dqk_log::CLogApi g_Log << thid;
	LOG("请求查询资金账户响应-OnRspQryTradingAccount-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	LOG("请求查询资金账户响应-OnRspQryTradingAccount-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("请求查询资金账户响应 : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pTradingAccount)
	{
		LOG("pTradingAccount:BrokerID-" << pTradingAccount->BrokerID
			<< ",AccountID-" << pTradingAccount->AccountID
			<< ",PreMortgage-" << pTradingAccount->PreMortgage
			<< ",PreCredit-" << pTradingAccount->PreCredit
			<< ",PreDeposit-" << pTradingAccount->PreDeposit
			<< ",PreBalance-" << pTradingAccount->PreBalance
			<< ",PreMargin-" << pTradingAccount->PreMargin
			<< ",InterestBase-" << pTradingAccount->InterestBase
			<< ",Interest-" << pTradingAccount->Interest
			<< ",Deposit-" << pTradingAccount->Deposit
			<< ",Withdraw-" << pTradingAccount->Withdraw
			<< ",FrozenMargin-" << pTradingAccount->FrozenMargin
			<< ",FrozenCash-" << pTradingAccount->FrozenCash
			<< ",FrozenCommission-" << pTradingAccount->FrozenCommission
			<< ",CurrMargin-" << pTradingAccount->CurrMargin
			<< ",CashIn-" << pTradingAccount->CashIn
			<< ",Commission-" << pTradingAccount->Commission
			<< ",CloseProfit-" << pTradingAccount->CloseProfit
			<< ",PositionProfit-" << pTradingAccount->PositionProfit
			<< ",Balance-" << pTradingAccount->Balance
			<< ",Available-" << pTradingAccount->Available
			<< ",WithdrawQuota-" << pTradingAccount->WithdrawQuota
			<< ",Reserve-" << pTradingAccount->Reserve
			<< ",TradingDay-" << pTradingAccount->TradingDay
			<< ",SettlementID-" << pTradingAccount->SettlementID
			<< ",Credit-" << pTradingAccount->Credit
			<< ",Mortgage-" << pTradingAccount->Mortgage
			<< ",ExchangeMargin-" << pTradingAccount->ExchangeMargin
			<< ",DeliveryMargin-" << pTradingAccount->DeliveryMargin
			<< ",ExchangeDeliveryMargin-" << pTradingAccount->ExchangeDeliveryMargin << "\n" );

	}
};

///请求查询投资者响应
void CTradeSpi::OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	//dqk_log::CLogApi g_Log << thid;
	LOG("请求查询投资者响应-OnRspQryInvestor-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("请求查询投资者响应 : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n" );
	}
	if (NULL != pInvestor)
	{
		LOG("pInvestor:InvestorID-" << pInvestor->InvestorID
			<< ",BrokerID-" << pInvestor->BrokerID
			<< ",InvestorGroupID-" << pInvestor->InvestorGroupID
			<< ",InvestorName-" << pInvestor->InvestorName
			<< ",IdentifiedCardType-" << pInvestor->IdentifiedCardType
			<< ",IdentifiedCardNo-" << pInvestor->IdentifiedCardNo
			<< ",IsActive-" << pInvestor->IsActive
			<< ",Telephone-" << pInvestor->Telephone
			<< ",Address-" << pInvestor->Address
			<< ",OpenDate-" << pInvestor->OpenDate
			<< ",Mobile-" << pInvestor->Mobile
			<< ",CommModelID-" << pInvestor->CommModelID << "\n" );
		//CThostFtdcQryInvestorField* fb = static_cast<CThostFtdcQryInvestorField*>(queryFeedBack);
		//memcpy(fb, pInvestor, sizeof(CThostFtdcInvestorField));
		//if (bIsLast)ResetQryFB();
		strcpy(InvestorID_Future, pInvestor->InvestorID);
	}
};


///请求查询交易编码响应
void CTradeSpi::OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	//dqk_log::CLogApi g_Log << thid;
	LOG("请求查询交易编码响应-OnRspQryTradingCode-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("请求查询交易编码响应 : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pTradingCode)
	{
		LOG("pTradingCode:InvestorID-" << pTradingCode->InvestorID
			<< ",BrokerID-" << pTradingCode->BrokerID
			<< ",ExchangeID-" << pTradingCode->ExchangeID
			<< ",ClientID-" << pTradingCode->ClientID
			<< ",IsActive-" << pTradingCode->IsActive
			<< ",ClientIDType-" << pTradingCode->ClientIDType << "\n");

		CGetParam param;
		param.WriteTradecode(pTradingCode);
	}

};

///请求查询合约保证金率响应
void CTradeSpi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	//dqk_log::CLogApi g_Log << thid;
	LOG("请求查询合约保证金率响应-OnRspQryInstrumentMarginRate-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("请求查询合约保证金率响应：Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		LOG("pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pInstrumentMarginRate)
	{
		LOG("pInstrumentMarginRate:InstrumentID-" << pInstrumentMarginRate->InstrumentID
			<< ",InvestorRange-" << pInstrumentMarginRate->InvestorRange
			<< ",BrokerID-" << pInstrumentMarginRate->BrokerID
			<< ",InvestorID-" << pInstrumentMarginRate->InvestorID
			<< ",HedgeFlag-" << pInstrumentMarginRate->HedgeFlag
			<< ",LongMarginRatioByMoney-" << pInstrumentMarginRate->LongMarginRatioByMoney
			<< ",LongMarginRatioByVolume-" << pInstrumentMarginRate->LongMarginRatioByVolume
			<< ",ShortMarginRatioByMoney-" << pInstrumentMarginRate->ShortMarginRatioByMoney
			<< ",ShortMarginRatioByVolume-" << pInstrumentMarginRate->ShortMarginRatioByVolume
			<< ",IsRelative-" << pInstrumentMarginRate->IsRelative
			<< "\n");
	}
};

///请求查询合约手续费率响应
void CTradeSpi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("请求查询合约手续费率响应-%d\n", nRequestID);

	LOG("请求查询合约手续费率响应-OnRspQryInstrumentCommissionRate-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("请求查询合约手续费率响应：Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		LOG("pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pInstrumentCommissionRate)
	{
		LOG("pInstrumentCommissionRate:InstrumentID-" << pInstrumentCommissionRate->InstrumentID
			<< ",InvestorRange-" << pInstrumentCommissionRate->InvestorRange
			<< ",BrokerID-" << pInstrumentCommissionRate->BrokerID
			<< ",InvestorID-" << pInstrumentCommissionRate->InvestorID
			<< ",OpenRatioByMoney-" << pInstrumentCommissionRate->OpenRatioByMoney
			<< ",OpenRatioByVolume-" << pInstrumentCommissionRate->OpenRatioByVolume
			<< ",CloseRatioByMoney-" << pInstrumentCommissionRate->CloseRatioByMoney
			<< ",CloseRatioByVolume-" << pInstrumentCommissionRate->CloseRatioByVolume
			<< ",CloseTodayRatioByMoney-" << pInstrumentCommissionRate->CloseTodayRatioByMoney
			<< ",CloseTodayRatioByVolume-" << pInstrumentCommissionRate->CloseTodayRatioByVolume
			<< "\n");
	}
};

///请求查询交易所响应
void CTradeSpi::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("请求查询交易所响应-OnRspQryExchange-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("请求查询交易所响应 : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pExchange)
	{
		LOG("pExchange:ExchangeID-" << pExchange->ExchangeID
			<< ",ExchangeName-" << pExchange->ExchangeName
			<< ",ExchangeProperty-" << pExchange->ExchangeProperty << "\n");
	}

};

///请求查询产品响应
void CTradeSpi::OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询合约响应
void CTradeSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	// dqk_log::CLogApi log;
	//std::cout << "请求查询合约响应-OnRspQryInstrument-" << nRequestID << ",bIsLast-" << bIsLast << ",bIsLast-" << bIsLast << "\n";
	if (NULL != pRspInfo)
	{
		//log << "pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n";
		LOG("请求查询合约响应 : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n" );
	}
	if (NULL != pInstrument)
	{
		//std::cout << pInstrument->InstrumentID << " ";
		//return;
		bool found = false;
		int ns = sizeof(pSubInstrumnet) / sizeof(char*);
		for (int i = 0; i < ns; i++)
		{
			if (0 == strcmp(pInstrument->InstrumentID, pSubInstrumnet[i]))
			{
				found = true;
				break;
			}
		}
		if (!found)return;
		/*std::cout << "pInstrument:InstrumentID-" << pInstrument->InstrumentID
			<< ",ExchangeID-" << pInstrument->ExchangeID
			<< ",InstrumentName-" << pInstrument->InstrumentName
			<< ",ExchangeInstID-" << pInstrument->ExchangeInstID
			<< ",ProductID-" << pInstrument->ProductID
			<< ",ProductClass-" << pInstrument->ProductClass
			<< ",DeliveryYear-" << pInstrument->DeliveryYear
			<< ",DeliveryMonth-" << pInstrument->DeliveryMonth
			<< ",MaxMarketOrderVolume-" << pInstrument->MaxMarketOrderVolume
			<< ",MinMarketOrderVolume-" << pInstrument->MinMarketOrderVolume
			<< ",MaxLimitOrderVolume-" << pInstrument->MaxLimitOrderVolume
			<< ",MinLimitOrderVolume-" << pInstrument->MinLimitOrderVolume
			<< ",VolumeMultiple-" << pInstrument->VolumeMultiple
			<< ",PriceTick-" << pInstrument->PriceTick
			<< ",CreateDate-" << pInstrument->CreateDate
			<< ",OpenDate-" << pInstrument->OpenDate
			<< ",ExpireDate-" << pInstrument->ExpireDate
			<< ",StartDelivDate-" << pInstrument->StartDelivDate
			<< ",EndDelivDate-" << pInstrument->EndDelivDate
			<< ",InstLifePhase-" << pInstrument->InstLifePhase
			<< ",IsTrading-" << pInstrument->IsTrading
			<< ",PositionType-" << pInstrument->PositionType
			<< ",PositionDateType-" << pInstrument->PositionDateType
			<< ",LongMarginRatio-" << pInstrument->LongMarginRatio
			<< ",ShortMarginRatio-" << pInstrument->ShortMarginRatio
			<< "\n";*/
		std::string instid = pInstrument->InstrumentID;
		auto fb = static_cast<zc::QryInstrumentFB*>(queryFeedBack);
		//auto ifound = std::find_if(fb->Insts.begin(), fb->Insts.end(), [&instid](zc::InstumentInfo& in)->bool{return in.InstrumentId == instid; };
		fb->Insts.push_back({ pInstrument->InstrumentID, pInstrument->PriceTick });
	}
};

///请求查询行情响应
void CTradeSpi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询投资者结算结果响应
void CTradeSpi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("请求查询投资者结算结果响应-OnRspQrySettlementInfo-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("请求查询投资者结算结果响应：Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		LOG("pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pSettlementInfo)
	{
		LOG("pSettlementInfo:TradingDay-" << pSettlementInfo->TradingDay
			<< ",SettlementID-" << pSettlementInfo->SettlementID
			<< ",BrokerID-" << pSettlementInfo->BrokerID
			<< ",InvestorID-" << pSettlementInfo->InvestorID
			<< ",SequenceNo-" << pSettlementInfo->SequenceNo
			<< ",Content-" << pSettlementInfo->Content << "\n");
	}
};

///请求查询转帐银行响应
void CTradeSpi::OnRspQryTransferBank(CThostFtdcTransferBankField *pTransferBank, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询投资者持仓明细响应
void CTradeSpi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("请求查询转帐银行响应-%d\n", nRequestID);
	//dqk_log::CLogApi g_Log << thid;
	LOG("请求查询投资者持仓明细响应-OnRspQrySettlementInfo-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("请求查询转帐银行响应：Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		LOG("pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pInvestorPositionDetail)
	{
		LOG("pInvestorPositionDetail:InstrumentID-" << pInvestorPositionDetail->InstrumentID
			<< ",BrokerID-" << pInvestorPositionDetail->BrokerID
			<< ",InvestorID-" << pInvestorPositionDetail->InvestorID
			<< ",HedgeFlag-" << pInvestorPositionDetail->HedgeFlag
			<< ",Direction-" << pInvestorPositionDetail->Direction
			<< ",OpenDate-" << pInvestorPositionDetail->OpenDate
			<< ",TradeID-" << pInvestorPositionDetail->TradeID
			<< ",Volume-" << pInvestorPositionDetail->Volume
			<< ",OpenPrice-" << pInvestorPositionDetail->OpenPrice
			<< ",TradingDay-" << pInvestorPositionDetail->TradingDay
			<< ",SettlementID-" << pInvestorPositionDetail->SettlementID
			<< ",TradeType-" << pInvestorPositionDetail->TradeType
			<< ",CombInstrumentID-" << pInvestorPositionDetail->CombInstrumentID
			<< ",ExchangeID-" << pInvestorPositionDetail->ExchangeID
			<< ",CloseProfitByDate-" << pInvestorPositionDetail->CloseProfitByDate
			<< ",CloseProfitByTrade-" << pInvestorPositionDetail->CloseProfitByTrade
			<< ",PositionProfitByDate-" << pInvestorPositionDetail->PositionProfitByDate
			<< ",PositionProfitByTrade-" << pInvestorPositionDetail->PositionProfitByTrade
			<< ",Margin-" << pInvestorPositionDetail->Margin
			<< ",ExchMargin-" << pInvestorPositionDetail->ExchMargin
			<< ",MarginRateByMoney-" << pInvestorPositionDetail->MarginRateByMoney
			<< ",MarginRateByVolume-" << pInvestorPositionDetail->MarginRateByVolume
			<< ",LastSettlementPrice-" << pInvestorPositionDetail->LastSettlementPrice
			<< ",SettlementPrice-" << pInvestorPositionDetail->SettlementPrice
			<< ",CloseVolume-" << pInvestorPositionDetail->CloseVolume
			<< ",CloseAmount-" << pInvestorPositionDetail->CloseAmount
			<< "\n");
	}
};

///请求查询客户通知响应
void CTradeSpi::OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("请求查询客户通知响应-%d\n", nRequestID);
	//dqk_log::CLogApi g_Log << thid;
	LOG("请求查询客户通知响应-OnRspQryNotice-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("请求查询客户通知响应：Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		LOG("pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pNotice)
	{
		LOG("pNotice:BrokerID-" << pNotice->BrokerID
			<< "pNotice-" << pNotice->Content
			<< "pNotice-" << pNotice->SequenceLabel
			<< "\n");
	}
};

///请求查询结算信息确认响应
void CTradeSpi::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("请求查询结算信息确认响应-%d\n", nRequestID);
	//dqk_log::CLogApi g_Log << thid;
	LOG("请求查询结算信息确认响应-OnRspQrySettlementInfoConfirm-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("请求查询结算信息确认响应：Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		LOG("pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pSettlementInfoConfirm)
	{
		LOG("pSettlementInfoConfirm:BrokerID-" << pSettlementInfoConfirm->BrokerID
			<< ",InvestorID-" << pSettlementInfoConfirm->InvestorID
			<< ",ConfirmDate-" << pSettlementInfoConfirm->ConfirmDate
			<< ",ConfirmTime-" << pSettlementInfoConfirm->ConfirmTime << "\n");
	}
};

///请求查询投资者持仓明细响应
void CTradeSpi::OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///查询保证金监管系统经纪公司资金账户密钥响应
void CTradeSpi::OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询仓单折抵信息响应
void CTradeSpi::OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField *pEWarrantOffset, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询投资者品种/跨品种保证金响应
void CTradeSpi::OnRspQryInvestorProductGroupMargin(CThostFtdcInvestorProductGroupMarginField *pInvestorProductGroupMargin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询交易所保证金率响应
void CTradeSpi::OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField *pExchangeMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询交易所调整保证金率响应
void CTradeSpi::OnRspQryExchangeMarginRateAdjust(CThostFtdcExchangeMarginRateAdjustField *pExchangeMarginRateAdjust, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询汇率响应
void CTradeSpi::OnRspQryExchangeRate(CThostFtdcExchangeRateField *pExchangeRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询二级代理操作员银期权限响应
void CTradeSpi::OnRspQrySecAgentACIDMap(CThostFtdcSecAgentACIDMapField *pSecAgentACIDMap, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询产品报价汇率
void CTradeSpi::OnRspQryProductExchRate(CThostFtdcProductExchRateField *pProductExchRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询产品组
void CTradeSpi::OnRspQryProductGroup(CThostFtdcProductGroupField *pProductGroup, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询期权交易成本响应
void CTradeSpi::OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询期权合约手续费响应
void CTradeSpi::OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询执行宣告响应
void CTradeSpi::OnRspQryExecOrder(CThostFtdcExecOrderField *pExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询询价响应
void CTradeSpi::OnRspQryForQuote(CThostFtdcForQuoteField *pForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询报价响应
void CTradeSpi::OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询组合合约安全系数响应
void CTradeSpi::OnRspQryCombInstrumentGuard(CThostFtdcCombInstrumentGuardField *pCombInstrumentGuard, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询申请组合响应
void CTradeSpi::OnRspQryCombAction(CThostFtdcCombActionField *pCombAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询转帐流水响应
void CTradeSpi::OnRspQryTransferSerial(CThostFtdcTransferSerialField *pTransferSerial, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询银期签约关系响应
void CTradeSpi::OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///错误应答
void CTradeSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("错误应答-%d\n", nRequestID);
	LOG("错误应答-OnRspError-" << nRequestID << ",bIsLast-" << bIsLast << "\n" );
	if (NULL != pRspInfo)
	{
		printf("错误应答：Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		LOG("pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n" );
	}
};

///报单通知
void CTradeSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	int thid = GetCurrentThreadId();
	LOG("报单通知-OnRtnOrder" << "\n");
	if (NULL != pOrder)
	{
		if (THOST_FTDC_OST_Canceled == pOrder->OrderStatus)return;

		LOG("pOrder:BrokerID-" << pOrder->BrokerID
			//<< ",InvestorID-" << pOrder->InvestorID
			<< ",InstrumentID-" << pOrder->InstrumentID
			<< ",OrderRef-" << pOrder->OrderRef
			//<< ",UserID-" << pOrder->UserID
			//<< ",OrderPriceType-" << pOrder->OrderPriceType
			<< ",Direction-" << pOrder->Direction
			<< ",CombOffsetFlag-" << pOrder->CombOffsetFlag
			//<< ",CombHedgeFlag-" << pOrder->CombHedgeFlag
			<< ",LimitPrice-" << pOrder->LimitPrice
			<< ",VolumeTotalOriginal-" << pOrder->VolumeTotalOriginal
			//<< ",TimeCondition-" << pOrder->TimeCondition
			//<< ",GTDDate-" << pOrder->GTDDate
			//<< ",VolumeCondition-" << pOrder->VolumeCondition
			//<< ",MinVolume-" << pOrder->MinVolume
			//<< ",ContingentCondition-" << pOrder->ContingentCondition
			//<< ",StopPrice-" << pOrder->StopPrice
			//<< ",ForceCloseReason-" << pOrder->ForceCloseReason
			//<< ",IsAutoSuspend-" << pOrder->IsAutoSuspend
			//<< ",BusinessUnit-" << pOrder->BusinessUnit
			//<< ",RequestID-" << pOrder->RequestID
			<< ",OrderLocalID-" << pOrder->OrderLocalID
			//<< ",ExchangeID-" << pOrder->ExchangeID
			//<< ",ParticipantID-" << pOrder->ParticipantID
			//<< ",ClientID-" << pOrder->ClientID
			//<< ",ExchangeInstID-" << pOrder->ExchangeInstID
			//<< ",TraderID-" << pOrder->TraderID
			//<< ",InstallID-" << pOrder->InstallID
			<< ",OrderSubmitStatus-" << pOrder->OrderSubmitStatus
			//<< ",NotifySequence-" << pOrder->NotifySequence
			<< ",TradingDay-" << pOrder->TradingDay
			//<< ",SettlementID-" << pOrder->SettlementID
			<< ",OrderSysID-" << pOrder->OrderSysID
			//<< ",OrderSource-" << pOrder->OrderSource
			<< ",OrderStatus-" << pOrder->OrderStatus
			//<< ",OrderType-" << pOrder->OrderType
			<< ",VolumeTraded-" << pOrder->VolumeTraded
			<< ",VolumeTotal-" << pOrder->VolumeTotal
			<< ",InsertDate-" << pOrder->InsertDate
			<< ",InsertTime-" << pOrder->InsertTime
			<< ",ActiveTime-" << pOrder->ActiveTime
			//<< ",SuspendTime-" << pOrder->SuspendTime
			//<< ",UpdateTime-" << pOrder->UpdateTime
			//<< ",CancelTime-" << pOrder->CancelTime
			//<< ",ActiveTraderID-" << pOrder->ActiveTraderID
			//<< ",ClearingPartID-" << pOrder->ClearingPartID
			//<< ",SequenceNo-" << pOrder->SequenceNo
			//<< ",FrontID-" << pOrder->FrontID
			//<< ",SessionID-" << pOrder->SessionID
			//<< ",UserProductInfo-" << pOrder->UserProductInfo
			<< ",StatusMsg-" << pOrder->StatusMsg
			//<< ",UserForceClose-" << pOrder->UserForceClose
			//<< ",ActiveUserID-" << pOrder->ActiveUserID
			//<< ",BrokerOrderSeq-" << pOrder->BrokerOrderSeq
			//<< ",RelativeOrderSysID-" << pOrder->RelativeOrderSysID
			//<< ",ZCETotalTradedVolume-" << pOrder->ZCETotalTradedVolume
			//<< ",IsSwapOrder-" << pOrder->IsSwapOrder
			<< "\n" );
	}

	zc::PlannedOrderItem* po = getLocalOrder(pOrder->OrderRef);
	if (po)
	{
		if (THOST_FTDC_OST_Canceled == pOrder->OrderStatus)
		{
			// 撤单成功后的回报
			LOG("总送单数量：" << std::internal <<po->lot << " 撤单数量：" << po->clot << std::endl);
			/*
			if (po->clot == po->lot) // 全撤
			{
				po->status = zc::LEG_STATUS::EM_LEG_CANCELED;
			}
			else
			{
				po->status = zc::LEG_STATUS::EM_LEG_ParCANCELED;
			}
			*/
			LOG("撤单成功！\n");
			return;
		}

		po->ordOrderedTime = zc::GetCurTime();
		if (THOST_FTDC_OSS_Accepted == pOrder->OrderSubmitStatus && 0 == pOrder->VolumeTraded)
		{
			po->status = zc::LEG_STATUS::EM_LEG_ORDERED; // 已报
			LOG("已报成功！\n");
			po->exchId = pOrder->ExchangeID;
			po->ordsysId = pOrder->OrderSysID;
		}
		else if (THOST_FTDC_OSS_InsertRejected == pOrder->OrderSubmitStatus)
		{
			LOG("order ref:" << pOrder->OrderRef << " is rejected!\n");
			po->status = zc::LEG_STATUS::EM_LEG_CANCELED; // 被拒，系统自动撤单
		}
	}
	else
	{
		// 没找到
		LOG("OnRtnOrder ERROR: local order: " << pOrder->OrderRef << " mei zhao dao mei zhao dao ################################ mei zhao dao\n" );
	}
};

///成交通知
void CTradeSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	int thid = GetCurrentThreadId();
	LOG("成交通知-OnRtnTrade : " << pTrade->OrderRef << "\n");
	if (NULL != pTrade)
	{
		LOG("pTrade:BrokerID-" << pTrade->BrokerID
			//<< ",InvestorID-" << pTrade->InvestorID
			<< ",InstrumentID-" << pTrade->InstrumentID
			<< ",OrderRef-" << pTrade->OrderRef
			//<< ",UserID-" << pTrade->UserID
			//<< ",ExchangeID-" << pTrade->ExchangeID
			//<< ",TradeID-" << pTrade->TradeID
			<< ",Direction-" << pTrade->Direction
			//<< ",OrderSysID-" << pTrade->OrderSysID
			//<< ",ParticipantID-" << pTrade->ParticipantID
			//<< ",ClientID-" << pTrade->ClientID
			//<< ",TradingRole-" << pTrade->TradingRole
			//<< ",ExchangeInstID-" << pTrade->ExchangeInstID
			<< ",OffsetFlag-" << pTrade->OffsetFlag
			//<< ",HedgeFlag-" << pTrade->HedgeFlag
			<< ",Price-" << pTrade->Price
			<< ",Volume-" << pTrade->Volume
			<< ",TradeDate-" << pTrade->TradeDate
			<< ",TradeTime-" << pTrade->TradeTime
			//<< ",TradeType-" << pTrade->TradeType
			//<< ",PriceSource-" << pTrade->PriceSource
			//<< ",TraderID-" << pTrade->TraderID
			//<< ",OrderLocalID-" << pTrade->OrderLocalID
			//<< ",ClearingPartID-" << pTrade->ClearingPartID
			//<< ",BusinessUnit-" << pTrade->BusinessUnit
			//<< ",SequenceNo-" << pTrade->SequenceNo
			<< ",TradingDay-" << pTrade->TradingDay
			//<< ",SettlementID-" << pTrade->SettlementID
			//<< ",BrokerOrderSeq-" << pTrade->BrokerOrderSeq
			//<< ",TradeSource-" << pTrade->TradeSource
			<< "\n" );
	}
	// 成交后修改另一腿状态
	zc::PlannedOrderItem* po = getLocalOrder(pTrade->OrderRef);

	if (po)
	{
		po->ordTradedTime = zc::GetCurTime();
		po->elot += pTrade->Volume;
		po->alot -= pTrade->Volume;
		assert(po->alot > -1);
		if (po->alot < 1)
		{
			assert(po->elot == po->lot);
			po->status = zc::LEG_STATUS::EM_LEG_TRADED;
			LOG("订单已全部成交: "<<pTrade->Volume<<"\n");
		}
		else
		{
			po->status = zc::LEG_STATUS::EM_LEG_ParTRADED;
			LOG("订单部分成交数量: "<<pTrade->Volume<<"\n");
		}
	}
	else
	{
		// 没找到订单ref
		LOG("OnRtnTrade没有找到订单ref " << pTrade->OrderRef << "\n" );
	}
};


///报单录入错误回报
void CTradeSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	int thid = GetCurrentThreadId();
	LOG("报单录入错误回报-OnErrRtnOrderInsert");
	if (NULL != pRspInfo)
	{
		LOG("报单录入错误回报 : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}

	if (NULL != pInputOrder)
	{
		LOG("pInputOrder:BrokerID-" << pInputOrder->BrokerID
			//<< ",InvestorID-" << pInputOrder->InvestorID
			<< ",InstrumentID-" << pInputOrder->InstrumentID
			<< ",OrderRef-" << pInputOrder->OrderRef
			//<< ",UserID-" << pInputOrder->UserID
			<< ",OrderPriceType-" << pInputOrder->OrderPriceType
			<< ",Direction-" << pInputOrder->Direction
			<< ",CombOffsetFlag-" << pInputOrder->CombOffsetFlag
			//<< ",CombHedgeFlag-" << pInputOrder->CombHedgeFlag
			<< ",LimitPrice-" << pInputOrder->LimitPrice
			<< ",VolumeTotalOriginal-" << pInputOrder->VolumeTotalOriginal
			//<< ",TimeCondition-" << pInputOrder->TimeCondition
			//<< ",GTDDate-" << pInputOrder->GTDDate
			//<< ",VolumeCondition-" << pInputOrder->VolumeCondition
			//<< ",MinVolume-" << pInputOrder->MinVolume
			//<< ",ContingentCondition-" << pInputOrder->ContingentCondition
			//<< ",StopPrice-" << pInputOrder->StopPrice
			//<< ",ForceCloseReason-" << pInputOrder->ForceCloseReason
			//<< ",IsAutoSuspend-" << pInputOrder->IsAutoSuspend
			//<< ",BusinessUnit-" << pInputOrder->BusinessUnit
			//<< ",RequestID-" << pInputOrder->RequestID
			//<< ",UserForceClose-" << pInputOrder->UserForceClose
			//<< ",IsSwapOrder-" << pInputOrder->IsSwapOrder
			<< "\n" );
	}
};

///报单操作错误回报
void CTradeSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{
	int thid = GetCurrentThreadId();
	LOG("报单操作错误回报-OnErrRtnOrderAction" << "\n");
	if (NULL != pRspInfo)
	{
		LOG("报单操作错误回报 : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pOrderAction)
	{
		LOG("pOrderAction:BrokerID-" << pOrderAction->BrokerID
			<< ",InvestorID-" << pOrderAction->InvestorID
			<< ",OrderActionRef-" << pOrderAction->OrderActionRef
			<< ",OrderRef-" << pOrderAction->OrderRef
			<< ",RequestID-" << pOrderAction->RequestID
			<< ",FrontID-" << pOrderAction->FrontID
			<< ",SessionID-" << pOrderAction->SessionID
			<< ",ExchangeID-" << pOrderAction->ExchangeID
			<< ",OrderSysID-" << pOrderAction->OrderSysID
			<< ",ActionFlag-" << pOrderAction->ActionFlag
			<< ",LimitPrice-" << pOrderAction->LimitPrice
			<< ",VolumeChange-" << pOrderAction->VolumeChange
			<< ",ActionDate-" << pOrderAction->ActionDate
			<< ",ActionTime-" << pOrderAction->ActionTime
			<< ",TraderID-" << pOrderAction->TraderID
			<< ",InstallID-" << pOrderAction->InstallID
			<< ",OrderLocalID-" << pOrderAction->OrderLocalID
			<< ",ActionLocalID-" << pOrderAction->ActionLocalID
			<< ",ParticipantID-" << pOrderAction->ParticipantID
			<< ",ClientID-" << pOrderAction->ClientID
			<< ",BusinessUnit-" << pOrderAction->BusinessUnit
			<< ",OrderActionStatus-" << pOrderAction->OrderActionStatus
			<< ",UserID-" << pOrderAction->UserID
			<< ",StatusMsg-" << pOrderAction->StatusMsg
			<< ",InstrumentID-" << pOrderAction->InstrumentID
			<< "\n" );

		zc::PlannedOrderItem* po = getLocalOrder(pOrderAction->OrderRef);
		// po->status = zc::LEG_STATUS::
	}
};

///合约交易状态通知
void CTradeSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) {};

///交易通知
void CTradeSpi::OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo) {};

///提示条件单校验错误
void CTradeSpi::OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField *pErrorConditionalOrder) {};

///执行宣告通知
void CTradeSpi::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder) {};

///执行宣告录入错误回报
void CTradeSpi::OnErrRtnExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo) {};

///执行宣告操作错误回报
void CTradeSpi::OnErrRtnExecOrderAction(CThostFtdcExecOrderActionField *pExecOrderAction, CThostFtdcRspInfoField *pRspInfo) {};

///询价录入错误回报
void CTradeSpi::OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo) {};

///报价通知
void CTradeSpi::OnRtnQuote(CThostFtdcQuoteField *pQuote) {};

///报价录入错误回报
void CTradeSpi::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo) {};

///报价操作错误回报
void CTradeSpi::OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo) {};

///询价通知
void CTradeSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {};

///保证金监控中心用户令牌
void CTradeSpi::OnRtnCFMMCTradingAccountToken(CThostFtdcCFMMCTradingAccountTokenField *pCFMMCTradingAccountToken) {};

///批量报单操作错误回报
void CTradeSpi::OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField *pBatchOrderAction, CThostFtdcRspInfoField *pRspInfo) {};

///申请组合通知
void CTradeSpi::OnRtnCombAction(CThostFtdcCombActionField *pCombAction) {};

///申请组合录入错误回报
void CTradeSpi::OnErrRtnCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo) {};

///请求查询签约银行响应
void CTradeSpi::OnRspQryContractBank(CThostFtdcContractBankField *pContractBank, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询预埋单响应
void CTradeSpi::OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询预埋撤单响应
void CTradeSpi::OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询交易通知响应
void CTradeSpi::OnRspQryTradingNotice(CThostFtdcTradingNoticeField *pTradingNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询经纪公司交易参数响应
void CTradeSpi::OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询经纪公司交易算法响应
void CTradeSpi::OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField *pBrokerTradingAlgos, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///请求查询监控中心用户令牌
void CTradeSpi::OnRspQueryCFMMCTradingAccountToken(CThostFtdcQueryCFMMCTradingAccountTokenField *pQueryCFMMCTradingAccountToken, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///银行发起银行资金转期货通知
void CTradeSpi::OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField *pRspTransfer) {};

///银行发起期货资金转银行通知
void CTradeSpi::OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField *pRspTransfer) {};

///银行发起冲正银行转期货通知
void CTradeSpi::OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField *pRspRepeal) {};

///银行发起冲正期货转银行通知
void CTradeSpi::OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField *pRspRepeal) {};

///期货发起银行资金转期货通知
void CTradeSpi::OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField *pRspTransfer) {};

///期货发起期货资金转银行通知
void CTradeSpi::OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField *pRspTransfer) {};

///系统运行时期货端手工发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
void CTradeSpi::OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {};

///系统运行时期货端手工发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
void CTradeSpi::OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {};

///期货发起查询银行余额通知
void CTradeSpi::OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField *pNotifyQueryAccount) {};

///期货发起银行资金转期货错误回报
void CTradeSpi::OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo) {};

///期货发起期货资金转银行错误回报
void CTradeSpi::OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo) {};

///系统运行时期货端手工发起冲正银行转期货错误回报
void CTradeSpi::OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo) {};

///系统运行时期货端手工发起冲正期货转银行错误回报
void CTradeSpi::OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo) {};

///期货发起查询银行余额错误回报
void CTradeSpi::OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount, CThostFtdcRspInfoField *pRspInfo) {};

///期货发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
void CTradeSpi::OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField *pRspRepeal) {};

///期货发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
void CTradeSpi::OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField *pRspRepeal) {};

///期货发起银行资金转期货应答
void CTradeSpi::OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///期货发起期货资金转银行应答
void CTradeSpi::OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///期货发起查询银行余额应答
void CTradeSpi::OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///银行发起银期开户通知
void CTradeSpi::OnRtnOpenAccountByBank(CThostFtdcOpenAccountField *pOpenAccount) {};

///银行发起银期销户通知
void CTradeSpi::OnRtnCancelAccountByBank(CThostFtdcCancelAccountField *pCancelAccount) {};

///银行发起变更银行账号通知
void CTradeSpi::OnRtnChangeAccountByBank(CThostFtdcChangeAccountField *pChangeAccount) {};

/// 当收到合约价位查询应答时回调该函数
void CTradeSpi::onRspMBLQuot(CThostMBLQuotData *pMBLQuotData, CThostFtdcRspInfoField *pRspMsg, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("当收到合约价位查询应答时回调该函数-%d\n", nRequestID);
	LOG("当收到合约价位查询应答时回调该函数-OnRspQrySettlementInfoConfirm-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspMsg)
	{
		printf("当收到合约价位查询应答时回调该函数：Msg-%d-%s\n", pRspMsg->ErrorID, pRspMsg->ErrorMsg);
		LOG("pRspInfo:errID-" << pRspMsg->ErrorID << ",errMsg-" << pRspMsg->ErrorMsg << "\n");
	}
	if (NULL != pMBLQuotData)
	{
		LOG("pMBLQuotData:ContractID-" << pMBLQuotData->InstrumentID
			<< ",BsFlag-" << pMBLQuotData->Direction
			<< ",Qty-" << pMBLQuotData->Volume
			<< ",Price-" << pMBLQuotData->Price << "\n");
	}
};

void CTradeSpi::quit()
{
	running = 0;
};

void CTradeSpi::CancelOrd(zc::PlannedOrderItem* pOrd)
{
	int thid = GetCurrentThreadId();
	LOG("cancel ord of ref: " << pOrd->ordRef << std::endl );
	CThostFtdcInputOrderActionField act;
	strncpy(act.InstrumentID, pOrd->instId.c_str(), sizeof(TThostFtdcInstrumentIDType));
	strncpy(act.ExchangeID, pOrd->exchId.c_str(), sizeof(TThostFtdcExchangeIDType));
	strncpy(act.OrderSysID, pOrd->ordsysId.c_str(), sizeof(TThostFtdcOrderSysIDType));
	act.ActionFlag = THOST_FTDC_AF_Delete;
	sprintf(act.OrderRef, "%012ld", pOrd->ordRef);
	//act.LimitPrice = pOrd->price;
	//act.OrderActionRef = pOrd->ordRef;
	strncpy(act.UserID, m_loginField.UserID, sizeof(TThostFtdcUserIDType));
	strncpy(act.InvestorID, pOrd->isSpot?InvestorID_Spot:InvestorID_Future, sizeof(TThostFtdcInvestorIDType));
	strncpy(act.BrokerID, "", sizeof(TThostFtdcBrokerIDType));
	int ret = m_pReqApi->ReqOrderAction(&act, GetRequsetID());
	if (0 != ret)
	{
		LOG("撤单指令发送失败！\n" );
	}
}

zc::PlannedOrderItem* CTradeSpi::getLocalOrder(const char* ref)
{
	int thid = GetCurrentThreadId();
	long or = atol(ref);
	zc::PlannedOrderItem* po = nullptr;

	int blocked = 0;
	while (InterlockedExchange64(&zc::Arbitrage::spin_Locker_ordbook, TRUE))
	{
		blocked++;
		Sleep(0); 
	}
	LOG("blocked at getLocalOrder times:" << blocked << std::endl );

	//std::cout << "getLocalOrder... ordbook size:" << zc::Arbitrage::ordbook.size() << std::endl;
	LOG("order ref to be found:" << ref << std::endl );
	for (auto jt = zc::Arbitrage::ordbook.begin(); jt != zc::Arbitrage::ordbook.end(); ++jt)LOG("refs in ordbook:" << (*jt)->ordRef << std::endl);

	auto it = std::find_if(zc::Arbitrage::ordbook.begin(), zc::Arbitrage::ordbook.end(), [or](zc::PlannedOrderItem* in){return or == in->ordRef; });
	if (zc::Arbitrage::ordbook.end() != it) po = (*it);
	InterlockedExchange64(&zc::Arbitrage::spin_Locker_ordbook, FALSE);

	return po;
}

long CTradeSpi::Send(const char* instId, int lot, char dir, double prc, char ocflg, bool isSpot)
{
	int thid = GetCurrentThreadId();
	long thisOrf = OrderRef;
	CThostFtdcInputOrderField InputOrder;
	memset(&InputOrder, 0, sizeof(InputOrder));
	strncpy(InputOrder.BrokerID, m_loginField.BrokerID, sizeof(InputOrder.BrokerID));//会员号
	strncpy(InputOrder.UserID, m_loginField.UserID, sizeof(InputOrder.UserID));//交易员
	strncpy(InputOrder.InvestorID, isSpot ? InvestorID_Spot : InvestorID_Future, sizeof(TThostFtdcInvestorIDType));
	sprintf(InputOrder.OrderRef, "%012ld", thisOrf);//本地保单号
	LOG("cur sended OrderRef: " << InputOrder.OrderRef << std::endl );
	//InputOrder.Direction = THOST_FTDC_D_Buy;//买卖
	//InputOrder.CombHedgeFlag[0] = '4';//THOST_FTDC_HF_Speculation;//投机
	InputOrder.CombOffsetFlag[0] = ocflg;//开平
	InputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;

	InputOrder.LimitPrice = prc;

	///交易合约
	strcpy(InputOrder.InstrumentID, instId);
	///交易方向
	InputOrder.Direction = dir;
	///投机套保标记
	InputOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///有效期类型
	InputOrder.TimeCondition = THOST_FTDC_TC_GFD;//当日有效
	///GTD日期
	//sprintf(InputOrder.GTDDate, "%s", pTradeDate);
	///成交量
	InputOrder.VolumeTotalOriginal = lot;
	///成交量类型
	InputOrder.VolumeCondition = THOST_FTDC_VC_AV;//任何数量，可以试试THOST_FTDC_VC_CV
	///最小成交量
	InputOrder.MinVolume = 1;
	///触发条件
	InputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;//立即
	///止损价
	InputOrder.StopPrice = 0.0;
	//if (THOST_FTDC_OF_Open != ocflg)
	{
		///强平原因
		InputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;//非强平
		///用户强平标志
		InputOrder.UserForceClose = 0;
	}
	///自动挂起标志
	InputOrder.IsAutoSuspend = 0;
	///业务单元
	sprintf(InputOrder.BusinessUnit, "%s", "");
	///请求编号
	InputOrder.RequestID = GetRequsetID();
	///互换单标志
	InputOrder.IsSwapOrder = 0;
	int ret = m_pReqApi->ReqOrderInsert(&InputOrder, InputOrder.RequestID);
	if (0 != ret)
	{
		LOG("委托失败!\n" );
	}
	return thisOrf;
}

long CTradeSpi::SendOpen(const char* instId, int lot, char dir, double prc, bool isSpot)
{
	int thid = GetCurrentThreadId();
	// zc::TRADE_OCFLAG oc
	LOG("send open:" << zc::tradedir(dir) << " " << lot << " lot at price " << prc << std::endl );
	// Send(int lot, char dir, float prc, char ocflg)
	return Send(instId, lot, dir, prc, THOST_FTDC_OF_Open, isSpot);
}

long CTradeSpi::SendClose(const char* instId, int lot, char dir, double prc, bool isSpot)
{
	int thid = GetCurrentThreadId();
	//zc::TRADE_OCFLAG oc
	LOG("send close:" << zc::tradedir(dir) << " " << lot << " lot at price " << prc << std::endl );
	return Send(instId, lot, dir, prc, isSpot ? THOST_FTDC_OF_Close : THOST_FTDC_OF_CloseToday, isSpot);
}

bool CTradeSpi::QryInvestorId()
{
	int thid = GetCurrentThreadId();
	memset(InvestorID_Future, 0, sizeof(TThostFtdcInvestorIDType));
	CThostFtdcQryInvestorField qfd;
	strcpy(qfd.BrokerID, "");
	strcpy(qfd.InvestorID, "");
	strcpy(InvestorID_Future, "");
	int ret = m_pReqApi->ReqQryInvestor(&qfd, GetRequsetID());
	if (0 != ret)
	{
		LOG("查询InvestorID失败！\n" );
	}
	int timeout = 100000000;
	for (; timeout >0 ; timeout--)
	{
		if (strlen(InvestorID_Future)>0)
		{
			LOG("查询InvestorID成功: " << InvestorID_Future << std::endl );
			return true;
		}
		Sleep(0);
	}
	LOG("查询InvestorID超时\n");
	return false;
}