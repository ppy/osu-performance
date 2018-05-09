#include <Shared.h>

#include "DatabaseConnection.h"

DatabaseConnection::DatabaseConnection(
	std::string host,
	s16 port,
	std::string username,
	std::string password,
	std::string database
) : _host{std::move(host)}, _port{port}, _username{std::move(username)}, _password{std::move(password)}, _database{std::move(database)}
{
	if (!mysql_init(&_mySQL))
		throw DatabaseException(SRC_POS, StrFormat("MySQL struct could not be initialized. ({0})", Error()));

	_isInitialized = true;

	connect();

	_pActive = Active::Create();
}

DatabaseConnection::DatabaseConnection(DatabaseConnection&& other)
{
	*this = std::move(other);
}

DatabaseConnection& DatabaseConnection::operator=(DatabaseConnection&& other)
{
	if (!other._pActive || other._pActive->IsBusy())
	{
		// Deconstruct the previous connection's active object
		// to force background tasks to finish.
		other._pActive = nullptr;
		_pActive = Active::Create();
	}
	else
		_pActive = std::move(other._pActive);

	_host = std::move(other._host);
	_port = other._port;
	_username = std::move(other._username);
	_password = std::move(other._password);
	_database = std::move(other._database);

	other._isInitialized = false;
	_isInitialized = true;

	_mySQL = other._mySQL;

	return *this;
}

DatabaseConnection::~DatabaseConnection()
{
	// Destruct our active object before closing the mysql connection.
	_pActive = nullptr;
	if (_isInitialized)
		mysql_close(&_mySQL);
}

void DatabaseConnection::connect()
{
	mysql_close(&_mySQL);
	if (!mysql_real_connect(&_mySQL, _host.c_str(), _username.c_str(), _password.c_str(), _database.c_str(), _port, nullptr, CLIENT_MULTI_STATEMENTS))
		throw DatabaseException(SRC_POS, StrFormat("Could not connect. ({0})", Error()));
}

void DatabaseConnection::NonQueryBackground(const std::string& queryString)
{
	// We arbitrarily decide, that we don't want to have more than 1000 pending queries
	while (NumPendingQueries() > 1000)
		// Avoid to have the processor "spinning" at full power if there is no work to do in Update()
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

	_pActive->Send([=]() { NonQuery(queryString); });
}

void DatabaseConnection::NonQuery(const std::string& queryString)
{
	// We don't want concurrent queries
	std::lock_guard<std::recursive_mutex> lock{_dbMutex};

	if (mysql_query(&_mySQL, queryString.c_str()) != 0)
		throw DatabaseException(SRC_POS, StrFormat("Error executing query {0}. ({1})", queryString, Error()));

	s32 status;
	do
	{
		/* did current statement return data? */
		MYSQL_RES* pRes = mysql_store_result(&_mySQL);
		if (pRes != nullptr)
			mysql_free_result(pRes);
		else          /* no result set or error */
		{
			if (mysql_field_count(&_mySQL) != 0)
				throw DatabaseException(SRC_POS, StrFormat("Error getting result. ({0})", Error()));
		}
		/* more results? -1 = no, >0 = error, 0 = yes (keep looping) */

		status = mysql_next_result(&_mySQL);
		if (status > 0)
			throw DatabaseException(SRC_POS, StrFormat("Error executing query {0}. ({1})", queryString, Error()));
	}
	while(status == 0);
}

QueryResult DatabaseConnection::Query(const std::string& queryString)
{
	// We don't want concurrent queries
	std::lock_guard<std::recursive_mutex> lock{_dbMutex};

	if (mysql_query(&_mySQL, queryString.c_str()) != 0)
		throw DatabaseException(SRC_POS, StrFormat("Error executing query {0}. ({1})", queryString, Error()));

	MYSQL_RES* pRes = mysql_store_result(&_mySQL);
	if (pRes == nullptr)
		throw DatabaseException(SRC_POS, StrFormat("Error getting result. ({0})", Error()));

	return QueryResult{pRes};
}

const char *DatabaseConnection::Error()
{
	// We don't want concurrent queries
	std::lock_guard<std::recursive_mutex> lock{_dbMutex};
	return mysql_error(&_mySQL);
}

u32 DatabaseConnection::AffectedRows()
{
	// We don't want concurrent queries
	std::lock_guard<std::recursive_mutex> lock{_dbMutex};
	return u32(mysql_affected_rows(&_mySQL));
}
