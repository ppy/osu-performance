#pragma once

#define DEFINE_EXCEPTION(x) \
	class x : public Exception \
	{ public: x(std::string File, s32 Line, std::string Description) \
	: Exception{std::move(File), Line, std::move(Description)} {} }

#define DEFINE_LOGGED_EXCEPTION(x) \
	class x : public LoggedException \
	{ public: x(std::string File, s32 Line, std::string Description) \
	: LoggedException{std::move(File), Line, std::move(Description)} {} }

#define SRC_POS __FILE__,__LINE__

class Exception
{
public:
	Exception(std::string file, s32 line, std::string description);
	~Exception() = default;

	void Print() const;

	std::string Description() const { return _description; }

	std::string File() const { return _file; }

	s32 Line() const { return _line; }

protected:
	std::string _file;
	s32 _line;
	std::string _description;
};

class LoggedException : public Exception
{
public:
	LoggedException(std::string file, s32 fine, std::string description);
	~LoggedException() = default;

	void Log() const;
};
