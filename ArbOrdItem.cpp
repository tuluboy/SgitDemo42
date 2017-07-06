#include "ArbOrdItem.h"

namespace zc
{
	ArbOrdItem::ArbOrdItem()
	{
	}


	ArbOrdItem::~ArbOrdItem()
	{
	}

	void ArbOrdItem::setLeftFirst()
	{
		std::cout << "left";
		first = left;
		left->condition = zc::LEG_CONDITION::EM_OK;
		left->status = zc::LEG_STATUS::EM_LEG_SENDREADY;
		second = right;
		right->condition = zc::LEG_CONDITION::EM_WAIT;
		right->status = zc::LEG_STATUS::EM_LEG_STATUS_NULL;
		std::cout << " first...\n";
	}

	void ArbOrdItem::setRightFirst()
	{
		std::cout << "right";
		second = left;
		//left->condition = zc::LEG_CONDITION::EM_WAIT;
		//left->status = zc::LEG_STATUS::EM_LEG_STATUS_NULL;
		first = right;
		//right->condition = zc::LEG_CONDITION::EM_OK;
		//right->status = zc::LEG_STATUS::EM_LEG_SENDREADY;
		std::cout << " first...\n"; 
	}

	void ArbOrdItem::setBothFirst()
	{
		std::cout << "both first...\n";
		first = left;
		left->condition = zc::LEG_CONDITION::EM_OK;
		left->status = zc::LEG_STATUS::EM_LEG_SENDREADY;
		second = right;
		right->condition = zc::LEG_CONDITION::EM_OK;
		right->status = zc::LEG_STATUS::EM_LEG_SENDREADY;
	}
}