#include "SysLog.h"
#include "ArbOrdItem.h"
#include <fstream>


namespace zc
{
	ArbOrdItem::ArbOrdItem()
	{
	}

	ArbOrdItem::~ArbOrdItem()
	{
	}

	void ArbOrdItem::setLeftFirst(int thid)
	{
		first = left;
		left->condition = zc::LEG_CONDITION::EM_OK;
		left->status = zc::LEG_STATUS::EM_LEG_SENDREADY;
		second = right;
		right->condition = zc::LEG_CONDITION::EM_WAIT;
		right->status = zc::LEG_STATUS::EM_LEG_STATUS_NULL;
		LOG("set left first...\n");
	}

	void ArbOrdItem::setRightFirst(int thid)
	{
		second = left;
		//left->condition = zc::LEG_CONDITION::EM_WAIT;
		//left->status = zc::LEG_STATUS::EM_LEG_STATUS_NULL;
		first = right;
		//right->condition = zc::LEG_CONDITION::EM_OK;
		//right->status = zc::LEG_STATUS::EM_LEG_SENDREADY;
		LOG("set right first...\n"); 
	}

	void ArbOrdItem::setBothFirst(int thid)
	{
		LOG("both first...\n");
		first = left;
		left->condition = zc::LEG_CONDITION::EM_OK;
		left->status = zc::LEG_STATUS::EM_LEG_SENDREADY;
		second = right;
		right->condition = zc::LEG_CONDITION::EM_OK;
		right->status = zc::LEG_STATUS::EM_LEG_SENDREADY;
	}
}