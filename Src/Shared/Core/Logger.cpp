#include <Shared.h>

#include <iostream>
#include <iomanip>
#include <sstream>

void Log(CLog::EType flags, std::string text)
{
	static auto pLog = CLog::CreateLogger();
	pLog->Log(flags, std::move(text));
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
	_pActive = CActive::Create();
}

CLog::~CLog()
{
#ifndef __WIN32
	// Reset
	Log(EType::None, CONSOLE_RESET);
#endif
}

void CLog::Log(EType flags, std::string text)
{
	_pActive->Send([this, flags, text]() { logText(flags, std::move(text)); });
}

void CLog::logText(EType flags, std::string text)
{
	EStream stream;
	if (flags & EType::Error || flags & EType::CriticalError || flags & EType::SQL || flags & EType::Exception)
		stream = EStream::STDERR;
	else
		stream = EStream::STDOUT;

	std::string textOut;

	if (!(flags & EType::None))
	{
		// Display time format
		const auto currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

#ifdef __WIN32
		textOut += STREAMTOSTRING(std::put_time(std::localtime(&currentTime), CONSOLE_TIMESTAMP));
#else
		char timeBuf[128];
		const auto tmCurrentTime = localtime(&currentTime);
		strftime(timeBuf, 127, CONSOLE_TIMESTAMP, tmCurrentTime);

		textOut += timeBuf;
#endif
	}

	if (flags & EType::Success)
		textOut += CONSOLE_GREEN "SUCCESS" CONSOLE_RESET;
	else if (flags & EType::SQL)
		textOut += CONSOLE_BOLD_BLUE "SQL" CONSOLE_RESET;
	else if (flags & EType::Threads)
		textOut += CONSOLE_BOLD_MAGENTA "THREADS" CONSOLE_RESET;
	else if (flags & EType::Info)
		textOut += CONSOLE_CYAN "INFO" CONSOLE_RESET;
	else if (flags & EType::Notice)
		textOut += CONSOLE_BOLD_WHITE "NOTICE" CONSOLE_RESET;
	else if (flags & EType::Warning)
		textOut += CONSOLE_BOLD_YELLOW "WARNING" CONSOLE_RESET;
	else if (flags & EType::Debug)
		textOut += CONSOLE_BOLD_CYAN "DEBUG" CONSOLE_RESET;
	else if (flags & EType::Error)
		textOut += CONSOLE_RED "ERROR" CONSOLE_RESET;
	else if (flags & EType::CriticalError)
		textOut += CONSOLE_RED "CRITICAL" CONSOLE_RESET;
	else if (flags & EType::Exception)
		textOut += CONSOLE_BOLD_RED "EXCEPT" CONSOLE_RESET;
	else if (flags & EType::Graphics)
		textOut += CONSOLE_BOLD_BLUE "GRAPHICS" CONSOLE_RESET;

	if (!(flags & EType::None))
	{
#ifdef __WIN32
		textOut.resize(CONSOLE_PREFIX_LEN - 1, ' ');
#else
		textOut += CONSOLE_FMT_BEGIN;
#endif

		// start with processing
		write(textOut, stream);
	}

	// Make sure there is a linebreak in the end. We don't want duplicates!
	if (text.empty() || text.back() != '\n')
		text += '\n';

	// Reset after each message
	text += CONSOLE_RESET "";

	write(text, stream);
}

void CLog::write(const std::string& text, EStream stream)
{
	if (stream == EStream::STDERR)
		fwrite(text.c_str(), sizeof(char), text.length(), stderr);
	else if (stream == EStream::STDOUT)
		fwrite(text.c_str(), sizeof(char), text.length(), stdout);
	else
		std::cerr << "Unknown stream specified.\n";
}
