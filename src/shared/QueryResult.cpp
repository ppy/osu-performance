#include <pp/common.h>
#include <pp/shared/QueryResult.h>

#include <mysql.h>

PP_NAMESPACE_BEGIN

QueryResult::QueryResult(MYSQL_RES* pRes)
: _pRes{pRes, &mysql_free_result}, _row{nullptr}
{
}

bool QueryResult::NextRow()
{
	// Don't bother executing the SQL func... we don't have a valid result anyways!
	if (!_pRes)
		return false;

	// Return wether we actually HAVE any more (or any) rows
	return ((_row = mysql_fetch_row(_pRes.get())) != nullptr);
}

PP_NAMESPACE_END
