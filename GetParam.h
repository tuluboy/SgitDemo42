#ifndef GET_PARAM_H_DQK_20151229
#define GET_PARAM_H_DQK_20151229

#include "../s_include/SgitFtdcUserApiStruct.h"
#include "../s_include/SgitFtdcUserApiDataType.h"

class CGetParam
{
public:
	CGetParam(void);
	~CGetParam(void);
	int GetIpField(char* pTradeIp,char* pQuotIp);
	int GetLoginField(fstech::CThostFtdcReqUserLoginField& reqLoginField);
	int GetOrderInsertField(fstech::CThostFtdcInputOrderField& ReqOrderField);

	int WriteTradecode(fstech::CThostFtdcTradingCodeField* pTradingCodeField);
};


#endif