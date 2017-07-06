#include "dbman.h"
#include <iostream>



dbman::dbman(std::string& ip_in, int port_in, std::string& userid_in, std::string& pass, std::string& libname) :ip(ip_in), port(port_in), userid(userid_in), pswd(pass), lib(libname)
{
}

dbman::~dbman()
{
}

void dbman::init()
{
	conn = mysql_init(NULL);
	if (conn == NULL)
	{
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	if (mysql_real_connect(conn, ip.c_str(), userid.c_str(), pswd.c_str(), lib.c_str(), port, NULL, 0) == NULL)
	{
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	else std::cout << "mysql connect ok!\n";
}
