#pragma once

#include "Platform/GAPlatform.h"

#if IS_LINUX

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/sysctl.h>
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

			virtual std::string getConnectionType() override;

			virtual std::string getCpuModel() 			const override;
			virtual std::string getGpuModel() 			const override;
			virtual int 		getNumCpuCores() 		const override;
			virtual int64_t 	getTotalDeviceMemory() 	const override;

			virtual int64_t getAppMemoryUsage() const override;
			virtual int64_t getSysMemoryUsage() const override;

			virtual int64_t getBootTime() const override;

		private:

			static void signalHandler(int sig, siginfo_t* info, void* context);
	};
}

#endif