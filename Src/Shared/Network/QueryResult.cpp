#include <Shared.h>

#include "QueryResult.h"

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

template<> char* QueryResult::Get(u32 i) const
{
	if (IsNull(i))
		throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
	return reinterpret_cast<char*>(_row[i]);
}

template<> std::string QueryResult::Get(u32 i) const
{
	if (IsNull(i))
		throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
	return Get<char*>(i);
}

template<> s32 QueryResult::Get(u32 i) const
{
	if (IsNull(i))
		throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
	return xtoi(_row[i]);
}

template<> u32 QueryResult::Get(u32 i) const
{
	if (IsNull(i))
		throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
	return xtou(_row[i]);
}

template<> s64 QueryResult::Get(u32 i) const
{
	if (IsNull(i))
		throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
	return xtoi64(_row[i]);
}

template<> u64 QueryResult::Get(u32 i) const
{
	if (IsNull(i))
		throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
	return xtou64(_row[i]);
}

template<> f32 QueryResult::Get(u32 i) const
{
	if (IsNull(i))
		throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
	return xtof(_row[i]);
}

template<> f64 QueryResult::Get(u32 i) const
{
	if (IsNull(i))
		throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
	return xtod(_row[i]);
}

template<> bool QueryResult::Get(u32 i) const
{
	if (IsNull(i))
		throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
	return Get<s32>(i) != 0;
}
