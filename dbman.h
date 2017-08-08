#pragma once
#include "SysLog.h"
#include <mysql.h> 
#include <string>
#include <functional>

template<typename rowItem>
class dbQuery
{
public:
	dbQuery(MYSQL* in_conn, std::string sql, std::function<void(dbQuery<rowItem>* pq, MYSQL_ROW& Row, int col)> parser): conn(in_conn), qrySql(sql), nRows(0), rowParser(parser), allRowsData(nullptr)
	{
		curRow = 0;
	}
		
	~dbQuery(){ if (allRowsData)delete[] allRowsData; allRowsData = nullptr; };
	rowItem* allRowsData; // ½á¹û
	int nRows;
	int curRow;
	MYSQL* conn;
	std::string qrySql;
	std::function<void(dbQuery<rowItem>* pq, MYSQL_ROW& Row, int col)> rowParser; // ½«
	void doParse(int thid){
		MYSQL_RES *result;
		MYSQL_ROW row;
		//mysql_query(conn, qrySql.c_str());
		result = mysql_store_result(conn);
		if (!result)
		{
			LOG("db qry result null.....\n");
			return;
		}
		int num_fields = mysql_num_fields(result);
		nRows = mysql_num_rows(result);
		if (0 == nRows)
		{
			LOG("ERROR: select data returned 0 rows....\n");
		}

		allRowsData = new rowItem[nRows];
		curRow = 0;
		while ((row = mysql_fetch_row(result)))
		{
			for (int i = 0; i < num_fields; i++)
			{
				rowParser(this, row, i);
			}
			curRow++;
		}
	};
	void doQry(int thid)
	{
		mysql_query(conn, qrySql.c_str());
		doParse(thid);
	};
};

class dbman
{
public:
	dbman(std::string& ip, int port, std::string& userid, std::string& pass, std::string& libname);
	~dbman();
	std::string ip;
	int port;
	std::string userid;
	std::string pswd;
	std::string lib;
	MYSQL *conn;
	void init(int thid);
};

