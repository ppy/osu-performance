#pragma once

#include <Shared.h>

DEFINE_LOGGED_EXCEPTION(QueryResultException);

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
		operator std::string() const { return (const char*)*this; }

		operator s32() const { return atoi(_data); }
		operator u32() const { return strtoul(_data, 0, 0); }

		operator s64() const { return strtoll(_data, 0, 0); }
		operator u64() const { return strtoull(_data, 0, 0); }

		operator f32() const { return (f32)(f64)*this; }
		operator f64() const { return atof(_data); }

		operator bool() const { return (s32)*this != 0; }

		template<typename T, std::enable_if_t<std::is_enum<bare_type_t<T>>::value>...>
		operator T(){
			return static_cast<T>(static_cast<std::underlying_type_t<bare_type_t<T>>>(*this));
		}

	private:
		char* _data;
	};

public:
	Field operator[](size_t i) const
	{
		if (IsNull(i))
			throw QueryResultException{SRC_POS, StrFormat("Attempted to interpret null result at {0}.", i)};
		return Field{_row[i]};
	}

private:
	QueryResult(MYSQL_RES* pRes);

	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> _pRes;
	MYSQL_ROW _row;

	friend class DatabaseConnection;
};
