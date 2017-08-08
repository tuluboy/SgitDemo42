#include "SysLog.h"
#include "dbman.h"
#include <fstream>
#include <iostream>

dbman::dbman(std::string& ip_in, int port_in, std::string& userid_in, std::string& pass, std::string& libname) :ip(ip_in), port(port_in), userid(userid_in), pswd(pass), lib(libname)
{
}

dbman::~dbman()
{
}

void dbman::init(int thid)
{
	conn = mysql_init(NULL);
	if (conn == NULL)
	{
		LOG("Error " << mysql_errno(conn) << ": " << mysql_error(conn) << "\n");
		exit(1);
	}
	else LOG("mysql init ok!\n");
	if (mysql_real_connect(conn, ip.c_str(), userid.c_str(), pswd.c_str(), lib.c_str(), port, NULL, 0) == NULL)
	{
		LOG("Error "<< mysql_errno(conn)<<": "<<mysql_error(conn)<<"\n");
		exit(1);
	}
	else LOG("mysql connect ok!\n");
}
