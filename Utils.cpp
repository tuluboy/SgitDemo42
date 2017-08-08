#include <vector>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include <iostream>
#ifndef WIN32
#include <dirent.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#else
#include <Windows.h>
#endif
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <fcntl.h>

// 判断目录是否存在
bool IsDirExist(const char* DirName)
{
#ifdef WIN32
    if(!DirName)return false;
    struct _stat fileStat;
    if((_stat(DirName, &fileStat) == 0) && (fileStat.st_mode & S_IFDIR))
    {
        return true;
    }
    return false;
#else
    if(!DirName)return false;
    if(!opendir(DirName))return false;
    return true;
#endif
}

bool CreateDir(const char* DirName)
{
#ifdef WIN32
	if (!CreateDirectory(DirName, NULL))
	{
		return false;
	}
	return true;
#else
    int status = mkdir(DirName, S_IRWXU|S_IROTH|S_IXOTH|S_IRWXG);
    return 0 == status?true:false;
#endif
}

// 检查目录是否存在，不存在就创建之
// DirName: "dirname"  or   "dir1/dir2"
void CheckDirExist(const char* DirName)
{
	namespace fs = boost::filesystem;
	fs::path fullpath(fs::initial_path());
	fs::path fpth;
	fullpath /= DirName;
	if (!fs::exists(fullpath))
	{
		// 创建多层子目录
		try{ fs::create_directories(fullpath); }
		catch (std::exception& e)
		{
			std::cout << "exception!!!![" << e.what() << "]" << std::endl;
		}
	}
}

#ifndef WIN32
unsigned long GetCurrentThreadId()
{
    return (unsigned)pthread_self();
}
#endif

int getch()
{
    return 0;
}


// 没有改动返回false
bool path_refine(std::string& path)
{
    boost::replace_all(path,"\\","/");
    if(path.back() != '/')
    {
        path.append("/");
        return true;
    }
    return false;
}

// 把末尾的斜杠去掉
bool path_trim(std::string& path)
{
    boost::replace_all(path, "\\", "/");
    if(path.back() == '/')
    {
        path.erase(path.end() - 1, path.end());
        return true;
    }
    return false;
}

int kbhit(void)
{
#ifndef WIN32
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }
#endif
    return 0;
}











