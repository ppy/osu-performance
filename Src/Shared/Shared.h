#ifndef H_SHARED
#define H_SHARED

// May be unsafe for fopen, vsnprintf, localtime, ...
#define _CRT_SECURE_NO_WARNINGS

// Define consistent OS defines here
#ifdef _WIN32
	#define __WIN32
#elif defined(__APPLE__)
	#define __APPLE
#else
#	define __LINUX
#endif

// Apple and Linux both imply Unix
#if defined(__APPLE) || defined(__LINUX)
	#define __UNIX
#endif

// Some compiler-specific directives which might turn out useful.
// Wrapper macros to make code portable between compilers.
#ifdef __WIN32
	#define INLINE __forceinline
	#define UNREACHABLE __assume(false)
	#define ASSUME __assume
	#define THREAD_LOCAL __declspec(thread) // c++11 thread_local not yet supported by win32
#else
	#define INLINE inline
	#define UNREACHABLE __builtin_unreachable()
	#define ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
	#define THREAD_LOCAL thread_local
#endif

// Define a debug switch with the same consistency as the other defines
#ifdef _DEBUG
	#define __DEBUG
#endif

// Make a string out of a macro argument.
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifdef __WIN32
	#define NOMINMAX
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>

	// Networking. That is winsock for windows
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <WinSock2.h>
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/mman.h>

	// Required for networking under UNIX
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif

#include <cassert>

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <exception>

// Math
//#define _USE_MATH_DEFINES
#include <cmath>
#include <cfloat>
#include <climits>

#include <locale>

// Container
#include <array>
#include <list>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <vector>
#include <deque>
#include <string>

// Threading
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// Random other STD stuff
#include <memory>
#include <chrono>
#include <future>
#include <functional>
#include <stdexcept>
#include <ratio>
#include <random>

// Third party libraries

// MariaDB needs this to compile. Defining it here avoids modification of their
// header files and thus additional licensing work.
typedef unsigned int uint;

#include <mysql.h>

// Base types: int = s32, ...
#include "Core/Types.h"

#include <StrFormat.h>

#include "Core/Exception.h"

#include "Math/Math.h"

// The logger requires CVector2d<s32> for positions in the console, thus it comes
// after the math things
#include "Core/Logger.h"
#include "Core/Threading.h"

#endif //H_SHARED
