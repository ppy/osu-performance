#include <Shared.h>

CException::CException(std::string File, s32 Line, std::string Description)
: m_File{std::move(File)}, m_Line{Line}, m_Description{std::move(Description)}
{
}

void CException::Print()
{
	std::cerr << StrFormat("Exception in: {0}:{1} - {2}\n", m_File, m_Line, m_Description);
}

CLoggedException::CLoggedException(std::string File, s32 Line, std::string Description)
: CException{std::move(File), Line, std::move(Description)}
{
	//Log();
}

void CLoggedException::Log()
{
	::Log(CLog::EType::Exception, StrFormat("{0}:{1} - {2}", m_File, m_Line, m_Description));
}
