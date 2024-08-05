#pragma once

#include "Platform/GAPlatform.h"

#if IS_LINUX

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>

namespace gameanalytics
{
	class GAPlatformLinux :
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

			static void signalHandler(int sig, siginfo_t* info, void* context);
			static struct sigaction prevSigAction;
	};
}

#endif