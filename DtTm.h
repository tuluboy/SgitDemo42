// DtTm.h 时间日期处理类
#ifndef _asxejvnbgp_sdfsdkfsls_sdfsa_234cvg_
#define _asxejvnbgp_sdfsdkfsls_sdfsa_234cvg_
// 高精度计时技术 http://www.ccsl.carleton.ca/~jamuir/rdtscpm1.pdf
#include <string>
//#include<boost/timer.hpp>
#include <time.h>
#include <map>

#define cpuid __asm __emit 0fh __asm __emit 0a2h
#define rdtsc __asm __emit 0fh __asm __emit 031h

#define NANOSECNUM 100000000
#define win32

#ifdef win32
#include <Windows.h>
#endif
// using namespace std;

// 交易日历节点数据
struct Day
{
	int nDate; // 日期的数字格式 如 20131125
	char strDate[9]; // 字符串格式 "20131129"
	bool IsDelivery; // 当天是否交割日
	char* Info; // 其他信息存储字符串
	Day* Pre; // 紧邻前一个交易日
	Day* Next; // 紧邻后一个交易日
	Day(){Pre = NULL; Next = NULL; Info = NULL;}
};


class DtTm
{
public:
    DtTm(); // 返回精确到毫秒的当前系统时间
    
	// DtTm(SYSTEMTIME& st);
    DtTm(time_t seconds);
#ifdef win32
	DtTm::DtTm(SYSTEMTIME& st);
#else
    DtTm(const timespec& in);
#endif

    DtTm(int date, int ntime, float halfsec); // milisec 小于1秒,用0.xxxx秒
    DtTm(int year,int month,int day);
    DtTm(int year,int month,int day,int hour,int minute,int second);
    DtTm(time_t seconds, int msecs); // msecs[0, 1000)
	DtTm(const char* timestr); // 输入字符串为"2012/09/08 11:25:59"
	DtTm(const char* timestr, const char* format);
	DtTm(const int timenum, const char* format); // 输入 20131125 
	// 拷贝构造函数
	// 暂不需要拷贝构造函数，默认的已经管用了
	static double Time2Num(const char* timestr); // 输入09:34:12 返回93412.0
    static double Time2Num(const char* timestr, int misec);
    static int Date2Num(const char* datestr); // 输入"20160315" 返回20160315
    static double Parse(const char* timestr); // 输入“20130507” 输出20130507
    static DtTm Parse(std::string strDateTime);//yyyy/MM/dd HH:mm:ss
    static DtTm Parse(std::string strDateTime, std::string format);

    static DtTm GetNow(); //返回当前日期时间

public:
	void AddMilliSec(const int millisec); // 毫秒加
    void AddSeconds(const int seconds); //将指定的秒数加到此实例的值上。
    void AddMinutes(const int minutes);//将指定的分钟数加到此实例的值上。
    void AddHours(const int hours);//将指定的小时数加到此实例的值上。
    void AddDays(const int days); //将指定的天数加到此实例的值上。
    void AddWeeks(const int weeks);//将指定的周数加到些实上的值上。
    void AddMonths(const int Months);//将指定的月份数加到此实例的值上。
    void AddYears(const int years); //将指定的年份数加到此实例的值上。
	void SetDate(const char* DateStr); // 格式"20130809"
	void SetTime(const char* TimeStr); // 格式"091500"
public:
    static int Compare(const DtTm *value1,const DtTm *value2);
    int CompareTo(const DtTm *value) const;

    bool Equals(const DtTm* dateTime) const;
    static bool Equals(const DtTm *value1,const DtTm *value2);

    std::string ToString() const;

	//format = "%Y-%m-%d %H:%M:%S" %Y=年 %m=月 %d=日 %H=时 %M=分 %S=秒
	std::string ToString(const std::string format) const;
	
    std::string ToShortDateString() const; // 将当前 DtTm 对象的值转换为其等效的短日期字符串表示形式。
	std::string ToShortTimeString() const; // 将当前时间转换为字符串
	//long GetDateNumber() const; // 将当前日期或时间用整型数据表示 如20130409 091258, 表示展示日期或\和时间
	//long GetTimeNumber() const; // 将当前日期或时间用整型数据表示 如20130409 091258, 表示展示日期或\和时间
	
public:
    int get_cur_trading_date(); // 考虑夜盘,夜盘时间算下一个交易日,注意星期五的夜盘算下周一
    
    int GetYear() const;//获取此实例所表示日期的年份部分。
    int GetMonth() const;//获取此实例所表示日期的年份部分。
    int GetDay() const;// 获取此实例所表示的日期为该月中的第几天。
    int GetHour() const;//获取此实例所表示日期的小时部分。
    int GetMinute() const;//获取此实例所表示日期的分钟部分
    int GetSecond() const;//获取此实例所表示日期的秒部分。
    float GetMiniSecond() const; // 获取此实例所表示的日期的毫秒部分
    
    int GetDayNum() const; // 返回形如 20130606的字符串型日期
    float GetTimeNum() const; // 返回形如 95058.500的数字型时间
    
    int DayOfWeek() const; //获取此实例所表示的日期是星期几。
    int DayOfYear() const;//记录今天是一年里面的第几天,从1月1日起,0-365
    void Update(); // 更新当前系统时间到本实例
    void update_trade_date(){ IsTrading = IsTradingDate(m_nDateNum);};
    static int DaysInMonth(const int year,const int months);//返回指定年和月中的天数。
    static DtTm GetNextTradingDate(DtTm& t, int shft = 1); // shft表示往后或往前多少个交易日，为0时无意义，转为1，即当前往后一个交易日，-1表示往前一个交易日
    static int TradeDateDist(int date1, int date2); // 两个日期之间的交易日个数
	DtTm& ShiftToTradingDate(int shft);
public:
    bool operator == (DtTm &dateTime);
    bool operator > (DtTm &dateTime);
    bool operator < (DtTm &dateTime);
    bool operator >= (DtTm &dateTime);
    bool operator <= (DtTm &dateTime);
    bool operator != (DtTm &dateTime);

public:
	unsigned long long  GetCycleCount()
	{
		//__asm _emit 0x0F
        //     __asm _emit 0x31
    }

    void Init(time_t seconds);
	void Init(int date, float time, float millisec);
	void Init(time_t seconds, int millisec);
    void Init(const char* datestr, const char* timestr, const int milsec);
    void Init(int year,int month,int day,int hour,int minute,int second);
	void Init(const char* strDateTim);
    void Init(const char* strDateTime, const char* format); // "%4d%2d%2d %d:%d:%d"
	void Init(const char* dateStr, const char*  datefmt, const char* timestr, const char* timefmt, int millisec = 0);

	// 多种时间存储格式
#ifdef win32
   	SYSTEMTIME m_st;
    void SystemTime2tm(const SYSTEMTIME& st, tm& t); // 将SYSTEMTIME 结构转换到tm结构，丢弃毫秒部分
#else
	timespec ts;
    long m_nanosec; // 数字格式的纳秒
#endif
    time_t seconds;//自1970起的秒数
    tm m_tm;
    int m_nDateNum; // 形如20130506
    float m_fTimeNum; // 精确到毫秒时间，形如 095015.500
	bool IsTrading; // 当日是否是交易日
	bool IsWorking; // 当天是否工作日
	//static 数据区
	static std::map<int, Day> Calendar; // 交易日历
	static bool IsTradingDate(int daynum);
};

#endif /* _asxejvnbgp_sdfsdkfsls_sdfsa_234cvg_ */