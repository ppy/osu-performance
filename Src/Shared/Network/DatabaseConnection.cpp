#include <Shared.h>

#include "DatabaseConnection.h"


CDatabaseConnection::CDatabaseConnection(
	std::string host,
	s16 port,
	std::string username,
	std::string password,
	std::string database)
	: _host{host}, _port{port}, _username{username}, _password{password}, _database{database}
{
	_pMySQL = new MYSQL;

	if(!mysql_init(_pMySQL))
	{
		// Clean up in case of error
		delete _pMySQL;

		throw CDatabaseException(SRC_POS, StrFormat("MySQL struct could not be initialized. ({0})", Error()));
	}

	connect();

	_pActive = CActive::Create();
}


CDatabaseConnection::~CDatabaseConnection()
{
	// Destruct our active object before closing the mysql connection.
	_pActive = nullptr;

	mysql_close(_pMySQL);
	delete _pMySQL;
}


void CDatabaseConnection::connect()
{
	mysql_close(_pMySQL);
	while(!mysql_real_connect(_pMySQL, _host.c_str(), _username.c_str(), _password.c_str(), _database.c_str(), _port, NULL, CLIENT_MULTI_STATEMENTS))
	{
		Log(CLog::Error, StrFormat("Could not connect. ({0})", Error()));
	}
}


void CDatabaseConnection::NonQueryBackground(const std::string& queryString)
{
	// We arbitrarily decide, that we don't want to have more than 1000 pending queries
	while(AmountPendingQueries() > 1000)
	{
		// Avoid to have the processor "spinning" at full power if there is no work to do in Update()
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	_pActive->Send([=]() { NonQuery(queryString); });
}

void CDatabaseConnection::NonQuery(const std::string& queryString)
{
	// We don't want concurrent queries
	std::lock_guard<std::mutex> lock{_dbMutex};

	while(mysql_query(_pMySQL, queryString.c_str()) != 0)
	{
		Log(CLog::Error, StrFormat("Error executing query {0}. ({1})", queryString, Error()));
		connect();
	}

	s32 status;
	do
	{
		/* did current statement return data? */
		MYSQL_RES* pRes = mysql_store_result(_pMySQL);
		if(pRes != NULL)
		{
			mysql_free_result(pRes);
		}
		else          /* no result set or error */
		{
			if(mysql_field_count(_pMySQL) == 0)
			{
			}
			else  /* some error occurred */
			{
				throw CDatabaseException(SRC_POS, StrFormat("Error getting result. ({0})", Error()));
			}
		}
		/* more results? -1 = no, >0 = error, 0 = yes (keep looping) */

		status = mysql_next_result(_pMySQL);
		if(status > 0)
		{
			throw CDatabaseException(SRC_POS, StrFormat("Error executing query {0}. ({1})", queryString, Error()));
		}
	}
	while(status == 0);
}


CQueryResult CDatabaseConnection::Query(const std::string& queryString)
{
	// We don't want concurrent queries
	std::lock_guard<std::mutex> lock{_dbMutex};

	while(mysql_query(_pMySQL, queryString.c_str()) != 0)
	{
		Log(CLog::Error, StrFormat("Error executing query {0}. ({1})", queryString, Error()));
		connect();
	}

	MYSQL_RES* pRes = mysql_store_result(_pMySQL);
	if(pRes == NULL)
	{
		throw CDatabaseException(SRC_POS, StrFormat("Error getting result. ({0})", Error()));
	}

	return CQueryResult{pRes};
}


bool CDatabaseConnection::ping()
{
	// We don't want concurrent queries
	std::lock_guard<std::mutex> lock{_dbMutex};

	while(mysql_ping(_pMySQL) != 0)
	{
		connect();
	}

	return true;
}

const char *CDatabaseConnection::Error()
{
	// We don't want concurrent queries
	std::lock_guard<std::mutex> lock{_dbMutex};

	return mysql_error(_pMySQL);
}

u32 CDatabaseConnection::AffectedRows()
{
	// We don't want concurrent queries
	std::lock_guard<std::mutex> lock{_dbMutex};

	return u32(mysql_affected_rows(_pMySQL));
}
