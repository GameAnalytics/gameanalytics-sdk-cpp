#pragma once

#include "GAPlatform.h"

#if IS_WIN32

    #include "GAWin32.h"

    namespace gameanalytics
    {
        using GADevicePlatform = GAPlatformWin32;
    }

#elif IS_UWP

    #include "GAUwp.h"

    namespace gameanalytics
    {
        using GADevicePlatform = GAPlatformUwp;
    }

#elif IS_LINUX

    #include "GALinux.h"
        
    namespace gameanalytics
    {
        using GADevicePlatform = GAPlatformLinux;
    }

#elif IS_MAC

    #include "GAMacOS.h"
        
    namespace gameanalytics
    {
        using GADevicePlatform = GAPlatformMacOS;
    }

#else
    #error unsupported platform!
#endif

namespace gameanalytics
{
    inline std::unique_ptr<GAPlatform> MakePlatform()
    {
        return std::make_unique<GADevicePlatform>();
    }
}
