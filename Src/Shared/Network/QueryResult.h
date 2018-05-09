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
	bool NextRow();

	inline s32 NumRows() { return (s32)mysql_num_rows(_pRes.get()); }

	// Entire current row - array of zero terminated strings
	inline char** CurrentRow() { return _row; }

	// Column entries of the current row
	inline bool IsNull(u32 i) const { return _row[i] == nullptr; }
	inline char* String(u32 i) const { return (char*)_row[i]; }
	inline bool Bool(u32 i) const { return (xtoi(_row[i]) != 0); }
	inline s32 S32(u32 i) const { return xtoi(_row[i]); }
	inline u32 U32(u32 i) const { return u32(xtou(_row[i])); }
	inline s64 S64(u32 i) const { return xtoi64(_row[i]); }
	inline u64 U64(u32 i) const { return xtou64(_row[i]); }
	inline f32 F32(u32 i) const { return xtof(_row[i]); }
	inline f64 F64(u32 i) const { return xtod(_row[i]); }

private:
	QueryResult(MYSQL_RES* pRes);

	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> _pRes;
	MYSQL_ROW  _row;

	friend class DatabaseConnection;
};
