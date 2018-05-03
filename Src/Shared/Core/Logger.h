#pragma once

#include "Active.h"

#define CONSOLE_TIMESTAMP "%H:%M:%S "
#define CONSOLE_PREFIX_LEN 19

#ifdef __WIN32
	#define CONSOLE_FMT_ESCAPE

	#define CONSOLE_RESET

	#define CONSOLE_BLACK
	#define CONSOLE_RED
	#define CONSOLE_GREEN
	#define CONSOLE_YELLOW
	#define CONSOLE_BLUE
	#define CONSOLE_MAGENTA
	#define CONSOLE_CYAN
	#define CONSOLE_WHITE

	#define CONSOLE_BOLD_BLACK
	#define CONSOLE_BOLD_RED
	#define CONSOLE_BOLD_GREEN
	#define CONSOLE_BOLD_YELLOW
	#define CONSOLE_BOLD_BLUE
	#define CONSOLE_BOLD_MAGENTA
	#define CONSOLE_BOLD_CYAN
	#define CONSOLE_BOLD_WHITE
#else
	#define CONSOLE_FMT_ESCAPE "\033"

	#define CONSOLE_RESET       CONSOLE_FMT_ESCAPE"[0;37m"
	#define CONSOLE_FMT_BEGIN   CONSOLE_FMT_ESCAPE"[" TOSTRING(CONSOLE_PREFIX_LEN) "G"

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
#endif

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
};

class Logger
{
public:
	Logger();
	~Logger();

	static std::unique_ptr<Logger> CreateLogger();

	//void log(byte bColor, const char* pcSys, const char* pcFmt, ...);
	void Log(ELogType flags, std::string text);

	enum class EStream : byte
	{
		STDOUT=0,
		STDERR,
	};

private:
	std::unique_ptr<Active> _pActive;

	static const size_t s_outputBufferSize = 10000;

	char _outputBufferStdout[Logger::s_outputBufferSize];
	char _outputBufferStderr[Logger::s_outputBufferSize];

	void logText(ELogType flags, std::string text);

	void write(const std::string& text, EStream Stream);
};

void Log(ELogType flags, std::string text);
