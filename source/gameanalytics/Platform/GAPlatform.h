#pragma once

#include "GACommon.h"
#include "GAConstants.h"

namespace gameanalytics
{
	class GAPlatform
	{
		public:

			virtual std::string getOSVersion()			 = 0;
			virtual std::string getDeviceManufacturer()  = 0;
			virtual std::string getBuildPlatform()		 = 0;
			virtual std::string getPersistentPath()		 = 0;
			virtual std::string getDeviceModel()		 = 0;

			virtual std::string getAdvertisingId();
			virtual std::string getDeviceId();

			virtual void setupUncaughtExceptionHandler();

			virtual std::string getConnectionType() = 0;

			virtual std::string getCpuModel() 			const {return "";};
			virtual std::string getGpuModel() 			const {return "";}
			virtual int 		getNumCpuCores() 		const {return 0;}
			virtual int64_t 	getTotalDeviceMemory() 	const {return 0;}

			virtual int64_t getAppMemoryUsage() const {return 0;}
			virtual int64_t getSysMemoryUsage() const {return 0;}
			
			void onInit();

			private:

                static std::terminate_handler previousTerminateHandler;
				static void terminateHandler();

	};
}
