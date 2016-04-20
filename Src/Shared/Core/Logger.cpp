#include <Shared.h>

#include <iostream>
#include <iomanip>
#include <sstream>


void Log(CLog::EType Flags, std::string&& Text)
{
	static auto pLog = CLog::CreateLogger();
	pLog->Log(Flags, std::forward<std::string>(Text));
}

std::unique_ptr<CLog> CLog::CreateLogger()
{
	auto pLogger = std::make_unique<CLog>();

	// Reset initially (also blank line)
	pLogger->Log(EType::None, CONSOLE_RESET "");

#ifdef __DEBUG
	pLogger->Log(EType::Info, "Program runs in debug mode.");
#endif

	return pLogger;
}

CLog::CLog()
{
	m_pActive = CActive::Create();
}

CLog::~CLog()
{
#ifndef __WIN32

	// Reset
	Log(EType::None, CONSOLE_RESET);

#endif
}

void CLog::Log(EType Flags, std::string&& Text)
{
	m_pActive->Send([this, Flags, Text]() { LogText(Flags, Text); });
}

void CLog::LogText(EType Flags, std::string Text)
{
	EStream Stream;
	if(Flags & EType::Error ||
	   Flags & EType::CriticalError ||
	   Flags & EType::SQL ||
	   Flags & EType::Exception)
	{
		Stream = EStream::STDERR;
	}
	else
	{
		Stream = EStream::STDOUT;
	}

	auto TextOut = std::string{};

	if(!(Flags & EType::None))
	{
		// Display time format
		const auto Time = std::chrono::system_clock::now();
		const auto Now = std::chrono::system_clock::to_time_t(Time);

#ifdef __WIN32
		TextOut += STREAMTOSTRING(std::put_time(std::localtime(&Now), CONSOLE_TIMESTAMP));
#else
		// gcc didn't implement put_time yet
		char TimeBuf[128];
		const auto tm_now = localtime(&Now);
		strftime(TimeBuf, 127, CONSOLE_TIMESTAMP, tm_now);

		TextOut += TimeBuf;
#endif
	}


	if(Flags & EType::Success)
	{
		TextOut += CONSOLE_GREEN "SUCCESS" CONSOLE_RESET;
	}
	else if(Flags & EType::SQL)
	{
		TextOut += CONSOLE_BOLD_BLUE "SQL" CONSOLE_RESET;
	}
	else if(Flags & EType::Threads)
	{
		TextOut += CONSOLE_BOLD_MAGENTA "THREADS" CONSOLE_RESET;
	}
	else if(Flags & EType::Info)
	{
		TextOut += CONSOLE_CYAN "INFO" CONSOLE_RESET;
	}
	else if(Flags & EType::Notice)
	{
		TextOut += CONSOLE_BOLD_WHITE "NOTICE" CONSOLE_RESET;
	}
	else if(Flags & EType::Warning)
	{
		TextOut += CONSOLE_BOLD_YELLOW "WARNING" CONSOLE_RESET;
	}
	else if(Flags & EType::Debug)
	{
		TextOut += CONSOLE_BOLD_CYAN "DEBUG" CONSOLE_RESET;
	}
	else if(Flags & EType::Error)
	{
		TextOut += CONSOLE_RED "ERROR" CONSOLE_RESET;
	}
	else if(Flags & EType::CriticalError)
	{
		TextOut += CONSOLE_RED "CRITICAL" CONSOLE_RESET;
	}
	else if(Flags & EType::Exception)
	{
		TextOut += CONSOLE_BOLD_RED "EXCEPT" CONSOLE_RESET;
	}
	else if(Flags & EType::Graphics)
	{
		TextOut += CONSOLE_BOLD_BLUE "GRAPHICS" CONSOLE_RESET;
	}

	if(!(Flags & EType::None))
	{
#ifdef __WIN32
		TextOut.resize(CONSOLE_PREFIX_LEN - 1, ' ');
#else
		TextOut += CONSOLE_FMT_BEGIN;
#endif

		// start with processing
		Write(TextOut.c_str(), Stream);
	}

	// Make sure there is a linebreak in the end. We don't want duplicates!
	if(Text.empty() || Text.back() != '\n')
	{
		Text += '\n';
	}

	// Reset after each message
	Text += CONSOLE_RESET "";

	Write(Text.c_str(), Stream);
}

void CLog::Write(const std::string& Text, EStream Stream)
{
	if(Stream == EStream::STDERR)
	{
		fwrite(Text.c_str(), sizeof(char), Text.length(), stderr);
	}
	else if(Stream == EStream::STDOUT)
	{
		fwrite(Text.c_str(), sizeof(char), Text.length(), stdout);
	}
	else
	{
		std::cerr << "Unknown stream specified.\n";
	}
}
