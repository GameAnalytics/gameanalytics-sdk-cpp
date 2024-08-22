#pragma once

#if defined(_WIN32) || defined(_WIN64)
	#define IS_WIN32 1

	#define _WIN32_DCOM

	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif

		#ifndef NO_MIN_MAX
			#define NO_MIN_MAX
		#endif

#else
	#define IS_WIN32 0
#endif

#if defined(__linux__) || defined(__unix__) || defined(__unix) || defined(unix)
	#define IS_LINUX 1
#else
	#define IS_LINUX 0
#endif

#if defined(__MACH__) || defined(__APPLE__)
	#define IS_MAC 1
#else
	#define IS_MAC 0
#endif

#include <cinttypes>

namespace gameanalytics
{
	constexpr const char* GA_VERSION_STR = "cpp 4.0.0";

	constexpr uint32_t MAX_CUSTOM_FIELDS_COUNT				 = 50u;
	constexpr uint32_t MAX_CUSTOM_FIELDS_KEY_LENGTH			 = 64u;
	constexpr uint32_t MAX_CUSTOM_FIELDS_VALUE_STRING_LENGTH = 256u;

	constexpr uint32_t UUID_STR_LENGTH		= 128u;
	constexpr uint32_t TEXT_BUFFER_LENGTH	= 256u;

	constexpr const char* UNKNOWN_VALUE = "unknown";

	constexpr uint32_t MAX_ERROR_TYPE_COUNT = 10u;
	constexpr uint32_t MAX_ERROR_MSG_LEN	= 8192u;

	constexpr uint32_t JSON_PRINT_INDENT = 4u;
}
