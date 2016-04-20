#pragma once

#include <Shared.h>


inline int xtoi(const char *_str)
{
	return ::atoi(_str);
}

inline u32 xtou(const char *_str)
{
	return ::strtoul(_str, 0, 0);
}

inline s64 xtoi64(const char *_str)
{
	return ::strtoull(_str, 0, 0);
}

inline u64 xtou64(const char *_str)
{
	return ::strtoull(_str, 0, 0);
}

inline float xtof(const char *_str)
{
	return static_cast<float>(::atof(_str));
}

inline double xtod(const char *_str)
{
	return ::atof(_str);
}


class CQueryResult
{
public:

	CQueryResult(CQueryResult&& other);
	CQueryResult(CQueryResult& other) = delete;

	CQueryResult& operator=(CQueryResult&& other);
	CQueryResult& operator=(CQueryResult& other) = delete;

	~CQueryResult();

	bool NextRow();

	inline s32 AmountRows() { return (s32)mysql_num_rows(_pRes); }

	// Entire current row - array of zero terminated strings
	inline char** CurrentRow() { return _Row; }

	// Column entries of the current row
	inline bool          IsNull(u32 Field) { return _Row[Field] == nullptr; }
	inline char*         String(u32 dwField) const { return (char*)_Row[dwField]; }
	inline unsigned long Ip(u32 dwField) const { return inet_addr(_Row[dwField]); }
	inline bool          Bool(u32 dwField) const { return (xtoi(_Row[dwField]) != 0); }
	inline int           S32(u32 dwField) const { return xtoi(_Row[dwField]); }
	inline u32           U32(u32 dwField) const { return u32(xtou(_Row[dwField])); }
	inline s64           S64(u32 dwField) const { return xtoi64(_Row[dwField]); }
	inline u64           U64(u32 dwField) const { return xtou64(_Row[dwField]); }
	inline float         F32(u32 dwField) const { return xtof(_Row[dwField]); }
	inline double        F64(u32 dwField) const { return xtod(_Row[dwField]); }


private:

	CQueryResult(MYSQL_RES* pRes);

	MYSQL_RES* _pRes;
	MYSQL_ROW  _Row;

	friend class CDatabaseConnection;
};
