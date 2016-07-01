#include <Shared.h>

#include "QueryResult.h"


CQueryResult::CQueryResult(MYSQL_RES* pRes)
	:
	_pRes(pRes),
	_Row(nullptr)
{
}


CQueryResult::CQueryResult(CQueryResult&& other)
{
	if(_pRes)
	{
		mysql_free_result(_pRes);
	}

	_pRes = other._pRes;
	_Row = other._Row;

	other._pRes = 0;
}


CQueryResult& CQueryResult::operator=(CQueryResult&& other)
{
	if(_pRes)
	{
		mysql_free_result(_pRes);
	}

	_pRes = other._pRes;
	_Row = other._Row;

	other._pRes = 0;

	return *this;
}


CQueryResult::~CQueryResult()
{
	// Clean up properly!
	if(_pRes)
	{
		mysql_free_result(_pRes);
	}
}

bool CQueryResult::NextRow()
{
	// Don't bother executing the SQL func... we don't have a valid result anyways!
	if(_pRes == NULL)
		return false;

	// Return wether we actually HAVE any more (or any) rows
	return ((_Row = mysql_fetch_row(_pRes)) != NULL);
}
