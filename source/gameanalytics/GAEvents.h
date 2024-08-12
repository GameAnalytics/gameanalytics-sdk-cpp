//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include "GameAnalytics.h"

namespace gameanalytics
{
    namespace events
    {
        class GAEvents
        {
         public:

            static void stopEventQueue();
            static void ensureEventQueueIsRunning();
            static void addSessionStartEvent();
            static void addSessionEndEvent();
            static void addBusinessEvent(std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, const json& fields, bool mergeFields);
            static void addResourceEvent(EGAResourceFlowType flowType, std::string const& currency, double amount, std::string const& itemType, std::string const& itemId, const json& fields, bool mergeFields);
            static void addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, bool sendScore, const json& fields, bool mergeFields);
            static void addDesignEvent(std::string const& eventId, double value, bool sendValue, const json& fields, bool mergeFields);
            static void addErrorEvent(EGAErrorSeverity severity, std::string const& message, const json& fields, bool mergeFields);
            static void addErrorEvent(EGAErrorSeverity severity, std::string const& message, const json& fields, bool mergeFields, bool skipAddingFields);
            
            static std::string progressionStatusString(EGAProgressionStatus progressionStatus);
            static std::string errorSeverityString(EGAErrorSeverity errorSeverity);
            static std::string resourceFlowTypeString(EGAResourceFlowType flowType);

            static void processEvents(std::string const& category, bool performCleanUp);

        private:

            static constexpr const char* CategorySessionStart           = "user";
            static constexpr const char* CategorySessionEnd             = "session_end";
            static constexpr const char* CategoryDesign                 = "design";
            static constexpr const char* CategoryBusiness               = "business";
            static constexpr const char* CategoryProgression            = "progression";
            static constexpr const char* CategoryResource               = "resource";
            static constexpr const char* CategoryError                  = "error";
            static constexpr double      ProcessEventsIntervalInSeconds = 8.0;
            static constexpr int         MaxEventCount                  = 500;

            GAEvents();
            ~GAEvents();
            GAEvents(const GAEvents&) = delete;
            GAEvents& operator=(const GAEvents&) = delete;

            void processEventQueue();
            void cleanupEvents();
            void fixMissingSessionEndEvents();
            void addEventToStore(json &eventData);
            void addDimensionsToEvent(json& eventData);
            void addCustomFieldsToEvent(json& eventData, json& fields);
            void updateSessionTime();

            static bool _destroyed;
            static GAEvents* _instance;
            static std::once_flag _initInstanceFlag;
            static void cleanUp();
            static GAEvents* getInstance();

            static void initInstance()
            {
                if(!_destroyed && !_instance)
                {
                    _instance = new GAEvents();
                    std::atexit(&cleanUp);
                }
            }

            bool isRunning;
            bool keepRunning;
        };
    }
}
