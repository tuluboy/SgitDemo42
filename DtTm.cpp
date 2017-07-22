#include "DtTm.h"
#include <math.h>
#include <cassert>

#include <ctype.h>

//#include <sstream>
//#include <stdio.h>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <iostream>

// 静态成员初始化
std::map<int, Day> DtTm::Calendar;

#ifndef win32
void GetLocalTime(struct timespec& ts);
#endif

void DtTm::Init(time_t secs)
{
    seconds = secs;
    m_tm = *localtime(&seconds);
#ifdef win32
    /*	t.tm_sec = st.wSecond;
        t.tm_min = st.wMinute;
        t.tm_hour = st.wHour;
        t.tm_mday = st.wDay;
        t.tm_mon = st.wMonth - 1;
        t.tm_year = st.wYear - 1900;
        t.tm_wday = st.wDayOfWeek;*/
    m_st.wSecond = m_tm.tm_sec;
    m_st.wMinute = m_tm.tm_min;
    m_st.wHour = m_tm.tm_hour;
    m_st.wDay = m_tm.tm_mday;
    m_st.wMonth = m_tm.tm_mon + 1;
    m_st.wYear = m_tm.tm_year + 1900;
    m_st.wDayOfWeek = m_tm.tm_wday;

    //m_st.wMilliseconds = 0;
    if(m_st.wMilliseconds > 1000)m_st.wMilliseconds = 0;

    m_nDateNum = m_st.wYear*10000 + m_st.wMonth*100 + m_st.wDay;
    m_fTimeNum = m_st.wHour*10000 + m_st.wMinute*100 + m_st.wSecond + float(m_st.wMilliseconds)/1000;
#else
    m_nDateNum = (m_tm.tm_year + 1900)*10000 +(m_tm.tm_mon + 1)*100 + m_tm.tm_mday;
    m_fTimeNum = m_tm.tm_hour*10000 + m_tm.tm_min*100 + m_tm.tm_sec + float(m_nanosec)/NANOSECNUM;
#endif

    IsTrading = IsTradingDate(m_nDateNum);
}

inline bool DtTm::IsTradingDate(const int daynum)
{
    std::map<int, Day>::iterator it = Calendar.find(daynum);
    return it == Calendar.end() ? false : true;
}

void DtTm::Init(time_t seconds, int millisec)
{
    assert(millisec >= 0 && millisec < 1000);
#ifdef win32
    m_st.wMilliseconds = millisec < 1000 ? millisec : 999;
#else
    m_nanosec = millisec*1000;
#endif
    Init(seconds);
}
//struct tm
//{
// int tm_sec;     /* seconds after the minute - [0,59] */
// int tm_min;     /* minutes after the hour - [0,59] */
// int tm_hour;    /* hours since midnight - [0,23] */
// int tm_mday;    /* day of the month - [1,31] */
// int tm_mon;     /* months since January - [0,11] */
// int tm_year;    /* years since 1900 */
// int tm_wday;    /* days since Sunday - [0,6] */
// int tm_yday;    /* days since January 1 - [0,365] */
// int tm_isdst;   /* daylight savings time flag */
//};

void DtTm::Init( int year,int month,int day,int hour,int minute,int second )
{
    if (year < 1900)
    {
        assert(0);
        year = 1970;
    }
    else if (month < 1 || month > 12)
    {
        assert(0);
        month = 0;
    }
    else if (day < 1 || day > 31)
    {
        assert(0);
        day = 1;
    }
    else if (hour < 0 || hour > 23)
    {
        assert(0);
        hour = 0;
    }
    else if (minute < 0 || minute > 59)
    {
        assert(0);
        minute = 0;
    }
    else if (second < 0 || second > 59)
    {
        assert(0);
        second = 0;
    }
    
#ifdef win32
    if(m_st.wMilliseconds > 1000)m_st.wMilliseconds = 0;
#endif
    m_tm.tm_year = year - 1900;
    m_tm.tm_mon = month - 1;
    m_tm.tm_mday = day;
    m_tm.tm_hour = hour;
    m_tm.tm_min = minute;
    m_tm.tm_sec = second;
	seconds = mktime(&m_tm);
	m_nDateNum = (m_tm.tm_year + 1900)*10000 +(m_tm.tm_mon + 1)*100 + m_tm.tm_mday;
#ifdef win32
	m_fTimeNum = m_tm.tm_hour * 10000 + m_tm.tm_min * 100 + m_tm.tm_sec;
#else
    m_nanosec = 0;
	m_fTimeNum = m_tm.tm_hour * 10000 + m_tm.tm_min * 100 + m_tm.tm_sec + float(m_nanosec) / NANOSECNUM;
#endif
    IsTrading = IsTradingDate(m_nDateNum);
}

void DtTm::Init(const char* strDateTime) // Init("09:34:23"); // 禁止此种用法
{
    Init(strDateTime, "%4d%2d%2d %d:%d:%d");
}

// 格式控制构造函数
// "20120904 09:35:24"
void DtTm::Init(const char* dateTimeStr, const char*  formaterStr)
{
    int year = 0, month = 0, day = 0, hour = 0, minutes = 0, seconds = 0;
    sscanf(dateTimeStr, formaterStr, &year, &month, &day, &hour, &minutes, &seconds);
    Init(year, month, day, hour, minutes, seconds);
}

void DtTm::Init(const char* dateStr, const char*  datefmt, const char* timestr, const char* timefmt, int Milliseconds)
{
#if 0
    if(Milliseconds >= 999) Milliseconds = 999;
    int year = 0, month = 0, day = 0, hour = 0, minutes = 0, seconds = 0;
    sscanf(dateStr, datefmt, &year, &month, &day);
    sscanf(timestr, timefmt, &hour, &minutes, &seconds);
    m_st.wMilliseconds = Milliseconds;
    Init(year, month, day, hour, minutes, seconds);
#endif

std::cout <<__FILE__<<__FUNCTION__<<__LINE__;
    assert(0);
}
// milisec < 1
void DtTm::Init(int date, float time, float milisec)
{
    assert(milisec<1);
    m_nDateNum = date;
    m_fTimeNum = time;
    div_t rs = div(date,10000);
    div_t md = div(rs.rem, 100);
    m_tm.tm_year = rs.quot - 1900;
    m_tm.tm_mon = md.quot - 1;
    m_tm.tm_mday = md.rem;
    rs = div((int)time, 10000);
    md = div(rs.rem, 100);
    m_tm.tm_hour = rs.quot;
    m_tm.tm_min = md.quot;
    m_tm.tm_sec = md.rem;
#ifndef win32
    m_nanosec = milisec * NANOSECNUM;
#endif
    m_fTimeNum += milisec;
    seconds = mktime(&m_tm);
    IsTrading = IsTradingDate(m_nDateNum);
}

void DtTm::Init(const char* datestr, const  char* timestr, const int milsec) // milsec 毫秒:0-999
{
    assert(milsec<1000);
    m_nDateNum = atoi(datestr);
    m_fTimeNum = ((((timestr[0]*10 + timestr[1])*10 + timestr[3])*10 + timestr[4])*10 + timestr[6])*10 + timestr[7] - 5333328;;
    div_t rs = div(m_nDateNum,10000);
    div_t md = div(rs.rem, 100);
    m_tm.tm_year = rs.quot - 1900;
    m_tm.tm_mon = md.quot - 1;
    m_tm.tm_mday = md.rem;
    rs = div((int)m_fTimeNum, 10000);
    md = div(rs.rem, 100);
    m_tm.tm_hour = rs.quot;
    m_tm.tm_min = md.quot;
    m_tm.tm_sec = md.rem;
#ifndef win32
    m_nanosec = milsec * 100000;
#endif
    m_fTimeNum += ((float)milsec)/1000;
    seconds = mktime(&m_tm);
    IsTrading = IsTradingDate(m_nDateNum);
}

#ifdef win32
void  DtTm::SystemTime2tm(const SYSTEMTIME& st, tm& t) // 将SYSTEMTIME 结构转换到tm结构，丢弃毫秒部分
{
    t.tm_sec = st.wSecond;
    t.tm_min = st.wMinute;
    t.tm_hour = st.wHour;
    t.tm_mday = st.wDay;
    t.tm_mon = st.wMonth - 1;
    t.tm_year = st.wYear - 1900;
    t.tm_wday = st.wDayOfWeek;
    //t.tm_isdst =  0;
    //t.tm_yday =  0;
}
#endif

// 用当前时间构造时间对象
DtTm::DtTm() // 返回精确到毫秒的当前系统时间
{
    if(Calendar.size() == 0)
    {
        // 读入交易日列表
        std::ifstream fs("Calendar.dat", std::ios::in); // 当前Calendar.dat存储的都是交易日
        if(!fs)
        {
			std::cout << "open file Calendar.dat failed!\n";
			//getchar();
			system("pause");
            exit(0);
        }
        int datenum;
        Day datvar;
        Day* thepre = NULL;
        //Day* thenext;
        while(!fs.eof())
        {
            fs>>datvar.strDate;
            datenum = atoi(datvar.strDate);
            datvar.nDate = datenum;
            datvar.Info = NULL;
            datvar.IsDelivery = false;
            datvar.Pre = thepre;
            Calendar.insert(std::map<int, Day>::value_type(datenum, datvar));
            if(thepre) Calendar[thepre->nDate].Next = &(Calendar[datenum]);
            thepre = &(Calendar[datenum]);
        }
		Calendar[datenum].Next = NULL; // 最后一个
        fs.close();
        
        // 读入股指交割日列表
        fs.open("IFDelivery.dat", std::ios::in); 
        if(!fs)
        {
			std::cout << "open file IFDelivery.dat failed!\n" << std::endl;
			//getchar();
			system("pause");
            exit(0);
        }
        int datnum;
        while(!fs.eof())
        {
            fs>>datnum;

			Calendar[datnum].IsDelivery = true;
        }
    }
	struct tm* ptm = nullptr;
#ifdef win32
    GetLocalTime(&m_st);
    SystemTime2tm(m_st, m_tm);
    seconds = mktime(&m_tm);
	ptm = localtime(&seconds);
#else
    clock_gettime(CLOCK_REALTIME, &ts);
    seconds = ts.tv_sec;
    m_nanosec = ts.tv_nsec;
	clock_gettime(CLOCK_REALTIME, &ts);
	ptm = localtime(&ts.tv_sec);
	seconds = ts.tv_sec;
	m_nanosec = ts.tv_nsec;
#endif
    
    m_nDateNum = (ptm->tm_year+1900)*10000 + (ptm->tm_mon+1)*100 + ptm->tm_mday;
    m_fTimeNum =  ptm->tm_hour*10000 + ptm->tm_min*100 + ptm->tm_sec;
    
    m_tm = *ptm;
    IsTrading = IsTradingDate(m_nDateNum);
}

//DtTm::DtTm(const DtTm& a)
//{
//    this.m_fTimeNum
//}

#ifdef win32
DtTm::DtTm(SYSTEMTIME& st)
{
    m_st = st;
    SystemTime2tm(m_st, m_tm);
    seconds = mktime(&m_tm);

    m_nDateNum = m_st.wYear*10000 + m_st.wMonth*100 + m_st.wDay;
    m_fTimeNum = m_st.wHour*10000 + m_st.wMinute*100 + m_st.wSecond + float(m_st.wMilliseconds)/1000;

}
#else
DtTm::DtTm(const timespec& in)
{
	Init(in.tv_sec, (int)(1000 * in.tv_nsec / NANOSECNUM));
}
#endif

DtTm::DtTm(const time_t secds)
{
    Init(secds, 0);
}



DtTm::DtTm(time_t secds, int msecs)
{
    Init(secds, msecs);
}

// 用年月日构造时间对象
DtTm::DtTm(int year,int month,int day)
{
    Init(year, month, day, 0, 0, 0);
}

DtTm::DtTm(int date, int numtime, float secfloat) // secfloat 小于1秒的部份,用小数表示
{
    Init(date, numtime, secfloat); 
}

// 用年月日时分秒构造时间对象
DtTm::DtTm(int year,int month,int day,int hour,int minute,int second)
{
    Init(year, month, day, hour, minute, second);
}

// 用时间字符串构造
// 字符串格式必须: 20120509 09:34:24
DtTm::DtTm(const char* strDateTime)
{
    Init(strDateTime, "%4d%2d%2d %d:%d:%d");
}

DtTm::DtTm(const char* strDateTime, const char* format)
{
    Init(strDateTime, format);
}

DtTm::DtTm(const int timenum, const char* format)
{
    char timestr[32];
	sprintf(timestr, "%d",timenum);
    Init(timestr, format);
}

DtTm DtTm::Parse(std::string strDateTime)
{
    return DtTm(strDateTime.c_str());
}

DtTm DtTm::Parse(const std::string strDateTime, const std::string format)
{
    return DtTm(strDateTime.c_str(), format.c_str());
}

double DtTm::Time2Num(const char* timestr)
{
    if(!timestr)return -1;
    return ((((timestr[0]*10 + timestr[1])*10 + timestr[3])*10 + timestr[4])*10 + timestr[6])*10 + timestr[7] - 5333328;
}

double DtTm::Time2Num(const char* timestr, int misec)
{
    return Time2Num(timestr) + ((float)misec)/1000;
}

int DtTm::Date2Num(const char* datestr)
{
    if(!datestr)return 0;
    return ((((datestr[0]*10 + datestr[1])*10 + datestr[2])*10 + datestr[3])*10 + datestr[4])*10 + datestr[5] - 5333328;
}
// 输出字符型时间，只支持如下字符串格式
// "20130507" -> 20130507
// "14:35:24" -> 143524
// "14:35:24:234" ->143524.234
// "20130507 14:35:24:234" ->20130507143524.124
// timestr 若为空字符串则返回0
double DtTm::Parse(const char* timestr) // 输入“20130507” 输出20130507
{
    return Time2Num(timestr);
}

int DtTm::DayOfWeek() const
{
    return this->m_tm.tm_wday;
}

int DtTm::DayOfYear() const
{
    return this->m_tm.tm_yday; /* days since January 1 - [0,365] */
}

int DtTm::DaysInMonth(const int year,const int month)
{
    static const int day_of_month[13] = {31,31,28,31,30,31,30,31,31,30,31,30,31};

    int m = (month < 1 || month > 12)? 0 : month;

    if (m == 2 && ( (year%4 == 0) && (year%100 != 0) ) || (year%400 == 0) )
    {
        return 29;
    }
    else
    {
        return day_of_month[m];
    }
}

// 为了效率，调用者必须确保date1和date2是交易日
int DtTm::TradeDateDist(int date1, int date2) // 两个日期之间的交易日个数
{
    if(date1 == date2) return 0;
    int frm = date1 < date2 ? date1 : date2;
    int to = date2 < date1 ? date2 : date1;
    std::map<int, Day>::iterator it1 =  Calendar.find(frm);
    
	assert(it1!=Calendar.end());

    std::map<int, Day>::iterator it2 =  Calendar.find(to);
    
	assert(it2!=Calendar.end());
    
    int n = 0;
    Day* p = &(it1->second);
    while((p!=NULL && p->nDate != date2))
    {
        p = p->Next;
        n++;
    }
    return n;
}


// 为了效率起见，调用方负责检查是否越界
DtTm DtTm::GetNextTradingDate(DtTm& t, int shft)
{
	assert(t.IsTrading);
	if(shft == 0) shft = 1;
	int dtdt = t.m_nDateNum;
	Day dt = Calendar[dtdt];
	int Af = abs(shft);
	int nextdatenum;
    for(int i = 0; i < Af; i++)
	{
		nextdatenum = shft > 0 ? dt.Next->nDate : dt.Pre->nDate;
	}
    nextdatenum = dt.Next->nDate;
    return DtTm(nextdatenum, "%4d%2d%2d %d:%d:%d");
}

// 调用前请确认本对象是交易日
// 为了效率起见，调用方负责检查是否越界
DtTm& DtTm::ShiftToTradingDate(int shft)
{
	if(shft == 0) return *this;
	assert(IsTrading);
	int dtdt = m_nDateNum;
	Day dt = Calendar[dtdt];
	int nextdatenum;
	int Af = abs(shft);
	for(int i = 0; i < Af; i++)
	{
		nextdatenum = shft > 0 ? dt.Next->nDate : dt.Pre->nDate;
	}
	char timestr[32];
	sprintf(timestr, "%d", nextdatenum);
	Init(timestr, "%4d%2d%2d %d:%d:%d");
	return *this;
}

bool DtTm::Equals(const DtTm *dateTime) const
{
    return this->seconds == dateTime->seconds;
}

bool DtTm::Equals(const DtTm *value1,const DtTm *value2)
{
    return value1->seconds == value2->seconds;
}

int DtTm::GetDay() const
{
    return this->m_tm.tm_mday;;
}

int DtTm::GetHour() const
{
    return this->m_tm.tm_hour;
}

int DtTm::GetMinute() const
{
    return this->m_tm.tm_min;
}

int DtTm::GetMonth() const
{
    return this->m_tm.tm_mon + 1;
}

DtTm DtTm::GetNow()
{
    return DtTm();
}

int DtTm::GetSecond() const
{
    return this->m_tm.tm_sec;
}

float DtTm::GetMiniSecond() const
{
#ifdef win32
    return this->m_st.wMilliseconds;
#else
    return -1;
#endif
}

int DtTm::GetYear() const
{
    return this->m_tm.tm_year+1900;
}

void DtTm::AddYears( const int years )
{
    this->m_tm.tm_year += (int)years;
    this->seconds = mktime(&m_tm);
    m_tm = *localtime(&seconds);
}

void DtTm::AddMonths(const int months)
{
    int a =(int)((this->m_tm.tm_mon + months)/12);
    this->m_tm.tm_year += a;
    this->m_tm.tm_mon = (int)((this->m_tm.tm_mon + months)%12);
    this->seconds = mktime(&this->m_tm);
	Init(this->seconds);
    //date = *localtime(&seconds);
}

void DtTm::AddDays(const int days)
{
    this->AddHours(days*24);
}

void DtTm::AddHours(const int hours)
{
    this->AddMinutes(hours*60);
}

void DtTm::AddMinutes(const int minutes)
{
    this->AddSeconds(minutes *60);
}

void DtTm::AddSeconds(const int seconds)
{
    this->seconds = this->seconds + seconds;
    Init(this->seconds);
    // this->date = *localtime(&this->seconds);
}

void DtTm::AddMilliSec(const int millisec)
{
#ifdef win32
    m_st.wMilliseconds += millisec;
    int sec = (int)(m_st.wMilliseconds/1000);
    if(sec > 0) AddSeconds(sec);
#endif
}

void DtTm::AddWeeks(const int weeks)
{
    this->AddDays(weeks*7);
}

void DtTm::SetDate(const char* DateStr) // 格式"20130809"
{
    int year, month, day, hour, minutes, seconds;
    hour = m_tm.tm_hour;
    minutes = m_tm.tm_min;
    seconds = m_tm.tm_sec;
    sscanf(DateStr, "%4d%2d%2d", &year, &month, &day);
    Init(year, month, day, hour, minutes, seconds);
}

// 设置时间,输入格式为"00:00:00"
void DtTm::SetTime(const char* TimeStr)
{
    int year, month, day, hour, min, sec;
    year = m_tm.tm_year +1900;
    month = m_tm.tm_mon + 1;
    day = m_tm.tm_mday;
    sscanf(TimeStr, "%d:%d:%d", &hour, &min, &sec);
    Init(year, month, day, hour, min, sec);
}

int DtTm::GetDayNum() const
{
    return this->m_nDateNum;
}

float DtTm::GetTimeNum() const // 精确到毫秒时间
{
    return this->m_fTimeNum;
}

void DtTm::Update()
{
#ifdef win32
    GetLocalTime(&m_st);
    SystemTime2tm(m_st, m_tm);

    this->m_nDateNum = m_st.wYear*10000 + m_st.wMonth*100 + m_st.wDay;
    this->m_fTimeNum = m_st.wHour*10000 + m_st.wMinute*100 + m_st.wSecond + float(m_st.wMilliseconds)/1000;
    this->seconds = mktime(&this->m_tm);
#else
    // GetLocalTime(m_systemtime); // 获取精度到毫秒
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm* ptm = nullptr;
    ptm = localtime(&ts.tv_sec);
    m_nDateNum = (ptm->tm_year+1900)*10000 + (ptm->tm_mon+1)*100 + ptm->tm_mday;
    m_fTimeNum =  ptm->tm_hour*10000 + ptm->tm_min*100 + ptm->tm_sec;
    seconds = ts.tv_sec;
    m_nanosec = ts.tv_nsec;
    m_tm = *ptm;
#endif
}

int DtTm::CompareTo(const DtTm *value) const
{
    return (int)(this->seconds - value->seconds);
}

int DtTm::Compare(const DtTm *t1,const DtTm *t2)
{
    return t1->CompareTo(t2);
}

std::string DtTm::ToString(const std::string formaterStr) const
{
    char s[256];
    strftime(s, sizeof(s), formaterStr.c_str(), &this->m_tm);
    return std::string(s);
}

std::string DtTm::ToString() const
{
    return this->ToString("%Y-%m-%d %H:%M:%S");
}

std::string DtTm::ToShortDateString() const
{
    return this->ToString("%Y%m%d");
}

std::string DtTm::ToShortTimeString() const
{
    return this->ToString("%H:%M:%S");
}

bool DtTm::operator ==( DtTm &datetime)
{
#ifdef win32
    return this->seconds == datetime.seconds && m_st.wMilliseconds == datetime.m_st.wMilliseconds;
#else
    return seconds == datetime.seconds && m_nanosec == datetime.m_nanosec;
#endif
}

bool DtTm::operator != (DtTm &datetime)
{
#ifdef win32
    return this->seconds != datetime.seconds || m_st.wMilliseconds != datetime.m_st.wMilliseconds;
#else
    return seconds != datetime.seconds || m_nanosec != datetime.m_nanosec;
#endif
}

bool DtTm::operator > (DtTm &dateTime)
{
#ifdef win32
    if(seconds == dateTime.seconds) return  m_st.wMilliseconds > dateTime.m_st.wMilliseconds;
    else return seconds > dateTime.seconds;
#else
    if(seconds == dateTime.seconds) return m_nanosec > dateTime.m_nanosec;
    else return seconds > dateTime.seconds;
#endif
}

bool DtTm::operator < (DtTm &datetime)
{
#if 0
    if(seconds == datetime.seconds) return  m_st.wMilliseconds < datetime.m_st.wMilliseconds;
    else return this->seconds < datetime.seconds;
#endif
    assert(0);
    return false;
}

bool DtTm::operator >=(DtTm &datetime)
{
#if 0
    if(seconds == datetime.seconds) return  m_st.wMilliseconds >= datetime.m_st.wMilliseconds;
    else return this->seconds >= datetime.seconds;
#endif
    assert(0);
    return false;
}

bool DtTm::operator <=(DtTm &datetime)
{
#if 0
    if(seconds == datetime.seconds) return  m_st.wMilliseconds <= datetime.m_st.wMilliseconds;
    return this->seconds <= datetime.seconds;
#endif
    assert(0);
    return false;
}

int DtTm::get_cur_trading_date()
{
    return m_fTimeNum < 210000 ? m_nDateNum : GetNextTradingDate(*this).m_nDateNum;
}

