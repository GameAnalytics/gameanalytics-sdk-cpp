#pragma once

#include <cinttypes>
#include <vector>

#if defined(__linux__) || defined(__unix__) || defined(__unix) || defined(unix)
    
    constexpr std::size_t MAX_PATH_LENGTH = 4096;

#elif defined(__MACH__) || defined(__APPLE__)
    
    constexpr std::size_t MAX_PATH_LENGTH = 1017;

#elif _WIN32
    
    constexpr std::size_t MAX_PATH_LENGTH = 261;

#endif