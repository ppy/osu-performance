#include <Shared.h>

Exception::Exception(std::string file, s32 line, std::string description)
: _file{std::move(file)}, _line{line}, _description{std::move(description)}
{
}

void Exception::Print() const
{
	std::cerr << StrFormat("Exception in: {0}:{1} - {2}\n", _file, _line, _description);
}

LoggedException::LoggedException(std::string file, s32 line, std::string description)
: Exception{std::move(file), line, std::move(description)}
{
}

void LoggedException::Log() const
{
	::Log(Except, StrFormat("{0}:{1} - {2}", _file, _line, _description));
}
