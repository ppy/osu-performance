#pragma once

#include "Active.h"

#define CONSOLE_TIMESTAMP "%H:%M:%S "
#define CONSOLE_PREFIX_LEN 19

#define CONSOLE_FMT_ESCAPE "\033"

#define CONSOLE_RESET       CONSOLE_FMT_ESCAPE"[0;37m"
#define CONSOLE_FMT_BEGIN   CONSOLE_FMT_ESCAPE"[" TOSTRING(CONSOLE_PREFIX_LEN) "G"

#define CONSOLE_LINE_BEGIN  CONSOLE_FMT_ESCAPE"[0G"

#define CONSOLE_ERASE_TO_END_OF_LINE CONSOLE_FMT_ESCAPE"[K"

#define CONSOLE_BLACK     CONSOLE_FMT_ESCAPE"[0;30m"
#define CONSOLE_RED       CONSOLE_FMT_ESCAPE"[0;31m"
#define CONSOLE_GREEN     CONSOLE_FMT_ESCAPE"[0;32m"
#define CONSOLE_YELLOW    CONSOLE_FMT_ESCAPE"[0;33m"
#define CONSOLE_BLUE      CONSOLE_FMT_ESCAPE"[0;34m"
#define CONSOLE_MAGENTA   CONSOLE_FMT_ESCAPE"[0;35m"
#define CONSOLE_CYAN      CONSOLE_FMT_ESCAPE"[0;36m"
#define CONSOLE_WHITE     CONSOLE_FMT_ESCAPE"[0;37m"

#define CONSOLE_BOLD_BLACK     CONSOLE_FMT_ESCAPE"[1;30m"
#define CONSOLE_BOLD_RED       CONSOLE_FMT_ESCAPE"[1;31m"
#define CONSOLE_BOLD_GREEN     CONSOLE_FMT_ESCAPE"[1;32m"
#define CONSOLE_BOLD_YELLOW    CONSOLE_FMT_ESCAPE"[1;33m"
#define CONSOLE_BOLD_BLUE      CONSOLE_FMT_ESCAPE"[1;34m"
#define CONSOLE_BOLD_MAGENTA   CONSOLE_FMT_ESCAPE"[1;35m"
#define CONSOLE_BOLD_CYAN      CONSOLE_FMT_ESCAPE"[1;36m"
#define CONSOLE_BOLD_WHITE     CONSOLE_FMT_ESCAPE"[1;37m"

#define CONSOLE_HIDE_CURSOR    CONSOLE_FMT_ESCAPE"[?25l"
#define CONSOLE_SHOW_CURSOR    CONSOLE_FMT_ESCAPE"[?25h"

DEFINE_EXCEPTION(LoggerException);

enum ELogType : u32
{
	None          = 0x00000001,
	Success       = 0x00000002,
	SQL           = 0x00000004,
	Threads       = 0x00000008,
	Info          = 0x00000010,
	Notice        = 0x00000020,
	Warning       = 0x00000040,
	Debug         = 0x00000080,
	Error         = 0x00000100,
	Critical      = 0x00000200,
	Except        = 0x00000400,
	Graphics      = 0x00000800,
	Progress      = 0x00001000,
};

class Logger
{
public:
	~Logger();

	static std::unique_ptr<Logger>& Instance();

	void Log(ELogType flags, const std::string& text);

	template <typename T>
	void LogProgress(u64 current, u64 total, T duration)
	{
		using namespace std;

		f64 fraction = (f64)current / total;

		// Percentage display. Looks like so:
		//  69%
		s32 percentage = (s32)std::round(fraction * 100);
		string percentageFmt = "{0w3ar}%";

		// Fraction display. Looks like so:
		// ( 123/1337)
		string totalStr = StrFormat("{0}", total);
		string fractionStr = StrFormat("{0}/{1}", current, total);
		string fractionFmt = string{"{1w"} + to_string(totalStr.size() * 2 + 1) + "ar}";

		// Time display. Looks like so:
		//     3s/17m03s
		auto projectedDuration = duration * (1 / fraction);
		auto projectedDurationStr = durationToString(projectedDuration);
		string timeStr = StrFormat("{0}/{1}", durationToString(duration), projectedDurationStr);
		string timeFmt = string{"{2w"} + to_string(projectedDurationStr.size() * 2 + 1) + "ar}";

		// Put the label together. Looks like so:
		//  69% ( 123/1337)     3s/17m03s
		string fmt = StrFormat("{0} ({1}) {2}", percentageFmt, fractionFmt, timeFmt);
		string label = StrFormat(fmt, percentage, fractionStr, timeStr);

		// Build the progress bar itself. Looks like so:
		// [=================>                         ]
		s32 usableWidth = max(0, consoleWidth()
			- 2 // The surrounding [ and ]
			- 1 // Space between progress bar and label
			- (s32)label.size() // Label itself
			- CONSOLE_PREFIX_LEN // Prefix
		);

		s32 numFilledChars = (s32)round(usableWidth * fraction);

		string body(usableWidth, ' ');
		if (numFilledChars > 0) {
			for (s32 i = 0; i < numFilledChars; ++i)
				body[i] = '=';
			if (numFilledChars < usableWidth) {
				body[numFilledChars] = '>';
			}
		}

		// Put everything together. Looks like so:
		// [=================>                         ]  69% ( 123/1337)     3s/17m03s
		string message = StrFormat("[{0}] {1} {2}", body, label);

		Log(Progress, message);
	}

	enum class EStream : byte
	{
		STDOUT=0,
		STDERR,
	};

private:
	Logger();

	static std::unique_ptr<Logger> createLogger();

	static s32 consoleWidth();

	bool enableControlCharacters();

	void logText(ELogType flags, const std::string& text);

	void write(const std::string& text, EStream Stream);

	std::unique_ptr<Active> _pActive;

	bool canHandleControlCharacters;
};

inline void Log(ELogType flags, const std::string& text)
{
	Logger::Instance()->Log(flags, std::move(text));
}

template <typename T>
void LogProgress(u64 current, u64 total, T duration)
{
	Logger::Instance()->LogProgress(current, total, duration);
}
