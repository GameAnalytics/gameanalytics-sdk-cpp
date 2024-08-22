//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//
#include "GAUncaughtExceptionHandler.h"
#include "GAState.h"
#include "GAEvents.h"

#include <stacktrace/call_stack.hpp>

#include <string.h>
#include <stdio.h>
#include <cstdarg>
#include <execinfo.h>
#include <cstring>
#include <cstdlib>

namespace gameanalytics
{
    namespace errorreporter
    {
        std::terminate_handler GAUncaughtExceptionHandler::previousTerminateHandler = NULL;

        /* terminateHandler
         * C++ exception terminate handler
         */
        void GAUncaughtExceptionHandler::terminateHandler()
        {
            static int errorCount = 0;

            if(state::GAState::useErrorReporting())
            {
                /*
                 *    Now format into a message for sending to the user
                 */

                if(errorCount <= MAX_ERROR_TYPE_COUNT)
                {
                    stacktrace::call_stack st;
                    size_t totalSize = st.to_string_size() + 1;
                   
                    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(totalSize);
                    
                    if(!buffer)
                        return;

                    st.to_string(buffer.get());

                    std::string stackTrace = "Uncaught C++ Exception\nStack trace:\n";

                    stackTrace += std::string(buffer.get(), totalSize);
                    stackTrace += '\n';

                    ++errorCount;

                    events::GAEvents::addErrorEvent(EGAErrorSeverity::Critical, stackTrace, "", -1, {}, false, false);
                    events::GAEvents::processEvents("error", false);
                }
            }

            if(previousTerminateHandler)
            {
                previousTerminateHandler();
            }
        }

        void GAUncaughtExceptionHandler::setUncaughtExceptionHandlers()
        {
            if(state::GAState::useErrorReporting())
            {
                setupUncaughtSignals();
                previousTerminateHandler = std::set_terminate(terminateHandler);
            }
        }
    }
}
