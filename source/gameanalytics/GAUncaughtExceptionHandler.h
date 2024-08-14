//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#if !USE_UWP && !USE_TIZEN
#include <exception>
#include <signal.h>

namespace gameanalytics
{
    namespace errorreporter
    {
        class GAUncaughtExceptionHandler
        {
            public:

                static void setUncaughtExceptionHandlers();
        
            private:

                static constexpr int MAX_ERROR_TYPE_COUNT = 5;

                static void setupUncaughtSignals();
                static void terminateHandler();

                static std::terminate_handler previousTerminateHandler;
        };
    }
}
#endif
