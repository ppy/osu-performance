#include <Shared.h>

CException::CException(std::string file, s32 line, std::string description)
: _file{std::move(file)}, _line{line}, _description{std::move(description)}
{
}

void CException::Print()
{
	std::cerr << StrFormat("Exception in: {0}:{1} - {2}\n", _file, _line, _description);
}

CLoggedException::CLoggedException(std::string file, s32 line, std::string description)
: CException{std::move(file), line, std::move(description)}
{
	//Log();
}

void CLoggedException::Log()
{
	::Log(CLog::EType::Exception, StrFormat("{0}:{1} - {2}", _file, _line, _description));
}
