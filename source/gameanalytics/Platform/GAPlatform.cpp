#include "GAPlatform.h"
#include "GAState.h"
#include "GAEvents.h"
#include <stacktrace/call_stack.hpp>

std::terminate_handler gameanalytics::GAPlatform::previousTerminateHandler;

std::string gameanalytics::GAPlatform::getAdvertisingId()
{
	return "";
}

std::string gameanalytics::GAPlatform::getDeviceId()
{
	return "";
}

void gameanalytics::GAPlatform::setupUncaughtExceptionHandler()
{
	return;
}

/* terminateHandler
* C++ exception terminate handler
*/
void gameanalytics::GAPlatform::terminateHandler()
{
	constexpr int MAX_ERROR_TYPE_COUNT = 5;
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
        
        if(previousTerminateHandler)
        {
            previousTerminateHandler();
        }
    }
}

void gameanalytics::GAPlatform::onInit()
{
	if(state::GAState::useErrorReporting())
	{
        previousTerminateHandler = std::set_terminate(terminateHandler);
    }
}
