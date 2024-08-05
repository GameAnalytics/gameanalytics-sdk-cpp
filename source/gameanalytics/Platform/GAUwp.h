#pragma once

#ifdef USE_UWP

#include <Windows.h>
#include <sstream>

namespace gameanalytics
{
	class GAPlatformUWP :
		public GAPlatform
	{
		public:

			std::string getOSVersion()			override;
			std::string getDeviceManufacturer() override;
			std::string getBuildPlatform()		override;
			std::string getPersistentPath()		override;
			std::string getDeviceModel()		override;

			std::string getAdvertisingId()	override;
			std::string getDeviceId()		override;
	};
}

#endif
