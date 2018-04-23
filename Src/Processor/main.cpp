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

	args::Positional<std::string> targetPositional{
		parser,
		"target",
		"The target osu-performance is meant to compute pp for. Can be one of\n"
		"  new      - monitor and process new scores\n"
		"  all      - recompute existing scores\n"
		"  continue - continue previous 'all'\n"
		"  users    - recompute specific users",
		"new",
		args::Options::Required,
	};

	args::PositionalList<std::string> usersPositional{
		parser,
		"users",
		"Users to recompute pp for if the target 'users' has been chosen.",
	};

	args::HelpFlag helpFlag{
		parser,
		"help",
		"Display this help menu",
		{'h', "help"},
	};

	args::ValueFlag<u32> modeFlag{
		parser,
		"mode",
		"The game mode to compute pp for.",
		{'m', "mode"},
	};

	args::Flag recomputeFlag{
		parser,
		"recompute",
		"Forces recomputation of pp for all players. Useful if the underlying algorithm changed.",
		{'r', "recompute"},
	};

	// Parse command line arguments and react to parsing
	// errors using exceptions.
	try
	{
		parser.Prog(argv[0]);
		parser.ParseArgs(arguments);
	}
	catch (args::Help)
	{
		std::cout << parser;
		return 0;
	}
	catch (args::ParseError e)
	{
		std::cerr << e.what() << std::endl << std::endl;
		std::cerr << parser;
		return -1;
	}
	catch (args::ValidationError e)
	{
		std::cerr << e.what() << std::endl << std::endl;
		std::cerr << parser;
		return -2;
	}

	try
	{
		EGamemode gamemode = EGamemode::Standard;
		if (modeFlag)
		{
			u32 modeId = args::get(modeFlag);
			if (modeId < NumGamemodes)
				gamemode = (EGamemode)modeId;
			else
				throw CLoggedException(SRC_POS, StrFormat("Invalid gamemode ID {0} supplied.", modeId));
		}

		// Fail early if an invalid target was supplied
		auto target = args::get(targetPositional);
		if (target != "new" && target != "all" && target != "continue" && target != "users")
			throw CLoggedException(SRC_POS, StrFormat("Invalid target '{0}' supplied.", target));

		CProcessor processor{gamemode};
		if (target == "new")
			processor.MonitorNewScores();
		else if (target == "all")
			processor.ProcessAllScores(true);
		else if (target == "continue")
			processor.ProcessAllScores(false);
		else if (target == "users")
			throw CLoggedException(SRC_POS, "Not yet implemented");
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
