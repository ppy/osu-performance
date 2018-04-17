#include <PrecompiledHeader.h>

#include "Processor.h"
#include "SharedEnums.h"

using namespace std::chrono;
using namespace SharedEnums;

void WrapperProcess(std::string executableName, EGamemode gamemode, bool ReProcess)
{
#ifdef __WIN32
	auto ExecutionCommand = StrFormat("{0} -f -m {1}", executableName, gamemode);
#else
	auto ExecutionCommand = StrFormat("./{0} -f -m {1}", executableName, gamemode);
#endif

	if(ReProcess)
		ExecutionCommand += " -r";

	while(true)
	{
		static const int RestartDelay = 5;
		int ErrorCode = system(ExecutionCommand.c_str());
		Log(ErrorCode != 0 ? CLog::Error : CLog::Info, StrFormat("Program terminated with error code {0}. Restarting in {1} seconds.", ErrorCode, RestartDelay));
		std::this_thread::sleep_for(seconds{RestartDelay});
	}
}

int main(s32 argc, char* argv[])
{
	srand(static_cast<unsigned int>(time(NULL)));
	curl_global_init(CURL_GLOBAL_ALL);

#ifdef __WIN32
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if(WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		Log(CLog::CriticalError, "Couldn't startup winsock.");
	}
#endif

	bool Force = false;
	bool ReProcess = false;
	EGamemode Gamemode = EGamemode::Standard;

	// Process arguments. Can ignore first one which is the executable name
	for(s32 i = 1; i < argc; ++i)
	{
		if(std::string{"-f"} == argv[i])
			Force = true;

		if(std::string{"-r"} == argv[i])
			ReProcess = true;

		if(std::string{"-m"} == argv[i])
		{
			++i;

			// An errornous input... better report
			if(i >= argc)
			{
				Log(CLog::Error, "Missing mode specifier after \"-m\".");
				return 0;
			}

			Gamemode = static_cast<EGamemode>(std::atoi(argv[i]));
		}
	}

	// If there is no force parameter, then we are a wrapper process, ensuring, that there will always be an actual process running
	// This is no excuse for poor exception handling - but a worst-case measure to prevent downtime.
	// TODO: Proper logging of crashes, including core dumps
	if(!Force)
	{
		WrapperProcess(argv[0], Gamemode, ReProcess);
		return 0;
	}

	try
	{
		// Create game object
		CProcessor Processor{Gamemode, ReProcess};
	}
	catch(CLoggedException& e)
	{
		e.Log();
		return 1;
	}
	catch(CException& e)
	{
		e.Print();
		return 1;
	}
	catch(const std::exception& e)
	{
		std::cerr << "Uncaught exception: " << e.what() << std::endl;
		return 1;
	}
}
