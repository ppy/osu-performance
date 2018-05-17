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

private:
	class Field
	{
	public:
		Field(char* data) : _data{data} {}

		explicit operator const char*() const { return _data; }
		operator std::string() const { return static_cast<const char*>(*this); }
		operator s32() const { return xtoi(_data); }
		operator u32() const { return xtou(_data); }
		operator s64() const { return xtoi64(_data); }
		operator u64() const { return xtou64(_data); }
		operator f32() const { return xtof(_data); }
		operator f64() const { return xtod(_data); }
		operator bool() const { return static_cast<s32>(*this) != 0; }

		template<typename T, std::enable_if_t<std::is_enum<bare_type_t<T>>::value>...>
		operator T(){
			return static_cast<T>(static_cast<std::underlying_type_t<bare_type_t<T>>>(*this));
		}

	private:
		char* _data;
	};

public:
	Field Get(u32 i) const
	{
		if (IsNull(i))
			throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
		return Field{_row[i]};
	}

	template <typename T>
	T Get(u32 i) const { return static_cast<T>(Get(i)); }

private:
	QueryResult(MYSQL_RES* pRes);

	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> _pRes;
	MYSQL_ROW _row;

	friend class DatabaseConnection;
};
