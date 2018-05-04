#include <Shared.h>

#include <iostream>
#include <iomanip>
#include <sstream>

#ifndef __WIN32
	#include <sys/ioctl.h>
#endif

using namespace std;

void Log(ELogType flags, const string& text)
{
	Logger::Instance()->Log(flags, move(text));
}

void LogProgress(u64 current, u64 total, chrono::milliseconds elapsedMs)
{
	Logger::Instance()->LogProgress(current, total, elapsedMs);
}

Logger::~Logger()
{
	Log(None, CONSOLE_RESET CONSOLE_SHOW_CURSOR);
}

unique_ptr<Logger>& Logger::Instance()
{
	static auto pLog = createLogger();
	return pLog;
}

void Logger::Log(ELogType flags, const string& text)
{
	_pActive->Send([this, flags, text]() { logText(flags, text); });
}

void Logger::LogProgress(u64 current, u64 total, chrono::milliseconds elapsedMs)
{
	f64 fraction = (f64)current / total;

	auto projectedMs = elapsedMs * (1 / fraction);
	string timeStr = StrFormat("{0}/{1}", toString(elapsedMs), toString(projectedMs));

	string totalStr = StrFormat("{0}", total);
	string unitsFmt = string{"{0w"} + to_string(totalStr.size() * 2 + 8) + "}";

	string units = StrFormat("{2w3ar}% ({0}/{1}) ", current, total, (s32)std::round(fraction * 100), timeStr);
	// Give units some space in case they're short enough
	units = StrFormat(unitsFmt, units);

	s32 usableWidth = max(0, consoleWidth() - 4 - (s32)units.size() - (s32)timeStr.size() - CONSOLE_PREFIX_LEN);

	s32 numFilledChars = (s32)round(usableWidth * fraction);

	string body(usableWidth, ' ');
	if (numFilledChars > 0) {
		for (s32 i = 0; i < numFilledChars - 1; ++i)
			body[i] = '=';
		if (numFilledChars < (s32)body.size()) {
			body[numFilledChars - 1] = '>';
		}
	}

	string message = StrFormat("[{0}] {1} {2}", body, units, timeStr);

	Log(Progress, message);
}

Logger::Logger()
{
	canHandleControlCharacters = enableControlCharacters();
	_pActive = Active::Create();
}

unique_ptr<Logger> Logger::createLogger()
{
	auto pLogger = unique_ptr<Logger>(new Logger());

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

void Logger::logText(ELogType flags, const string& text)
{
	EStream stream;
	if (flags & Error || flags & Critical || flags & SQL || flags & Except)
		stream = EStream::STDERR;
	else
		stream = EStream::STDOUT;

	string textOut;

	if (!(flags & None))
	{
		// Display time format
		const auto currentTime = chrono::system_clock::to_time_t(chrono::system_clock::now());

#ifdef __WIN32
#define STREAMTOSTRING(x) dynamic_cast<ostringstream &>((ostringstream{} << dec << x)).str()
		textOut += STREAMTOSTRING(put_time(localtime(&currentTime), CONSOLE_TIMESTAMP));
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

void Logger::write(const string& text, EStream stream)
{
	if (stream == EStream::STDERR)
		cerr << text << flush;
	else if (stream == EStream::STDOUT)
		cout << text << flush;
	else
		cerr << "Unknown stream specified.\n";
}
