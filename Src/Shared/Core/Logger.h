#pragma once

#include "Active.h"

#define CONSOLE_TIMESTAMP "%H:%M:%S "
#define CONSOLE_PREFIX_LEN 19

#define CONSOLE_FMT_ESCAPE "\033"

#define CONSOLE_RESET       CONSOLE_FMT_ESCAPE"[0;37m"
#define CONSOLE_FMT_BEGIN   CONSOLE_FMT_ESCAPE"[" TOSTRING(CONSOLE_PREFIX_LEN) "G"

#define CONSOLE_PREV_LINE   CONSOLE_FMT_ESCAPE"[0G" CONSOLE_FMT_ESCAPE"[1A"

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

	void LogProgress(u64 current, u64 total);

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

	void logText(ELogType flags, std::string text);

	void write(const std::string& text, EStream Stream);

	std::unique_ptr<Active> _pActive;

	bool canHandleControlCharacters;
};

void Log(ELogType flags, const std::string& text);
void LogProgress(u64 current, u64 total);
