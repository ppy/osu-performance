#pragma once

#include <Shared.h>

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
	QueryResult(QueryResult&& other);
	QueryResult(QueryResult& other) = delete;

	QueryResult& operator=(QueryResult&& other);
	QueryResult& operator=(QueryResult& other) = delete;

	~QueryResult();

	bool NextRow();

	inline s32 NumRows() { return (s32)mysql_num_rows(_pRes); }

	// Entire current row - array of zero terminated strings
	inline char** CurrentRow() { return _Row; }

	// Column entries of the current row
	inline bool IsNull(u32 i) { return _Row[i] == nullptr; }
	inline char* String(u32 i) const { return (char*)_Row[i]; }
	inline bool Bool(u32 i) const { return (xtoi(_Row[i]) != 0); }
	inline s32 S32(u32 i) const { return xtoi(_Row[i]); }
	inline u32 U32(u32 i) const { return u32(xtou(_Row[i])); }
	inline s64 S64(u32 i) const { return xtoi64(_Row[i]); }
	inline u64 U64(u32 i) const { return xtou64(_Row[i]); }
	inline f32 F32(u32 i) const { return xtof(_Row[i]); }
	inline f64 F64(u32 i) const { return xtod(_Row[i]); }

private:
	QueryResult(MYSQL_RES* pRes);

	MYSQL_RES* _pRes;
	MYSQL_ROW  _Row;

	friend class DatabaseConnection;
};
