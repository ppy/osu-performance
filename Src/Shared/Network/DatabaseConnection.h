#pragma once

#include "../Core/Active.h"

#include "QueryResult.h"

DEFINE_LOGGED_EXCEPTION(CDatabaseException);

class CDatabaseConnection
{
public:
	CDatabaseConnection(
		std::string host,
		s16 port,
		std::string username,
		std::string password,
		std::string database);
	~CDatabaseConnection();

	void NonQueryBackground(const std::string& queryString);
	void NonQuery(const std::string& queryString);
	CQueryResult Query(const std::string& queryString);

	//kind of connection test, not really important
	bool ping();

	//returns error messages
	const char* Error();

	//returns the number of rows e.g. returned by a SELECT
	u32 AffectedRows();

	inline void escape(char* dest, char* src)
	{
		mysql_real_escape_string(_pMySQL, dest, src, strlen(src));
	}

	inline s32 get_FieldCount()
	{
		return mysql_field_count(_pMySQL);
	}

	size_t AmountPendingQueries()
	{
		return _pActive->AmountPending();
	}

private:
	void connect();

	std::unique_ptr<CActive> _pActive;
	std::mutex _dbMutex;

	std::string _host;
	s16 _port;
	std::string _username;
	std::string _password;
	std::string _database;

	MYSQL* _pMySQL;
};
