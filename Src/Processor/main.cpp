#include <PrecompiledHeader.h>

#include "Processor.h"
#include "SharedEnums.h"

#include <args.hxx>

using namespace std::chrono;

void printUsage(std::string prog)
{
	std::cout
		<< "Usage:" << std::endl
		<< "  " << prog << " MODE TARGET" << std::endl
		<< std::endl
		<< "    MODE     The game mode to compute pp for." << std::endl
		<< "             Must be one of 'osu', 'taiko', 'catch', 'mania'." << std::endl
		<< "    TARGET   The target pp should be computed for." << std::endl
		<< "             Must be one of 'new', 'all', 'users'." << std::endl
		<< std::endl
		<< "For help about a specific target use:" << std::endl
		<< "  " << prog << " MODE TARGET -h" << std::endl
		<< std::endl;
		;
}

bool parse(args::ArgumentParser& parser, std::string prog, const std::vector<std::string>& arguments) {
	// Parse command line arguments and react to parsing
	// errors using exceptions.
	try
	{
		parser.Prog(prog);
		parser.ParseArgs(arguments);
	}
	catch (args::Help)
	{
		std::cout << parser;
		return false;
	}
	catch (args::ParseError e)
	{
		std::cerr << e.what() << std::endl << std::endl;
		std::cerr << parser;
		return false;
	}
	catch (args::ValidationError e)
	{
		std::cerr << e.what() << std::endl << std::endl;
		std::cerr << parser;
		return false;
	}

	return true;
}

void mainNew(std::string prog, const std::vector<std::string>& arguments, EGamemode mode)
{
	args::ArgumentParser parser{
		"Computes performance points (pp) for the rhythm game osu! for newly arriving scores",
		"",
	};

	args::ValueFlag<std::string> configFlag{
		parser,
		"config",
		"The configuration file to use.\nDefault: 'Config.cfg'",
		{"config"},
		"Config.cfg",
	};

	args::HelpFlag helpFlag{
		parser,
		"help",
		"Display this help menu",
		{'h', "help"},
	};

	if (!parse(parser, prog, arguments))
		return;

	CProcessor processor{mode, args::get(configFlag)};
	processor.MonitorNewScores();
}

void mainAll(std::string prog, const std::vector<std::string>& arguments, EGamemode mode)
{
	args::ArgumentParser parser{
		"Computes performance points (pp) for the rhythm game osu! for all users",
		"",
	};

	args::ValueFlag<std::string> configFlag{
		parser,
		"config",
		"The configuration file to use.\nDefault: 'Config.cfg'",
		{"config"},
		"Config.cfg",
	};

	args::Flag continueFlag{
		parser,
		"continue",
		"Continue where a previously aborted 'all' run left off.",
		{'c', "continue"},
	};

	args::HelpFlag helpFlag{
		parser,
		"help",
		"Display this help menu",
		{'h', "help"},
	};

	args::ValueFlag<u32> threadsFlag{
		parser,
		"threads",
		"Number of threads to use. Can be useful even if the processor itself has no "
		"parallelism due to additional connections to the database.\n"
		"Default: 1",
		{'t', "threads"},
		1,
	};

	if (!parse(parser, prog, arguments))
		return;

	u32 numThreads = args::get(threadsFlag);

	CProcessor processor{mode, args::get(configFlag)};
	processor.ProcessAllScores(!continueFlag, numThreads);
}

void mainUsers(std::string prog, const std::vector<std::string>& arguments, EGamemode mode)
{
	args::ArgumentParser parser{
		"Computes performance points (pp) for the rhythm game osu! for specific users",
		"",
	};

	args::ValueFlag<std::string> configFlag{
		parser,
		"config",
		"The configuration file to use.\nDefault: 'Config.cfg'",
		{"config"},
		"Config.cfg",
	};

	args::PositionalList<std::string> usersPositional{
		parser,
		"users",
		"Users to recompute pp for.",
	};

	if (!parse(parser, prog, arguments))
		return;

	throw CLoggedException(SRC_POS, "Not yet implemented");
}

int main(s32 argc, char* argv[])
{
	try
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
		for (int i = 0; i < argc; ++i)
		{
			std::string arg = argv[i];
			// macOS sometimes (seemingly sporadically) passes the
			// process serial number via a command line parameter.
			// We would like to ignore this.
			if (arg.find("-psn") != 0)
				arguments.emplace_back(argv[i]);
		}

		if (arguments.empty())
			return -1;

		if (arguments.size() < 3)
		{
			printUsage(arguments[0]);
			return -1;
		}

		auto modeString = arguments[1];
		auto targetString = arguments[2];

		EGamemode mode;
		if (modeString == "osu")
			mode = Standard;
		else if (modeString == "taiko")
			mode = Taiko;
		else if (modeString == "catch")
			mode = CatchTheBeat;
		else if (modeString == "mania")
			mode = Mania;
		else
			throw CLoggedException(SRC_POS, StrFormat("Invalid mode '{0}'", modeString));

		std::string prog = arguments[0] + " " + arguments[1] + " " + arguments[2];
		arguments.erase(std::begin(arguments), std::begin(arguments) + 3);
		if (targetString == "new")
			mainNew(prog, arguments, mode);
		else if (targetString == "all")
			mainAll(prog, arguments, mode);
		else if (targetString == "users")
			mainUsers(prog, arguments, mode);
		else
			throw CLoggedException(SRC_POS, StrFormat("Invalid target '{0}'", targetString));
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
