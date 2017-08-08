#ifndef __common_h_xjdkjwe_dkje3433242_dkkjjkdsfa__
#define __common_h_xjdkjwe_dkje3433242_dkkjjkdsfa__
#include "../s_include/SgitFtdcUserApiStruct.h"
#include <functional>
#include <vector>
#include <iostream>

// ����״̬���壺
// ���� - ��ʾ�����µ��ӿڷ��سɹ����յ��������ر��������������յ��˱���
// �ѱ� - �յ��������ر������������Ѿ������϶��еȴ����
// �ѳ� - ����ȫ���ɽ�
// ���� - �������ֳɽ�

class CTradeSpi;
namespace zc
{
	typedef long long LONGLONG;
	typedef enum TRADE_DIR{ EM_Long = THOST_FTDC_D_Buy, EM_Short = THOST_FTDC_D_Sell };
	typedef enum TRADE_OCFLAG{EM_Open = 3, EM_Close};
	typedef enum LEG_TYPE{ EM_NullLeg, EM_LeftLeg, EM_RightLeg, EM_Auto, EM_BothLegs }; // EM_BothLegs -- ����һ��
	typedef enum LEG_STATUS{ EM_LEG_STATUS_NULL, EM_LEG_SENDED, EM_LEG_SENDREADY, EM_LEG_ORDERED, EM_LEG_TRADED, EM_LEG_ParTRADED, EM_LEG_CANCELREADY, EM_LEG_CANCELSENED, EM_LEG_ParCANCELED, EM_LEG_CANCELED };
	// ������Լ״̬���壺0-���� 1-�ѱ� 2-���� 3-ȫ�� 4-���� 5-ȫ�� 6-ƽ�� 9-null
	typedef enum ARBI_STATUS{ EM_9_9, EM_0_0, EM_1_0, EM_1_1, EM_2_0, EM_3_0, EM_4_0, EM_4_5, EM_2_1, EM_1_2, EM_2_2, EM_4_1, EM_5_0, EM_6_1, EM_6_0, EM_3_5, EM_3_1, EM_1_3,  EM_6_4, EM_6_5, EM_3_2, EM_2_3, EM_3_4, EM_3_6, EM_6_6, EM_3_3, EM_ARB_LONG_OPENED, EM_ARB_LONG_CLOSED, EM_ARB_SHORT_OPENED, EM_ARB_SHORT_CLOSED };
	typedef enum ARBI_COMMAND{ EM_CMD_NULL, EM_SEND_ORD,EM_CANCEL_ORD};
	typedef enum LEG_CONDITION { EM_COND_NULL=9,EM_OK, EM_WAIT, EM_CANCEL }; // init-���������͵� wait-�ȴ���ʱ�͵� cancel-�������ϳ��� dead-ʧЧ�������������κβ���
	typedef enum ARBI_BALANCE{ EM_BL_NULL, EM_BL_CUT, EM_BL_FILL };
	struct ArbOrdItem;
	class Arbitrage;
	struct PlannedOrderItem
	{
		std::string instId;
		std::string exchId;
		std::string ordsysId;
		CTradeSpi* pTrdSpi;
		long ordRef;
		bool isSpot; // �ֻ�Ϊtrue
		zc::LEG_TYPE leg; // ������
		PlannedOrderItem* brotherLeg; // ��һ��
		fstech::CThostFtdcDepthMarketDataField* Tick;
		ArbOrdItem* arbiOrd; // ���������ĸ�������Լ
		TRADE_DIR dir; // ��������
		TRADE_OCFLAG ocFlag; // ��ƽ���
		int priceType; // 1 2 3 4
		int lot; // �͵�����
		int alot; // δ�ɽ�
		int elot; // �ѳɽ�
		int clot; // �ѳ�����;
		double price; // �͵���
		time_t ordCancleSendTime;
		time_t ordCancledTime;
		time_t ordTradedTime; // �ɽ��ɹ���ʱ��
		time_t ordOrderedTime; // �����ҵ��ɹ���ʱ��
		time_t ordSendedTime;
		time_t ordCreatedTime; // �����Ĵ���ʱ��,�൱�ڼƻ��´��ʱ��
		time_t curtime; // ��ǰʱ��
		int timeout; // ����Ϊ��λ����ʱ�ͳ�
		LEG_CONDITION condition; // 0 - ����Ч 1-�͵��������� 2-������������ 3-xxx  99-death�����������������κδ����������ظ�ʹ��һ���洢λ��
		LEG_STATUS status; // ���ȳɽ�״̬
		std::function<void(PlannedOrderItem* nextOrder)> orderedHook; // �͵��ɹ��󴥷����� -- ��������һ�������ĵ���
		std::function<void(PlannedOrderItem* nextOrder)> tradedHook; // �ɽ��󴥷����� -- �ɽ���Ϻ���� null
		std::function <void(PlannedOrderItem* nextOrder)> cancelHook; // �����ɹ��󴥷�����
		void setCloseOrd(int thid); // �ѳɽ��Ŀ��ֵ����͵ط�ƽ�ֵ�����д��Ӧ������֮ǰ���ֳɽ�ʣ�µ�ȫ����Ϊ��������
		void changeOrdPrc(int inPrc, int thid); // inΪ�����򽫼۸�ļ�����Ϊ�����ı���
		float GetMinPoint(int thid);
		float setPrice(int thid);
	};

	struct TradeRecItem
	{
		std::string date;
		std::string time;
		char dir; // ����
		char oc; // ����ƽ
		int tradeUnit;
		float leftCommision;
		float rightCommision;
	};

	/*
	������Լ�����Ⱥ����ȹ���
	����������Լ���������ȣ���������
	ƽ��������Լ���������ȣ���������

	����������Լ���������ȣ���������
	ƽ��������Լ���������ȣ���������
	*/
	class ArbiInstrument
	{
	public:
		ArbiInstrument()
		{
			init();
		};

		void init()
		{
			leftTick.LastPrice = -1;
			leftTick.AskPrice1 = -1;
			leftTick.BidPrice1 = -1;

			rightTick.LastPrice = -1;
			rightTick.AskPrice1 = -1;
			rightTick.BidPrice1 = -1;

			shortBasisSpread4 = -1; // ���ȹҼۣ����ȹҼ�
			shortBasisSpread3 = -1; // ���ȹҼۣ����ȶԼ�
			shortBasisSpread2 = -1; // ���ȶԼۣ����ȹҼ�
			shortBasisSpread1 = -1; // ���ȶԼۣ����ȶԼ�

			longBasisSpread1 = -1; // ���ȶԼۣ����ȶԼ�
			longBasisSpread2 = -1; // ���ȶԼۣ����ȹҼ�
			longBasisSpread3 = -1; // ���ȹҼۣ����ȶԼ�
			longBasisSpread4 = -1; // ���ȹҼۣ����ȹҼ�
		}

		~ArbiInstrument(){};

		void SetInst(const char* leftid, const char* rightid, const int leftunit, const int rightunit)
		{
			setLeftTradeUnit(leftunit);
			setRightTradeUnit(rightunit);
			leftleg = leftid;
			rightleg = rightid;
			ArbiInstID = leftleg + "-" + rightleg;
		}

		void setLeftTradeUnit(int nl){ leftTradeUnit = nl; }
		int getLeftTradeUnit(){ return leftTradeUnit; }

		void setRightTradeUnit(int nr){ rightTradeUnit = nr; }
		int getRightTradeUnit(){ return rightTradeUnit; }

		float getSpreadUpper1(){ return spreadUpper1; }
		float getSpreadUpper2(){ return spreadUpper2; }
		void setSpreadUpper1(float in1){ spreadUpper1 = in1;}
		void setSpreadUpper2(float in2){ spreadUpper2 = in2;}

		float getSpreadLower1(){ return spreadLower1; }
		float getSpreadLower2(){ return spreadLower2; }
		void setSpreadLower1(float in1){ spreadLower1 = in1;}
		void setSpreadLower2(float in2){ spreadLower2 = in2;}

		void UpdateSpread()
		{
			shortBasisSpread4 = rightTick.AskPrice1 - leftTick.BidPrice1; // ���ȹҼۣ����ȹҼ�
			longBasisSpread4 = rightTick.BidPrice1 - leftTick.AskPrice1;  // ���ȹҼۣ����ȹҼ�

			shortBasisSpread3 = rightTick.BidPrice1 - leftTick.BidPrice1; // ���ȹҼۣ����ȶԼ�
			longBasisSpread3 = rightTick.AskPrice1 - leftTick.AskPrice1;  // ���ȹҼۣ����ȶԼ�

			shortBasisSpread2 = rightTick.AskPrice1 - leftTick.AskPrice1; // ���ȶԼۣ����ȹҼ�
			longBasisSpread2 = rightTick.BidPrice1 - leftTick.BidPrice1;  // ���ȶԼۣ����ȹҼ�

			shortBasisSpread1 = rightTick.BidPrice1 - leftTick.AskPrice1; // ���ȶԼۣ����ȶԼ�
			longBasisSpread1 = rightTick.AskPrice1 - leftTick.BidPrice1;  // ���ȶԼۣ����ȶԼ�
		}
	public:
		std::string leftleg;  // ��Լid
		std::string rightleg; // ��Լid
		std::string ArbiInstID; // ��Լleftleg+rightleg
		LEG_TYPE spot; // ָ����һ�����ֻ�
		
		double leftMinPoint;
		double rightMinPoint;

		float shortBasisSpread4; // ���ȹҼۣ����ȹҼ�
		float shortBasisSpread3; // ���ȹҼۣ����ȶԼ�
		float shortBasisSpread2; // ���ȶԼۣ����ȹҼ�
		float shortBasisSpread1; // ���ȶԼۣ����ȶԼ�

		float longBasisSpread1; // ���ȶԼۣ����ȶԼ�
		float longBasisSpread2; // ���ȶԼۣ����ȹҼ�
		float longBasisSpread3; // ���ȹҼۣ����ȶԼ�
		float longBasisSpread4; // ���ȹҼۣ����ȹҼ�

		fstech::CThostFtdcDepthMarketDataField leftTick; // ���ȵ�ǰ�������
		time_t leftUpdateTime;
		fstech::CThostFtdcDepthMarketDataField rightTick; // ���ȵ�ǰ�������
		time_t rightUpdateTime;

	private:
		int leftTradeUnit; // ����2������
		int rightTradeUnit; // ����5������
		float spreadLower1; // �۲�����1
		float spreadLower2; // �۲�����2
		float spreadUpper1; // �۲�����1
		float spreadUpper2; // �۲�����2
	};

	struct InstumentInfo
	{
		std::string InstrumentId;
		double minpoint;
	};

	struct QryInstrumentFB
	{
		std::vector<InstumentInfo> Insts;
	};

	struct QryPositionFB
	{
		std::vector<fstech::CThostFtdcInvestorPositionField> posList;
	};

	// ����������ֵ����
	struct ParamRowItem4DbQry
	{
		std::string code;
		std::string groupname;
		float open_value;
		float close_value;
	};

	extern std::string tradedir(int dir);
};
#endif