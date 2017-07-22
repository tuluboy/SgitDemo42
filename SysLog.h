#ifndef _asdfsfs_sdsfsdf_sfssffsf_
#define _asdfsfs_sdsfsdf_sfssffsf_
//#include <fstream>
#include <unordered_map>
#include <time.h>
class fstream;
// 交易记录类
// 负责记录交易过程中所有的委托、撤单指令
// 同时记录一些执行时间统计工作
// 每个线程记录自己的日志
class SysLog
{
	typedef std::unordered_map<unsigned long ,std::fstream*>::iterator MapIt;
public:
    std::unordered_map<unsigned long, std::fstream*> FilesMap; // int为流的id，一般取当前线程的id
    std::unordered_map<unsigned long, std::fstream*>::iterator map_it;

	void UpdateTime();
	int Log(const char* str, int streamid = -1);
    void operator()(const char* str, int streamid = -1);
    std::fstream* GetLog(unsigned long streamid);
	void SetEchoToggle(int showid = 0); // showid 准备显示的流id，默认0表示显示所有
	void ShowLogStreamIds(); // 显示当前系统中的thread id
	bool m_bEchoToggle;
public:
	SysLog();
	~SysLog(void);
	char LogBuf[12];
private:
	// 会导致多线程麻烦 std::string Buf; // 共用字符串空间
	// 各种格式的时间
    timespec	m_systemtime; // 最精确时间
	tm m_tm; // tm格式时间
	int m_DateNum; // 数字格式的日期 20130509
	int  m_TimeNum; // 数字格式的时间如 140559.234
    long m_nanosec; // 数字格式的纳秒
    char m_DateStr[16]; // 字符串格式日期
	char m_TimeStr[16]; // 字符串格式时间
	int m_nShowedId; // 允许显示到屏幕的流id
};
#endif /* _asdfsfs_sdsfsdf_sfssffsf_ */