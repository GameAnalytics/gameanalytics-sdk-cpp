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

// todo include path
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

	template<typename ...args_t>
	static std::string print(std::string const& fmt, args_t&&... args)
	{
		constexpr int k_maxLogSize = 2048;
		char buffer[k_maxLogSize] = "";

		std::snprintf(buffer, k_maxLogSize, fmt.c_str(), std::forward<args_t>(args)...);

		return buffer;
	}
}
