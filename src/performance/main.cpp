#include <pp/Common.h>
#include <pp/performance/Processor.h>

#include <args.hxx>

PP_NAMESPACE_BEGIN

EGamemode StringToGamemode(const std::string& modeString)
{
	EGamemode mode;
	if (modeString == "osu")
		mode = EGamemode::Osu;
	else if (modeString == "taiko")
		mode = EGamemode::Taiko;
	else if (modeString == "catch")
		mode = EGamemode::Catch;
	else if (modeString == "mania")
		mode = EGamemode::Mania;
	else
		throw LoggedException(SRC_POS, StrFormat("Invalid mode '{0}'", modeString));

	return mode;
}

int main(s32 argc, char* argv[])
{
	try
	{
		srand(static_cast<unsigned int>(time(NULL)));
		curl_global_init(CURL_GLOBAL_ALL);

#ifdef _WIN32
		WORD wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;
		if (WSAStartup(wVersionRequested, &wsaData) != 0)
		{
			tlog::error() << "Could not startup winsock.";
			return 2;
		}
#endif

		args::ArgumentParser parser{
			"Computes performance points (pp) for the rhythm game osu!",
			"",
		};

		args::Group argumentsGroup{"OPTIONS"};

		args::ValueFlag<std::string> modePositional{
			argumentsGroup,
			"GAMEMODE",
			"The game mode to compute pp for.\nMust be one of 'osu', 'taiko', 'catch', and 'mania'.\nDefault: 'osu'",
			{'m', "mode"},
			"osu",
		};

		args::ValueFlag<std::string> configFlag{
			argumentsGroup,
			"CONFIG",
			"The configuration file to use.\nDefault: 'config.json'",
			{"config"},
			"config.json",
		};

		args::HelpFlag helpFlag{
			argumentsGroup,
			"HELP",
			"Display this help menu.",
			{'h', "help"},
		};

		args::Group commands(parser, "COMMAND");
		args::Command newCommand(commands, "new", "Continually poll for new scores and compute pp of these", [&](args::Subparser& parser)
		{
			parser.Parse();
			Processor processor{StringToGamemode(args::get(modePositional)), args::get(configFlag)};
			processor.MonitorNewScores();
		});

		args::Command allCommand(commands, "all", "Compute pp of all users", [&](args::Subparser& parser)
		{
			args::Flag continueFlag{
				parser,
				"CONTINUE",
				"Continue where a previously aborted 'all' run left off.",
				{'c', "continue"},
			};

			args::ValueFlag<u32> threadsFlag{
				parser,
				"THREADS",
				"Number of threads to use. Can be useful even if the processor itself has no "
				"parallelism due to additional connections to the database.\n"
				"Default: 1",
				{'t', "threads"},
				1,
			};

			parser.Parse();

			u32 numThreads = args::get(threadsFlag);

			Processor processor{StringToGamemode(args::get(modePositional)), args::get(configFlag)};
			processor.ProcessAllUsers(!continueFlag, numThreads);
		});

		args::Command usersCommand(commands, "users", "Compute pp of specific users", [&](args::Subparser& parser)
		{
			args::PositionalList<std::string> usersPositional{
				parser,
				"users",
				"Users to recompute pp for.",
			};

			parser.Parse();

			std::vector<std::string> userNames = args::get(usersPositional);
			Processor processor{StringToGamemode(args::get(modePositional)), args::get(configFlag)};
			processor.ProcessUsers(args::get(usersPositional));
		});

		args::GlobalOptions argumentsGlobal{parser, argumentsGroup};

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

		// Parse command line arguments and react to parsing
		// errors using exceptions.
		try
		{
			parser.Prog(arguments[0]);
			parser.ParseArgs(std::begin(arguments) + 1, std::end(arguments));
		}
		catch (args::Help)
		{
			std::cout << parser;
			return 0;
		}
		catch (args::ParseError e)
		{
			std::cerr << e.what() << std::endl;
			return -1;
		}
		catch (args::ValidationError e)
		{
			std::cerr << e.what() << std::endl;
			return -2;
		}

		auto modeString = arguments[1];
		auto targetString = arguments[2];
	}
	catch (const Exception& e)
	{
		e.Log();
		return 1;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Uncaught exception: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

PP_NAMESPACE_END

int main(s32 argc, char* argv[])
{
	return pp::main(argc, argv);
}
