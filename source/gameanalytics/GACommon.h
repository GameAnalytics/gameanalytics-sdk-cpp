#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NO_MIN_MAX
	#define NO_MIN_MAX
#endif

#endif

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

// todo include path
#include "../dependencies/nlohmann/json.hpp"

#if IS_MAC
	#include "GADeviceOSX.h"
	#include <sys/sysctl.h>
#elif IS_LINUX
	#include <sys/utsname.h>
	#include <sys/types.h>
	#include <sys/stat.h>
#endif

namespace gameanalytics
{
	using nlohmann::json;

	template<typename ...args_t>
	static std::string print(std::string const& fmt, args_t&&... args)
	{
		constexpr int k_maxLogSize = 2048;
		char buffer[k_maxLogSize] = "";

		std::snprintf(buffer, k_maxLogSize, fmt.c_str(), std::forward<args_t>(args)...);

		return buffer;
	}
}
