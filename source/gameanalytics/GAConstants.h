#pragma once

#include "GACommon.h"

namespace gameanalytics
{
	#if _WIN32
		#if !defined(MAX_PATH_LENGTH)
			constexpr int MAX_PATH_LENGTH = 261;
		#endif
	#endif

	#if defined(__linux__) || defined(__unix__) || defined(__unix) || defined(unix)
		#if !defined(MAX_PATH_LENGTH)
			constexpr int MAX_PATH_LENGTH = 4096;
		#endif
	#endif

	#if defined(__MACH__) || defined(__APPLE__)
		#if !defined(MAX_PATH_LENGTH)
			constexpr int MAX_PATH_LENGTH = 1017;
		#endif
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

	#if _WIN32
		#define _WIN32_DCOM
	#endif

	constexpr const char* GA_VERSION_STR = "cpp 4.0.0";

	constexpr uint32_t MAX_CUSTOM_FIELDS_COUNT				 = 50;
	constexpr uint32_t MAX_CUSTOM_FIELDS_KEY_LENGTH			 = 64;
	constexpr uint32_t MAX_CUSTOM_FIELDS_VALUE_STRING_LENGTH = 256;

	constexpr uint32_t UUID_STR_LENGTH		= 128;
	constexpr uint32_t TEXT_BUFFER_LENGTH	= 256;

	constexpr const char* UNKNOWN_VALUE = "unknown";

	constexpr uint32_t MAX_ERROR_TYPE_COUNT = 10u;
	constexpr size_t   MAX_ERROR_MSG_LEN	= 8192u;
}