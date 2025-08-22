#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(GA_UWP_BUILD)

    #ifndef GA_UWP_BUILD
        #define IS_WIN32 1
        #define IS_UWP 0
    #else
        #define IS_WIN32 0
        #define IS_UWP 1
    #endif

    #define _WIN32_DCOM

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

#else
    #define IS_WIN32 0
    #define IS_UWP   0
#endif

#if IS_MAC
    #include <sys/sysctl.h>
#elif IS_WIN32
    #include <winsock2.h>
    #include <windows.h>
#elif IS_UWP
    #include <winsock2.h>
    #include <windows.h>
#elif IS_LINUX
    #include <sys/utsname.h>
    #include <sys/types.h>
    #include <sys/stat.h>
#endif

#include "GameAnalytics/GATypes.h"

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

namespace gameanalytics
{
    using nlohmann::json;

    namespace state
    {
        class GAState;
    }

    constexpr const char* GA_VERSION_STR = "cpp 5.0.0";

    constexpr int MAX_CUSTOM_FIELDS_COUNT				 = 50;
    constexpr int MAX_CUSTOM_FIELDS_KEY_LENGTH			 = 64;
    constexpr int MAX_CUSTOM_FIELDS_VALUE_STRING_LENGTH  = 256;

    constexpr int UUID_STR_LENGTH		= 128;
    constexpr int TEXT_BUFFER_LENGTH	= 256;

    constexpr const char* UNKNOWN_VALUE = "unknown";

    constexpr int MAX_ERROR_TYPE_COUNT = 10;
    constexpr int MAX_ERROR_MSG_LEN	= 8192;

    constexpr int JSON_PRINT_INDENT = 4;

    constexpr const char* CONNECTION_OFFLINE = "offline";
    constexpr const char* CONNECTION_LAN = "lan";
    constexpr const char* CONNECTION_WIFI = "wifi";
    constexpr const char* CONNECTION_WWAN = "wwan";
}
