#include "zc_util.h"
#include <ctime>
namespace zc
{
	std::string& strToUpperInSitu(std::string& str)
	{
		int len = str.length();
		for (int i = 0; i < len; i++)	str[i] = toupper(str[i]);
		return str;
	}

	time_t GetCurTime()
	{
		return  ::time(NULL);
	}

	double round2minpoint(double price, double minpt)
	{
		return int((price / minpt) + 0.5)*minpt;
	}
}