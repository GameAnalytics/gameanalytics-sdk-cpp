#pragma once

#include "GAConstants.h"

#include <string>
#include <vector>
#include <cinttypes>
#include <memory>
#include <thread>
#include <filesystem>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cctype>
#include <future>
#include <ostream>
#include <mutex>
#include <fstream>
#include <csignal>
#include <utility>
#include <cstring>
#include <cstdio>
#include <array>

#include "nlohmann/json.hpp"

#if IS_MAC
	#include <sys/sysctl.h>
#elif IS_LINUX
	#include <sys/utsname.h>
	#include <sys/types.h>
	#include <sys/stat.h>
#endif

namespace gameanalytics
{
	using nlohmann::json;
    using StringVector = std::vector<std::string>;

    using LogHandler = std::function<void(std::string const&, EGALoggerMessageType)>;
    using FPSTracker = std::function<float()>;

	namespace state
	{
		class GAState;
	}

	struct IRemoteConfigsListener
    {
        virtual void onRemoteConfigsUpdated(std::string const& remoteConfigs) = 0;
    };
}
