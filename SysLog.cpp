#include <assert.h>
#include "SysLog.h"
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <pthread.h>
#define TCHAR char
// thread_self()
using std::fstream;
using std::cout;
using std::endl;

const char LogFileLast[] = "logall.txt";
bool IsDirExist(const TCHAR* DirName);
bool CreateDir(const TCHAR* DirName);
unsigned long GetCurrentThreadId();

void GetLocalTime(struct timespec& ts)
{
    // CLOCK_REALTIME:系统实时时间,随系统实时时间改变而改变,即从UTC1970-1-1 0:0:0开始计时,中间时刻如果系统时间被用户该成其他,则对应的时间相应改变
    // CLOCK_MONOTONIC:从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
    // CLOCK_PROCESS_CPUTIME_ID:本进程到当前代码系统CPU花费的时间
    // CLOCK_THREAD_CPUTIME_ID:本线程到当前代码系统CPU花费的时间
    clock_gettime(CLOCK_REALTIME, &ts);
}

SysLog::SysLog()
{
    m_bEchoToggle = false; // 默认不打开
    m_nShowedId = 0; // 默认打印所有
    UpdateTime(); // 更新当前时间
}

void SysLog::
UpdateTime() // 获取最新时间
{
    GetLocalTime(m_systemtime); // 获取精度到毫秒
    struct tm* ptm = nullptr;
    ptm = localtime(&m_systemtime.tv_sec);
    m_DateNum = (ptm->tm_year+1900)*10000 + (ptm->tm_mon+1)*100 + ptm->tm_mday;
    m_TimeNum =  ptm->tm_hour*10000 + ptm->tm_min*100 + ptm->tm_sec;
    m_nanosec = m_systemtime.tv_nsec;
}

SysLog::~SysLog(void)
{
    // 将当日log文件合并为一个txt文件，并关闭文件
    std::fstream* pof;
    std::fstream tofile(LogFileLast, std::ios::app);
    if (!tofile)
    {
        cout<<LogFileLast<<" not exist!..."<<33578<<endl;
    }
    int length;
    for (auto it = FilesMap.begin(); it != FilesMap.end(); ++it)
    {
        pof = it->second;
        pof->flush();
        if(it->first < 77777)
        {
            pof->seekg(0, std::ios::end);
            length = pof->tellg();
            pof->seekg(0, std::ios::beg);
            char* buffer = new char[length];
            pof->read(buffer, length);
            char* pos = strrchr(buffer,'\n'); // 找到最后的一个换行符
            if(pos) *pos = '\0';
            UpdateTime();
            tofile<<buffer<<endl;
            tofile<<"*****"<< m_DateNum <<" "<< m_TimeNum<<"******************** thread "<<it->first<<"***********************"<<endl;
            delete[] buffer;
        }

        delete pof;
    }
    tofile.close();
}

// 每个线程一个文件
int SysLog::Log(const char * str, int streamid)
{
    UpdateTime();
    unsigned long threadid = streamid < 0 ? GetCurrentThreadId() : streamid;
    char LogFile[64];
    sprintf(LogFile, "%ld_%d.log", threadid, m_DateNum);

    map_it = FilesMap.find(threadid);
    std::fstream* pof = NULL;
    if(map_it == FilesMap.end())
    {
		if (!IsDirExist(("log")))
		{
			CreateDir(("log"));
		}
		std::string LogPath = ("./log/");
		LogPath.append(LogFile);
        // 新的文件，创建之
		// cout<<"线程"<<threadid<<"对应的log文件不存在，创建之..."<<endl; // 只是系统中不存在，不代表硬盘上不存在
		pof = new std::fstream(LogPath, std::ios::out); // 有则打开，无则新建
		if (pof->fail())
		{
			cout << ("\nlog file open failed!....");
			return -1;
		}
        FilesMap[threadid] = pof;
        // FilesMap.insert(std::map<int, std::fstream*>::value_type(threadid, pof));
    }
    else
    {
        //找到了文件
        pof = map_it->second;
    }
    // pdt->Update(); // 取最新时间
    *pof<<str<<"--"<<m_DateNum<<" "<< (m_TimeNum < 100000 ? "0" : "")<<m_TimeNum<<"."<<m_nanosec<<endl;
    pof->flush();
    
    // if(sms)sms<<""<<endl; // 发短信
    if(m_bEchoToggle)
    {
        if(threadid == m_nShowedId || 0 == m_nShowedId)
        {
            cout<<str<<"--"<<m_DateNum<<" "<< (m_TimeNum < 100000 ? "0" : "")<<m_TimeNum<<endl;
        }
    }
    return 0;
}

std::fstream* SysLog::GetLog(unsigned long thid)
{
    auto itfound = FilesMap.find(thid);
    if(itfound != FilesMap.end())
    {
        return FilesMap[thid];
    }
    else
    {
        std::cout<<"SysLog::GetLog - error thread id input"<<std::endl;
        assert(0);
        return nullptr;
    }
}

void SysLog::SetEchoToggle(int showid)
{
    if(showid !=0)
    {
        for(MapIt it = FilesMap.begin(); it!=FilesMap.end(); ++it)
        {
            if(showid == it->first) // 找到
            {
                m_bEchoToggle = !m_bEchoToggle;
                m_nShowedId = showid;
                return;
            }
        }
        // 没找到
        cout<<"没有找到相关的Log流，请重新输入需要显示的id"<<endl;
    }
    else
    {
        m_bEchoToggle = !m_bEchoToggle;
        cout<<(m_bEchoToggle? "打开": "关闭")<<"全部Log流的同步cout输出。。。"<<endl;
        m_nShowedId = 0;
    }
}

void SysLog::ShowLogStreamIds()
{
    cout<<"当前系统已记录的Log流id(等于当前运行线程id):"<<endl;
    for(MapIt it = FilesMap.begin(); it != FilesMap.end(); ++it)
    {
        cout<<endl<<"                Log流ID: "<<it->first<<endl;
    }
    cout<<"当前m_nShowedId值:          "<<m_nShowedId<<endl;
}

void SysLog::operator()(const char* str, int streamid)
{
	UpdateTime();
    char LogFile[128];
	unsigned long threadid = streamid < 0 ? GetCurrentThreadId() : streamid;
    sprintf(LogFile, "%ld_%d.log", threadid, m_DateNum);

    map_it = FilesMap.find(threadid);
    std::fstream* pof = NULL;
    if(map_it == FilesMap.end())
    {
		if (!IsDirExist(("log")))
		{
			CreateDir(("log"));
		}
		std::string LogPath = ("log\\");
		LogPath.append(LogFile);
        // 新的文件，创建之
		//cout<<"线程"<<threadid<<"对应的log文件不存在，创建之..."<<endl; // 只是系统中不存在，不代表硬盘上不存在
		pof = new fstream(LogPath, std::ios::out); // 有则打开，无则新建
		if (pof->fail())
		{
			//cout<<"\nopen log file error!..";
#ifdef win32
			MessageBox(NULL, ("open log file failed!"), ("LogErr"), MB_OK);
#endif
			return;
		}
        FilesMap[threadid] = pof;
    }
    else
    {
        //找到了文件
        pof = map_it->second;
    }

    *pof<<str<<"--"<<m_DateNum<<" "<< (m_TimeNum < 100000 ? "0" : "")<<m_TimeNum<<endl;
    // if(sms)sms<<""<<endl; // 发短信
    if(m_bEchoToggle)
    {
        if(threadid == m_nShowedId || 0 == m_nShowedId)
        {
            cout<<str<<"--"<<m_DateNum<<" "<< (m_TimeNum < 100000 ? "0" : "")<<m_TimeNum<<endl;
        }
    }
}
