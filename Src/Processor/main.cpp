#include <PrecompiledHeader.h>

#include "Processor.h"
#include "SharedEnums.h"

#include <args.hxx>

using namespace std::chrono;

int main(s32 argc, char* argv[])
{
	srand(static_cast<unsigned int>(time(NULL)));
	curl_global_init(CURL_GLOBAL_ALL);

#ifdef __WIN32
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		Log(CLog::CriticalError, "Couldn't startup winsock.");
	}
#endif

	std::vector<std::string> arguments;
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		// macOS sometimes (seemingly sporadically) passes the
		// process serial number via a command line parameter.
		// We would like to ignore this.
		if (arg.find("-psn") != 0)
			arguments.emplace_back(argv[i]);
	}

	args::ArgumentParser parser{
		"Computes performance points (pp) for the rhythm game osu!",
		"",
	};

	args::HelpFlag helpFlag{
		parser,
		"help",
		"Display this help menu",
		{'h', "help"},
	};

	args::Flag recomputeFlag{
		parser,
		"recompute",
		"Forces recomputation of pp for all players. Useful if the underlying algorithm changed.",
		{'r', "recompute"},
	};

	args::ValueFlag<u32> modeFlag{
		parser,
		"mode",
		"The game mode to compute pp for.",
		{'m', "mode"},
	};

	// Parse command line arguments and react to parsing
	// errors using exceptions.
	try
	{
		parser.ParseArgs(arguments);
	}
	catch (args::Help)
	{
		std::cout << parser;
		return 0;
	}
	catch (args::ParseError e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return -1;
	}
	catch (args::ValidationError e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return -2;
	}

	try
	{
		EGamemode gamemode = EGamemode::Standard;
		if (modeFlag)
		{
			u32 modeId = args::get(modeFlag);
			if (modeId < EGamemode::AmountGamemodes)
				gamemode = (EGamemode)modeId;
			else
				throw CLoggedException(SRC_POS, StrFormat("Invalid gamemode ID {0} supplied.", modeId));
		}

		// Create game object
		CProcessor Processor{
			gamemode,
			recomputeFlag,
		};
	}
	catch (CLoggedException& e)
	{
		e.Log();
		return 1;
	}
	catch (CException& e)
	{
		e.Print();
		return 1;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Uncaught exception: " << e.what() << std::endl;
		return 1;
	}
}
