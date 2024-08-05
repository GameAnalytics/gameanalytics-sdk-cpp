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

	};
}
