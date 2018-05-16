#pragma once

#include <Shared.h>

DEFINE_LOGGED_EXCEPTION(QueryResultException);

inline int xtoi(const char *str)
{
	return ::atoi(str);
}

inline u32 xtou(const char *str)
{
	return ::strtoul(str, 0, 0);
}

inline s64 xtoi64(const char *str)
{
	return ::strtoull(str, 0, 0);
}

inline u64 xtou64(const char *str)
{
	return ::strtoull(str, 0, 0);
}

inline float xtof(const char *str)
{
	return static_cast<float>(::atof(str));
}

inline double xtod(const char *str)
{
	return ::atof(str);
}

class QueryResult
{
public:
	bool NextRow();

	inline s32 NumRows() { return (s32)mysql_num_rows(_pRes.get()); }
	inline s32 NumCols() { return (s32)mysql_num_fields(_pRes.get()); }

	// Entire current row - array of zero terminated strings
	inline char** CurrentRow() { return _row; }

	// Column entries of the current row
	inline bool IsNull(u32 i) const { return !_row[i]; }

	template <typename T>
	T Get(u32 i) const;

private:
	QueryResult(MYSQL_RES* pRes);

	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> _pRes;
	MYSQL_ROW _row;

	friend class DatabaseConnection;
};
