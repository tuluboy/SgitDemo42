#ifndef __zc_util_xjejugnbvxdh48968_dh5jjfkf_ckfjgn__
#define __zc_util_xjejugnbvxdh48968_dh5jjfkf_ckfjgn__

#include <string>


namespace zc
{
	std::string& strToUpperInSitu(std::string& str);

	template<typename T, class CHKAndIniter>
	void RecieveInput(const char* prompt, T& out, CHKAndIniter chk)
	{
		while (true)
		{
			std::cout << prompt;
			std::cin >> out;
			if (std::cin.fail())
			{
				std::cout << "input wrong,please input again!\n";
				std::cin.ignore();
				std::cin.clear();
				std::cin.sync();
			}
			else if (!chk(out))
			{
				std::cout << "wrong input,please try again!\n";
			}
			else
			{
				std::cin.ignore();
				std::cin.clear();
				std::cin.sync();
				return;
			}
		}
	}

	time_t GetCurTime();
	double round2minpoint(double price, double minpt);
}
#endif