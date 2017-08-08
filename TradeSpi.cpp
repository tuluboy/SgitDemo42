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
	std::cout << "traderspi ���ӳɹ�...\n";
	LOG("traderspi ���ӳɹ�...\n");
	std::cout << "traderspi ��½...\n";
	LOG("traderspi ��½...\n");
	m_pReqApi->ReqUserLogin(&m_loginField, 0);
	LOG("��¼:" << m_loginField.UserID << "\n");
};

///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
///@param nReason ����ԭ��
///        0x1001 �����ʧ��
///        0x1002 ����дʧ��
///        0x2001 ����������ʱ
///        0x2002 ��������ʧ��
///        0x2003 �յ�������
void CTradeSpi::OnFrontDisconnected(int nReason){
	int thid = GetCurrentThreadId();
	LOG( "OnFrontDisconnected" << "\n"<< "CTradeSpi::OnFrontDisconnected" << "\n");
	bConnected = false;
};

///������ʱ���档����ʱ��δ�յ�����ʱ���÷��������á�
///@param nTimeLapse �����ϴν��ձ��ĵ�ʱ��
void CTradeSpi::OnHeartBeatWarning(int nTimeLapse){};

///�ͻ�����֤��Ӧ
void CTradeSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};


///��¼������Ӧ
void CTradeSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	bConnected = false;
	LOG("��¼������Ӧ-OnRspUserLogin-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		if (0 == pRspInfo->ErrorID)
		{
			OrderRef = atol(pRspUserLogin->MaxOrderRef);
			LOG("���׵�¼�ɹ� --" << "MaxOrderRef returned as value: " << OrderRef << "\n" );
			bConnected = true;
			if (false == bFirstLogin)
			{
				bFirstLogin = true;
				//��ѯ���ױ���
				//��ѯ��Լ
			}
		}
		else
		{
			LOG("���׵�¼ʧ�ܣ�\n");
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

///�ǳ�������Ӧ
void CTradeSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	bConnected = false;
	LOG("�ǳ�������Ӧ-OnRspUserLogout" << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("pRspInfo:errcode-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n" );
	}
	if (NULL != pUserLogout)
	{
		LOG("pUserLogout:BrokerID-" << pUserLogout->BrokerID << ",UserID-" << pUserLogout->UserID << "\n" );
	}
};

///�û��������������Ӧ
void CTradeSpi::OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();

	//dqk_log::CLogApi g_Log << thid;
	LOG("�û��������������Ӧ-" << nRequestID << "\n" << "�û��������������Ӧ - OnRspUserPasswordUpdate bIsLast - " << bIsLast << "\n" );
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


///�ʽ��˻��������������Ӧ
void CTradeSpi::OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
};


///����¼��������Ӧ
void CTradeSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	//printf("OnRspOrderInsert����¼��������Ӧ-%d.\n", nRequestID);
	//dqk_log::CLogApi g_Log << thid;
	//std::cout << "����¼��������Ӧ-OnRspOrderInsert-" << nRequestID << ",bIsLast-" << bIsLast << ",bIsLast-" << bIsLast << "\n";

	if (NULL != pRspInfo)
	{
		//printf("����¼��������Ӧ-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		//std::cout << "pRspInfo:msgCode-" << pRspInfo->ErrorID << ",msg-" << pRspInfo->ErrorMsg << "\n";
		if (0 == pRspInfo->ErrorID && 0 < iOrderAction)
		{
			CThostFtdcInputOrderActionField inputOrderAction;
			memset(&inputOrderAction, 0, sizeof(inputOrderAction));
			strcpy(inputOrderAction.UserID, pInputOrder->UserID);
			strcpy(inputOrderAction.InvestorID, pInputOrder->InvestorID);
			strcpy(inputOrderAction.InstrumentID, pInputOrder->InstrumentID);
			//���ر����ų���
			strcpy(inputOrderAction.OrderRef, pInputOrder->OrderRef);
			//ϵͳ�����ų���
			strcpy(inputOrderAction.OrderSysID, pInputOrder->OrderSysID);
			strcpy(inputOrderAction.ExchangeID, pInputOrder->ExchangeID);
			if (0 != m_pReqApi->ReqOrderAction(&inputOrderAction, 0))
				LOG("��������ʧ��" << "\n" );

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

///Ԥ��¼��������Ӧ
void CTradeSpi::OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///Ԥ�񳷵�¼��������Ӧ
void CTradeSpi::OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///��������������Ӧ
void CTradeSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	//dqk_log::CLogApi g_Log << thid;
	LOG("��������������Ӧ-OnRspOrderAction-" << nRequestID << ",bIsLast-" << bIsLast << "\n");

	if (NULL != pRspInfo)
	{
		LOG("��������������Ӧ pRspInfo:msgCode-" << pRspInfo->ErrorID << ",msg-" << pRspInfo->ErrorMsg << "\n");

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
			LOG("���������ܣ�" << "�ѳ�����" << pInputOrderAction->VolumeChange << "lot��\n" );
			po->clot += pInputOrderAction->VolumeChange;
			if (po->clot == po->lot) // ȫ��
			{
				po->status = zc::LEG_STATUS::EM_LEG_CANCELED;
			}
			else if (po->clot < po->lot)
			{
				po->status = zc::LEG_STATUS::EM_LEG_ParCANCELED;
			}
			else
			{
				LOG("�����������ѳ������ظ����㡣����\n");
			}
		}
	}
	else
	{
		LOG("������û���ҵ����ض�Ӧ����\n" );
	}
};

///��ѯ��󱨵�������Ӧ
void CTradeSpi::OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("��ѯ��󱨵�������Ӧ-OnRspOrderAction-" << nRequestID << ",bIsLast-" << bIsLast << "\n");

	if (NULL != pRspInfo)
	{
		LOG("��ѯ��󱨵�������Ӧ pRspInfo:msgCode-" << pRspInfo->ErrorID << ",msg-" << pRspInfo->ErrorMsg << "\n");
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

///Ͷ���߽�����ȷ����Ӧ
void CTradeSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("Ͷ���߽�����ȷ����Ӧ-OnRspSettlementInfoConfirm:������ȷ��." << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("������ȷ�� pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pSettlementInfoConfirm)
	{
		LOG("CThostFtdcSettlementInfoConfirmField:BrokerID-" << pSettlementInfoConfirm->BrokerID
			<< ",ConfirmDate-" << pSettlementInfoConfirm->ConfirmDate
			<< ",ConfirmTime-" << pSettlementInfoConfirm->ConfirmTime
			<< ",InvestorID-" << pSettlementInfoConfirm->InvestorID << "\n");
	}
};

///ɾ��Ԥ����Ӧ
void CTradeSpi::OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///ɾ��Ԥ�񳷵���Ӧ
void CTradeSpi::OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///ִ������¼��������Ӧ
void CTradeSpi::OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///ִ���������������Ӧ
void CTradeSpi::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///ѯ��¼��������Ӧ
void CTradeSpi::OnRspForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///����¼��������Ӧ
void CTradeSpi::OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///���۲���������Ӧ
void CTradeSpi::OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///������������������Ӧ
void CTradeSpi::OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField *pInputBatchOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�������¼��������Ӧ
void CTradeSpi::OnRspCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ������Ӧ
void CTradeSpi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("�����ѯ������Ӧ-OnRspQryOrder-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
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

///�����ѯ�ɽ���Ӧ
void CTradeSpi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("�����ѯ�ɽ���Ӧ-OnRspQryTrade-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("�����ѯ�ɽ���Ӧ: pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
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

///�����ѯͶ���ֲ߳���Ӧ
void CTradeSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("�����ѯͶ���ֲ߳���Ӧ-OnRspQryInvestorPosition-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
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
///�����ѯ�ʽ��˻���Ӧ
void CTradeSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	if (!bAccount)
		bAccount = true;
	else
		return;
	//printf("�����ѯ�ʽ��˻���Ӧ-%d\n",nRequestID);
	//dqk_log::CLogApi g_Log << thid;
	LOG("�����ѯ�ʽ��˻���Ӧ-OnRspQryTradingAccount-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	LOG("�����ѯ�ʽ��˻���Ӧ-OnRspQryTradingAccount-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("�����ѯ�ʽ��˻���Ӧ : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
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

///�����ѯͶ������Ӧ
void CTradeSpi::OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	//dqk_log::CLogApi g_Log << thid;
	LOG("�����ѯͶ������Ӧ-OnRspQryInvestor-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("�����ѯͶ������Ӧ : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n" );
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


///�����ѯ���ױ�����Ӧ
void CTradeSpi::OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	//dqk_log::CLogApi g_Log << thid;
	LOG("�����ѯ���ױ�����Ӧ-OnRspQryTradingCode-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("�����ѯ���ױ�����Ӧ : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
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

///�����ѯ��Լ��֤������Ӧ
void CTradeSpi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	//dqk_log::CLogApi g_Log << thid;
	LOG("�����ѯ��Լ��֤������Ӧ-OnRspQryInstrumentMarginRate-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("�����ѯ��Լ��֤������Ӧ��Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
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

///�����ѯ��Լ����������Ӧ
void CTradeSpi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("�����ѯ��Լ����������Ӧ-%d\n", nRequestID);

	LOG("�����ѯ��Լ����������Ӧ-OnRspQryInstrumentCommissionRate-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("�����ѯ��Լ����������Ӧ��Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
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

///�����ѯ��������Ӧ
void CTradeSpi::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("�����ѯ��������Ӧ-OnRspQryExchange-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		LOG("�����ѯ��������Ӧ : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
	}
	if (NULL != pExchange)
	{
		LOG("pExchange:ExchangeID-" << pExchange->ExchangeID
			<< ",ExchangeName-" << pExchange->ExchangeName
			<< ",ExchangeProperty-" << pExchange->ExchangeProperty << "\n");
	}

};

///�����ѯ��Ʒ��Ӧ
void CTradeSpi::OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ��Լ��Ӧ
void CTradeSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	// dqk_log::CLogApi log;
	//std::cout << "�����ѯ��Լ��Ӧ-OnRspQryInstrument-" << nRequestID << ",bIsLast-" << bIsLast << ",bIsLast-" << bIsLast << "\n";
	if (NULL != pRspInfo)
	{
		//log << "pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n";
		LOG("�����ѯ��Լ��Ӧ : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n" );
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

///�����ѯ������Ӧ
void CTradeSpi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯͶ���߽�������Ӧ
void CTradeSpi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	LOG("�����ѯͶ���߽�������Ӧ-OnRspQrySettlementInfo-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("�����ѯͶ���߽�������Ӧ��Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
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

///�����ѯת��������Ӧ
void CTradeSpi::OnRspQryTransferBank(CThostFtdcTransferBankField *pTransferBank, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯͶ���ֲ߳���ϸ��Ӧ
void CTradeSpi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("�����ѯת��������Ӧ-%d\n", nRequestID);
	//dqk_log::CLogApi g_Log << thid;
	LOG("�����ѯͶ���ֲ߳���ϸ��Ӧ-OnRspQrySettlementInfo-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("�����ѯת��������Ӧ��Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
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

///�����ѯ�ͻ�֪ͨ��Ӧ
void CTradeSpi::OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("�����ѯ�ͻ�֪ͨ��Ӧ-%d\n", nRequestID);
	//dqk_log::CLogApi g_Log << thid;
	LOG("�����ѯ�ͻ�֪ͨ��Ӧ-OnRspQryNotice-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("�����ѯ�ͻ�֪ͨ��Ӧ��Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
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

///�����ѯ������Ϣȷ����Ӧ
void CTradeSpi::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("�����ѯ������Ϣȷ����Ӧ-%d\n", nRequestID);
	//dqk_log::CLogApi g_Log << thid;
	LOG("�����ѯ������Ϣȷ����Ӧ-OnRspQrySettlementInfoConfirm-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspInfo)
	{
		printf("�����ѯ������Ϣȷ����Ӧ��Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
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

///�����ѯͶ���ֲ߳���ϸ��Ӧ
void CTradeSpi::OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///��ѯ��֤����ϵͳ���͹�˾�ʽ��˻���Կ��Ӧ
void CTradeSpi::OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ�ֵ��۵���Ϣ��Ӧ
void CTradeSpi::OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField *pEWarrantOffset, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯͶ����Ʒ��/��Ʒ�ֱ�֤����Ӧ
void CTradeSpi::OnRspQryInvestorProductGroupMargin(CThostFtdcInvestorProductGroupMarginField *pInvestorProductGroupMargin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ��������֤������Ӧ
void CTradeSpi::OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField *pExchangeMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ������������֤������Ӧ
void CTradeSpi::OnRspQryExchangeMarginRateAdjust(CThostFtdcExchangeMarginRateAdjustField *pExchangeMarginRateAdjust, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ������Ӧ
void CTradeSpi::OnRspQryExchangeRate(CThostFtdcExchangeRateField *pExchangeRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ�����������Ա����Ȩ����Ӧ
void CTradeSpi::OnRspQrySecAgentACIDMap(CThostFtdcSecAgentACIDMapField *pSecAgentACIDMap, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ��Ʒ���ۻ���
void CTradeSpi::OnRspQryProductExchRate(CThostFtdcProductExchRateField *pProductExchRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ��Ʒ��
void CTradeSpi::OnRspQryProductGroup(CThostFtdcProductGroupField *pProductGroup, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ��Ȩ���׳ɱ���Ӧ
void CTradeSpi::OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ��Ȩ��Լ��������Ӧ
void CTradeSpi::OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯִ��������Ӧ
void CTradeSpi::OnRspQryExecOrder(CThostFtdcExecOrderField *pExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯѯ����Ӧ
void CTradeSpi::OnRspQryForQuote(CThostFtdcForQuoteField *pForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ������Ӧ
void CTradeSpi::OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ��Ϻ�Լ��ȫϵ����Ӧ
void CTradeSpi::OnRspQryCombInstrumentGuard(CThostFtdcCombInstrumentGuardField *pCombInstrumentGuard, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ���������Ӧ
void CTradeSpi::OnRspQryCombAction(CThostFtdcCombActionField *pCombAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯת����ˮ��Ӧ
void CTradeSpi::OnRspQryTransferSerial(CThostFtdcTransferSerialField *pTransferSerial, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ����ǩԼ��ϵ��Ӧ
void CTradeSpi::OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///����Ӧ��
void CTradeSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("����Ӧ��-%d\n", nRequestID);
	LOG("����Ӧ��-OnRspError-" << nRequestID << ",bIsLast-" << bIsLast << "\n" );
	if (NULL != pRspInfo)
	{
		printf("����Ӧ��Msg-%d-%s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		LOG("pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n" );
	}
};

///����֪ͨ
void CTradeSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	int thid = GetCurrentThreadId();
	LOG("����֪ͨ-OnRtnOrder" << "\n");
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
			// �����ɹ���Ļر�
			LOG("���͵�������" << std::internal <<po->lot << " ����������" << po->clot << std::endl);
			/*
			if (po->clot == po->lot) // ȫ��
			{
				po->status = zc::LEG_STATUS::EM_LEG_CANCELED;
			}
			else
			{
				po->status = zc::LEG_STATUS::EM_LEG_ParCANCELED;
			}
			*/
			LOG("�����ɹ���\n");
			return;
		}

		po->ordOrderedTime = zc::GetCurTime();
		if (THOST_FTDC_OSS_Accepted == pOrder->OrderSubmitStatus && 0 == pOrder->VolumeTraded)
		{
			po->status = zc::LEG_STATUS::EM_LEG_ORDERED; // �ѱ�
			LOG("�ѱ��ɹ���\n");
			po->exchId = pOrder->ExchangeID;
			po->ordsysId = pOrder->OrderSysID;
		}
		else if (THOST_FTDC_OSS_InsertRejected == pOrder->OrderSubmitStatus)
		{
			LOG("order ref:" << pOrder->OrderRef << " is rejected!\n");
			po->status = zc::LEG_STATUS::EM_LEG_CANCELED; // ���ܣ�ϵͳ�Զ�����
		}
	}
	else
	{
		// û�ҵ�
		LOG("OnRtnOrder ERROR: local order: " << pOrder->OrderRef << " mei zhao dao mei zhao dao ################################ mei zhao dao\n" );
	}
};

///�ɽ�֪ͨ
void CTradeSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	int thid = GetCurrentThreadId();
	LOG("�ɽ�֪ͨ-OnRtnTrade : " << pTrade->OrderRef << "\n");
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
	// �ɽ����޸���һ��״̬
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
			LOG("������ȫ���ɽ�: "<<pTrade->Volume<<"\n");
		}
		else
		{
			po->status = zc::LEG_STATUS::EM_LEG_ParTRADED;
			LOG("�������ֳɽ�����: "<<pTrade->Volume<<"\n");
		}
	}
	else
	{
		// û�ҵ�����ref
		LOG("OnRtnTradeû���ҵ�����ref " << pTrade->OrderRef << "\n" );
	}
};


///����¼�����ر�
void CTradeSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	int thid = GetCurrentThreadId();
	LOG("����¼�����ر�-OnErrRtnOrderInsert");
	if (NULL != pRspInfo)
	{
		LOG("����¼�����ر� : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
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

///������������ر�
void CTradeSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{
	int thid = GetCurrentThreadId();
	LOG("������������ر�-OnErrRtnOrderAction" << "\n");
	if (NULL != pRspInfo)
	{
		LOG("������������ر� : pRspInfo:errID-" << pRspInfo->ErrorID << ",errMsg-" << pRspInfo->ErrorMsg << "\n");
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

///��Լ����״̬֪ͨ
void CTradeSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) {};

///����֪ͨ
void CTradeSpi::OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo) {};

///��ʾ������У�����
void CTradeSpi::OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField *pErrorConditionalOrder) {};

///ִ������֪ͨ
void CTradeSpi::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder) {};

///ִ������¼�����ر�
void CTradeSpi::OnErrRtnExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo) {};

///ִ�������������ر�
void CTradeSpi::OnErrRtnExecOrderAction(CThostFtdcExecOrderActionField *pExecOrderAction, CThostFtdcRspInfoField *pRspInfo) {};

///ѯ��¼�����ر�
void CTradeSpi::OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo) {};

///����֪ͨ
void CTradeSpi::OnRtnQuote(CThostFtdcQuoteField *pQuote) {};

///����¼�����ر�
void CTradeSpi::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo) {};

///���۲�������ر�
void CTradeSpi::OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo) {};

///ѯ��֪ͨ
void CTradeSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {};

///��֤���������û�����
void CTradeSpi::OnRtnCFMMCTradingAccountToken(CThostFtdcCFMMCTradingAccountTokenField *pCFMMCTradingAccountToken) {};

///����������������ر�
void CTradeSpi::OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField *pBatchOrderAction, CThostFtdcRspInfoField *pRspInfo) {};

///�������֪ͨ
void CTradeSpi::OnRtnCombAction(CThostFtdcCombActionField *pCombAction) {};

///�������¼�����ر�
void CTradeSpi::OnErrRtnCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo) {};

///�����ѯǩԼ������Ӧ
void CTradeSpi::OnRspQryContractBank(CThostFtdcContractBankField *pContractBank, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯԤ����Ӧ
void CTradeSpi::OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯԤ�񳷵���Ӧ
void CTradeSpi::OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ����֪ͨ��Ӧ
void CTradeSpi::OnRspQryTradingNotice(CThostFtdcTradingNoticeField *pTradingNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ���͹�˾���ײ�����Ӧ
void CTradeSpi::OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ���͹�˾�����㷨��Ӧ
void CTradeSpi::OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField *pBrokerTradingAlgos, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�����ѯ��������û�����
void CTradeSpi::OnRspQueryCFMMCTradingAccountToken(CThostFtdcQueryCFMMCTradingAccountTokenField *pQueryCFMMCTradingAccountToken, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///���з��������ʽ�ת�ڻ�֪ͨ
void CTradeSpi::OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField *pRspTransfer) {};

///���з����ڻ��ʽ�ת����֪ͨ
void CTradeSpi::OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField *pRspTransfer) {};

///���з����������ת�ڻ�֪ͨ
void CTradeSpi::OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField *pRspRepeal) {};

///���з�������ڻ�ת����֪ͨ
void CTradeSpi::OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField *pRspRepeal) {};

///�ڻ����������ʽ�ת�ڻ�֪ͨ
void CTradeSpi::OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField *pRspTransfer) {};

///�ڻ������ڻ��ʽ�ת����֪ͨ
void CTradeSpi::OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField *pRspTransfer) {};

///ϵͳ����ʱ�ڻ����ֹ������������ת�ڻ��������д�����Ϻ��̷��ص�֪ͨ
void CTradeSpi::OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {};

///ϵͳ����ʱ�ڻ����ֹ���������ڻ�ת�����������д�����Ϻ��̷��ص�֪ͨ
void CTradeSpi::OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {};

///�ڻ������ѯ�������֪ͨ
void CTradeSpi::OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField *pNotifyQueryAccount) {};

///�ڻ����������ʽ�ת�ڻ�����ر�
void CTradeSpi::OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo) {};

///�ڻ������ڻ��ʽ�ת���д���ر�
void CTradeSpi::OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo) {};

///ϵͳ����ʱ�ڻ����ֹ������������ת�ڻ�����ر�
void CTradeSpi::OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo) {};

///ϵͳ����ʱ�ڻ����ֹ���������ڻ�ת���д���ر�
void CTradeSpi::OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo) {};

///�ڻ������ѯ����������ر�
void CTradeSpi::OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount, CThostFtdcRspInfoField *pRspInfo) {};

///�ڻ������������ת�ڻ��������д�����Ϻ��̷��ص�֪ͨ
void CTradeSpi::OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField *pRspRepeal) {};

///�ڻ���������ڻ�ת�����������д�����Ϻ��̷��ص�֪ͨ
void CTradeSpi::OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField *pRspRepeal) {};

///�ڻ����������ʽ�ת�ڻ�Ӧ��
void CTradeSpi::OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�ڻ������ڻ��ʽ�ת����Ӧ��
void CTradeSpi::OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///�ڻ������ѯ�������Ӧ��
void CTradeSpi::OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

///���з������ڿ���֪ͨ
void CTradeSpi::OnRtnOpenAccountByBank(CThostFtdcOpenAccountField *pOpenAccount) {};

///���з�����������֪ͨ
void CTradeSpi::OnRtnCancelAccountByBank(CThostFtdcCancelAccountField *pCancelAccount) {};

///���з����������˺�֪ͨ
void CTradeSpi::OnRtnChangeAccountByBank(CThostFtdcChangeAccountField *pChangeAccount) {};

/// ���յ���Լ��λ��ѯӦ��ʱ�ص��ú���
void CTradeSpi::onRspMBLQuot(CThostMBLQuotData *pMBLQuotData, CThostFtdcRspInfoField *pRspMsg, int nRequestID, bool bIsLast)
{
	int thid = GetCurrentThreadId();
	printf("���յ���Լ��λ��ѯӦ��ʱ�ص��ú���-%d\n", nRequestID);
	LOG("���յ���Լ��λ��ѯӦ��ʱ�ص��ú���-OnRspQrySettlementInfoConfirm-" << nRequestID << ",bIsLast-" << bIsLast << "\n");
	if (NULL != pRspMsg)
	{
		printf("���յ���Լ��λ��ѯӦ��ʱ�ص��ú�����Msg-%d-%s\n", pRspMsg->ErrorID, pRspMsg->ErrorMsg);
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
		LOG("����ָ���ʧ�ܣ�\n" );
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
	strncpy(InputOrder.BrokerID, m_loginField.BrokerID, sizeof(InputOrder.BrokerID));//��Ա��
	strncpy(InputOrder.UserID, m_loginField.UserID, sizeof(InputOrder.UserID));//����Ա
	strncpy(InputOrder.InvestorID, isSpot ? InvestorID_Spot : InvestorID_Future, sizeof(TThostFtdcInvestorIDType));
	sprintf(InputOrder.OrderRef, "%012ld", thisOrf);//���ر�����
	LOG("cur sended OrderRef: " << InputOrder.OrderRef << std::endl );
	//InputOrder.Direction = THOST_FTDC_D_Buy;//����
	//InputOrder.CombHedgeFlag[0] = '4';//THOST_FTDC_HF_Speculation;//Ͷ��
	InputOrder.CombOffsetFlag[0] = ocflg;//��ƽ
	InputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;

	InputOrder.LimitPrice = prc;

	///���׺�Լ
	strcpy(InputOrder.InstrumentID, instId);
	///���׷���
	InputOrder.Direction = dir;
	///Ͷ���ױ����
	InputOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///��Ч������
	InputOrder.TimeCondition = THOST_FTDC_TC_GFD;//������Ч
	///GTD����
	//sprintf(InputOrder.GTDDate, "%s", pTradeDate);
	///�ɽ���
	InputOrder.VolumeTotalOriginal = lot;
	///�ɽ�������
	InputOrder.VolumeCondition = THOST_FTDC_VC_AV;//�κ���������������THOST_FTDC_VC_CV
	///��С�ɽ���
	InputOrder.MinVolume = 1;
	///��������
	InputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;//����
	///ֹ���
	InputOrder.StopPrice = 0.0;
	//if (THOST_FTDC_OF_Open != ocflg)
	{
		///ǿƽԭ��
		InputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;//��ǿƽ
		///�û�ǿƽ��־
		InputOrder.UserForceClose = 0;
	}
	///�Զ������־
	InputOrder.IsAutoSuspend = 0;
	///ҵ��Ԫ
	sprintf(InputOrder.BusinessUnit, "%s", "");
	///������
	InputOrder.RequestID = GetRequsetID();
	///��������־
	InputOrder.IsSwapOrder = 0;
	int ret = m_pReqApi->ReqOrderInsert(&InputOrder, InputOrder.RequestID);
	if (0 != ret)
	{
		LOG("ί��ʧ��!\n" );
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
		LOG("��ѯInvestorIDʧ�ܣ�\n" );
	}
	int timeout = 100000000;
	for (; timeout >0 ; timeout--)
	{
		if (strlen(InvestorID_Future)>0)
		{
			LOG("��ѯInvestorID�ɹ�: " << InvestorID_Future << std::endl );
			return true;
		}
		Sleep(0);
	}
	LOG("��ѯInvestorID��ʱ\n");
	return false;
}