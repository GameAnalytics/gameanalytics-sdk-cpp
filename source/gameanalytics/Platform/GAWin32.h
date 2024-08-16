#pragma once

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NO_MIN_MAX
	#define NO_MIN_MAX
#endif

#include "Platform/GAPlatform.h"

#if IS_WIN32

	#include <direct.h>
	#include <windows.h>
	#include <VersionHelpers.h>
	#include <cerrno>
	#include <tchar.h>

	#if !GA_SHARED_LIB
		#include <comdef.h>
		#include <wbemidl.h>
		#pragma comment(lib, "wbemuuid.lib")
	#endif

#if GA_SHARED_LIB && !USE_MINGW
	#pragma comment(lib, "Ws2_32.lib")
	#pragma comment(lib, "gdi32.lib")
	#pragma comment(lib, "crypt32.lib")
#endif

namespace gameanalytics
{
	class GAPlatformWin32 :
		public GAPlatform
	{
		public:

			std::string getOSVersion()			override;
			std::string getDeviceManufacturer() override;
			std::string getBuildPlatform()		override;
			std::string getPersistentPath()		override;
			std::string getDeviceModel()		override;

			void setupUncaughtExceptionHandler() override;

		private:

			static void signalHandler(int sig);

			static void (*old_state_ill)	(int);
			static void (*old_state_abrt)	(int);
			static void (*old_state_fpe)	(int);
			static void (*old_state_segv)	(int);
	};
}
#endif