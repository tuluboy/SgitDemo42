// TradeChannel.h
// 交易通道接口抽象类
#ifndef __TradeChannel_xkejj_234_sjsdfjejnnvbf_xej32212mbj__
#define __TradeChannel_xkejj_234_sjsdfjejnnvbf_xej32212mbj__
struct AccountSumItem
{
	char AcctId[32];
	char UserName[32];
	float TotalCash; // 现金
	float TotalAsset;  // 总资产
	float AvailableFunds; // 可用资金
};

class IChannel
{
public:
	IChannel();
	virtual ~IChannel();

	///买入股票下单
	///参数：
	///   stockId -- 股票代码
	///   price -- 价格
	///   lot -- 下单量，单位（手）
	virtual int buy(const char* stockId, float price, int lot) = 0;
	
	///卖出股票下单
	///参数：
	///   stockId -- 股票代码
	///   price -- 价格
	///   lot -- 下单量，单位（手）
	virtual int sell(const char* stockId, float price, int lot) = 0;

	///期货买开下单
	///参数：
	///   instrumentId -- 期货合约id
	///   price -- 价格
	///   lot -- 下单量，单位（手）
	virtual int buyopen(const char* instrumentId, float price, int lot) = 0;

	///期货卖开下单
	///参数：
	///   instrumentId -- 期货合约id
	///   price -- 价格
	///   lot -- 下单量，单位（手）
	virtual int sellshort(const char* instrumentId, float price, int lot) = 0;

	///期货买平下单
	///参数：
	///   instrumentId -- 期货合约id
	///   price -- 价格
	///   lot -- 下单量，单位（手）
	virtual int sell2cover(const char* instrumenId, float price, int lot) = 0;

	///期货卖平下单
	///参数：
	///   instrumentId -- 期货合约id
	///   price -- 价格
	///   lot -- 下单量，单位（手）
	virtual int buy2cover(const char* instrumentId, float price, int lot) = 0;

	///买入看多期权
	///参数：
	///   optionId -- 期权合约id
	///   price -- 价格
	///   lot -- 下单量，单位（手）
	virtual int longcall(const char* optionId, float price, int lot) = 0;

	///卖出看多期权
	///参数：
	///   optionId -- 期权合约id
	///   price -- 价格
	///   lot -- 下单量，单位（手）
	virtual int shortcall(const char* optionId, float price, int lot) = 0;

	///买入看空期权
	///参数：
	///   optionId -- 期权合约id
	///   price -- 价格
	///   lot -- 下单量，单位（手）
	virtual int longput(const char* optionId, float price, int lot) = 0;

	///卖出看空期权
	///参数：
	///   optionId -- 期权合约id
	///   price -- 价格
	///   lot -- 下单量，单位（手）
	virtual int shortput(const char* optionId, float price, int lot) = 0;

	///账户查询
	///参数:
	///
	virtual int getAccountSummary(AccountSumItem& qryResult) = 0;

};
#endif