#ifdef OS_WINDOWS
	#define WINHEAD (1)
#elif defined(_WIN64)
	#define WINHEAD (1)
#elif defined(_WIN32)
	#define WINHEAD (1)
#elif defined(_MSC_VER) // Microsoft compiler
	#define WINHEAD (1)
#else
	#define WINHEAD (0)
#endif
