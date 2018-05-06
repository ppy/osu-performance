#include <Shared.h>

#include <iostream>
#include <iomanip>
#include <sstream>

#ifndef __WIN32
	#include <sys/ioctl.h>
#endif

Logger::~Logger()
{
	Log(None, CONSOLE_RESET CONSOLE_SHOW_CURSOR);
}

std::unique_ptr<Logger>& Logger::Instance()
{
	static auto pLog = createLogger();
	return pLog;
}

void Logger::Log(ELogType flags, const std::string& text)
{
	if (flags & hiddenTypes)
		return;

	_pActive->Send([this, flags, text]() { logText(flags, text); });
}

Logger::Logger()
{
#ifdef NDEBUG
	hideType(Debug);
	hideType(Threads);
#endif

	canHandleControlCharacters = enableControlCharacters();
	_pActive = Active::Create();
}

std::unique_ptr<Logger> Logger::createLogger()
{
	auto pLogger = std::unique_ptr<Logger>(new Logger());

	// Reset initially (also blank line)
	pLogger->Log(None, CONSOLE_RESET);

#ifdef __DEBUG
	pLogger->Log(Info, "Program runs in debug mode.");
#endif

	return pLogger;
}

s32 Logger::consoleWidth()
{
#ifdef __WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
	winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	return size.ws_col;
#endif
}

bool Logger::enableControlCharacters()
{
#ifdef __WIN32
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		return false;
	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
		return false;
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
		return false;
#endif

	return true;
}

void Logger::logText(ELogType flags, const std::string& text)
{
	EStream stream;
	if (flags & Error || flags & Critical || flags & SQL || flags & Except)
		stream = EStream::STDERR;
	else
		stream = EStream::STDOUT;

	std::string textOut;

	if (!(flags & None))
	{
		using namespace std::chrono;

		// Display time format
		const auto currentTime = system_clock::to_time_t(system_clock::now());

#ifdef __WIN32
#define STREAMTOSTRING(x) dynamic_cast<std::ostringstream &>((std::ostringstream{} << std::dec << x)).str()
		textOut += STREAMTOSTRING(std::put_time(localtime(&currentTime), CONSOLE_TIMESTAMP));
#undef STREAMTOSTRING
#else
		char timeBuf[128];
		const auto tmCurrentTime = localtime(&currentTime);
		strftime(timeBuf, 127, CONSOLE_TIMESTAMP, tmCurrentTime);

		textOut += timeBuf;
#endif
	}

	if (flags & Success)
		textOut += CONSOLE_GREEN "SUCCESS" CONSOLE_RESET;
	else if (flags & SQL)
		textOut += CONSOLE_BOLD_BLUE "SQL" CONSOLE_RESET;
	else if (flags & Threads)
		textOut += CONSOLE_BOLD_MAGENTA "THREADS" CONSOLE_RESET;
	else if (flags & Info)
		textOut += CONSOLE_CYAN "INFO" CONSOLE_RESET;
	else if (flags & Notice)
		textOut += CONSOLE_BOLD_WHITE "NOTICE" CONSOLE_RESET;
	else if (flags & Warning)
		textOut += CONSOLE_BOLD_YELLOW "WARNING" CONSOLE_RESET;
	else if (flags & Debug)
		textOut += CONSOLE_BOLD_CYAN "DEBUG" CONSOLE_RESET;
	else if (flags & Error)
		textOut += CONSOLE_RED "ERROR" CONSOLE_RESET;
	else if (flags & Critical)
		textOut += CONSOLE_RED "CRITICAL" CONSOLE_RESET;
	else if (flags & Except)
		textOut += CONSOLE_BOLD_RED "EXCEPT" CONSOLE_RESET;
	else if (flags & Graphics)
		textOut += CONSOLE_BOLD_BLUE "GRAPHICS" CONSOLE_RESET;
	else if (flags & Progress)
		textOut += CONSOLE_CYAN "PROGRESS" CONSOLE_RESET;

	if (!(flags & None))
		textOut.resize(CONSOLE_PREFIX_LEN + 13, ' ');

	// Reset after each message
	textOut += text + CONSOLE_ERASE_TO_END_OF_LINE CONSOLE_RESET;

	// Make sure there is a linebreak in the end. We don't want duplicates!
	if (!(flags & Progress))
	{
		if (textOut.empty() || textOut.back() != '\n')
			textOut += '\n';

		textOut += CONSOLE_SHOW_CURSOR;
	}
	else
		textOut += CONSOLE_LINE_BEGIN CONSOLE_HIDE_CURSOR;

	write(textOut, stream);
}

void Logger::write(const std::string& text, EStream stream)
{
	if (stream == EStream::STDERR)
		std::cerr << text << std::flush;
	else if (stream == EStream::STDOUT)
		std::cout << text << std::flush;
	else
		std::cerr << "Unknown stream specified.\n";
}
