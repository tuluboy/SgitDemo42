#include "GetParam.h"
#include <string.h>
#include "../s_xml/tinyxml2.h"

using namespace tinyxml2;

CGetParam::CGetParam(void)
{
}


CGetParam::~CGetParam(void)
{
}

int CGetParam::GetIpField(char* pTradeIp,char* pQuotIp)
{
	XMLDocument doc;
	if(XML_NO_ERROR != doc.LoadFile("param.xml"))
		return -1;
	XMLElement* pRoot = doc.RootElement();
	if(0 != pRoot)
	{
		XMLElement* pServer = pRoot->FirstChildElement("server");
		if(0 != pServer)
		{
			XMLElement *pTip = pServer->FirstChildElement("tradeServer");
			if(0 != pTip)
			{
				const char* pText = pTip->GetText();
				sprintf(pTradeIp,"%s",0 == pText ? "" : pText);
			}

			pTip = pServer->FirstChildElement("quoteServer");
			if(0 != pTip)
			{
				const char* pText = pTip->GetText();
				sprintf(pQuotIp,"%s",0 == pText ? "" : pText);
			}
		}
		else
			return -3;
	}
	else
		return -2;
	return 0;
}
int CGetParam::GetLoginField(fstech::CThostFtdcReqUserLoginField& reqLoginField)
{
	XMLDocument doc;
	if(XML_NO_ERROR != doc.LoadFile("param.xml"))
		return -1;
		
	XMLElement* pRoot = doc.RootElement();
	if(0 != pRoot)
	{
		XMLElement* pLogin = pRoot->FirstChildElement("login");
		if(0 == pLogin)
			return -3;
		XMLElement* pUser = pLogin->FirstChildElement("userID");
		if(0 != pUser)
		{
			const char* pText = pUser->GetText();
			strcpy(reqLoginField.UserID,0 == pText ? "" : pText);
		}

		XMLElement* pPwd = pLogin->FirstChildElement("pwd");
		if(0 != pPwd)
		{
			const char* pText = pPwd->GetText();
			strcpy(reqLoginField.Password,0 == pText ? "" : pText);
		}
	}
	else 
		return -2;

	return 0;
}

int CGetParam::GetOrderInsertField(fstech::CThostFtdcInputOrderField& ReqOrderField)
{
	XMLDocument doc;
	if(XML_NO_ERROR != doc.LoadFile("param.xml"))
		return -1;

	XMLElement* pRoot = doc.RootElement();
	if(0 != pRoot)
	{
		XMLElement* pSendOrder = pRoot->FirstChildElement("sendOrder");
		if(0== pSendOrder)
			return -2;
		XMLElement* pItem = pSendOrder->FirstChildElement("investorID");
		if(0 != pItem)
		{
			const char* pText = pItem->GetText();
			if(0 != pText)
				strcpy(ReqOrderField.InvestorID,pText);
		}
		pItem = pSendOrder->FirstChildElement("instrument");
		if(0 != pItem)
		{
			const char* pText = pItem->GetText();
			if(0 != pText)
				strcpy(ReqOrderField.InstrumentID,pText);
		}
		pItem = pSendOrder->FirstChildElement("Price");
		if(0 != pItem)
		{
			const char* pText = pItem->GetText();
			ReqOrderField.LimitPrice = atof(pText);
		}
		pItem = pSendOrder->FirstChildElement("volume");
		if(0 != pItem)
		{
			const char* pText = pItem->GetText();
			ReqOrderField.VolumeTotalOriginal = atoi(pText);
		}
		pItem = pSendOrder->FirstChildElement("shFlag");
		if(0 != pItem)
		{
			const char* pText = pItem->GetText();
			ReqOrderField.VolumeTotalOriginal = atoi(pText);
			if(0 != pText)
				ReqOrderField.CombHedgeFlag[0] = atoi(pText)+'0';
		}
		pItem = pSendOrder->FirstChildElement("bsFlag");
		if(0 != pItem)
		{
			const char* pText = pItem->GetText();
			ReqOrderField.VolumeTotalOriginal = atoi(pText);
			if(0 != pText)
				ReqOrderField.Direction = atoi(pText)+'0';
		}
	}

	return 0;
}

int CGetParam::WriteTradecode(fstech::CThostFtdcTradingCodeField* pTradingCodeField)
{
	if(0 == pTradingCodeField)
		return -1;
	XMLDocument doc;
	if(XML_NO_ERROR != doc.LoadFile("param.xml"))
		return -1;

	XMLElement* pRoot = doc.RootElement();
	if(0 != pRoot)
	{
		XMLElement* pTradeCode = pRoot->FirstChildElement("tradecode");
		if(0 == pTradeCode)
		{
			pTradeCode = doc.NewElement("tradecode");
			pRoot->InsertEndChild(pTradeCode);
		}
		XMLElement* pItem = pTradeCode->FirstChildElement("item");
		bool bHave = false;
		while (0 != pItem)
		{
			if(0 == strcmp(pTradingCodeField->ClientID,pItem->Attribute("clientid")))
			{
				bHave = true;
				break;
			}
			pItem = pItem->NextSiblingElement();
		}

		pItem = doc.NewElement("item");
		pTradeCode->InsertEndChild(pItem);

		pItem->SetAttribute("investorid",pTradingCodeField->InvestorID);
		pItem->SetAttribute("exchangid",pTradingCodeField->ExchangeID);
		pItem->SetAttribute("clientid",pTradingCodeField->ClientID);

		
	}
	doc.SaveFile("param.xml");
	return 0;
}

