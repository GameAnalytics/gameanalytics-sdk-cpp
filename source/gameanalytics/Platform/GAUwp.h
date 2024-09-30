#pragma once

#include "GACommon.h"

#if IS_UWP

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
	#define NOMINMAX
#endif

// needed for CURL
#include <winsock2.h>

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

			virtual std::string getConnectionType() override;
	};
}

#endif
