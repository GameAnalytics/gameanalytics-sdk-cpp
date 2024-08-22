#pragma once

#include "Platform/GAPlatform.h"

#if IS_MAC

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

namespace gameanalytics
{
	class GAPlatformMacOS :
		public GAPlatform
	{
		public:

			std::string getOSVersion()			override;
			std::string getDeviceManufacturer() override;
			std::string getBuildPlatform()		override;
			std::string getPersistentPath()		override;
			std::string getDeviceModel()		override;

			virtual std::string getCpuModel() 			const override;
			virtual std::string getGpuModel() 			const override;
			virtual int 		getNumCpuCores() 		const override;
			virtual int64_t 	getTotalDeviceMemory() 	const override;

			virtual int64_t getAppMemoryUsage() const override;
			virtual int64_t getSysMemoryUsage() const override;

			void setupUncaughtExceptionHandler() override;

			std::string getConnectionType() override;

		private:

			static void signalHandler(int sig, siginfo_t* info, void* context);
	};
}

#endif
