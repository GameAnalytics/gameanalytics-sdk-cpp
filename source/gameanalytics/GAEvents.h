//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include "GameAnalytics.h"
#include "rapidjson/document.h"
#include <mutex>
#include <cstdlib>

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
            static void addBusinessEvent(const char* currency, int amount, const char* itemType, const char* itemId, const char* cartType, const rapidjson::Value& fields, bool mergeFields);
            static void addResourceEvent(EGAResourceFlowType flowType, const char* currency, double amount, const char* itemType, const char* itemId, const rapidjson::Value& fields, bool mergeFields);
            static void addProgressionEvent(EGAProgressionStatus progressionStatus, const char* progression01, const char* progression02, const char* progression03, int score, bool sendScore, const rapidjson::Value& fields, bool mergeFields);
            static void addDesignEvent(const char* eventId, double value, bool sendValue, const rapidjson::Value& fields, bool mergeFields);
            static void addErrorEvent(EGAErrorSeverity severity, const char* message, const rapidjson::Value& fields, bool mergeFields);
            static void addErrorEvent(EGAErrorSeverity severity, const char* message, const rapidjson::Value& fields, bool mergeFields, bool skipAddingFields);
            static void progressionStatusString(EGAProgressionStatus progressionStatus, char* out);

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
            void addEventToStore(rapidjson::Document &eventData);
            void addDimensionsToEvent(rapidjson::Document& eventData);
            void addCustomFieldsToEvent(rapidjson::Document& eventData, rapidjson::Document& fields);
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
