//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include "GACommon.h"
#include "GameAnalytics.h"

namespace gameanalytics
{
    namespace events
    {
        class GAEvents
        {
            friend class state::GAState;

         public:

            static GAEvents& getInstance();
            static void stopEventQueue();
            static void ensureEventQueueIsRunning();
            static void addSessionStartEvent();
            static void addSessionEndEvent();
            static void addBusinessEvent(std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, const json& fields, bool mergeFields);
            static void addResourceEvent(EGAResourceFlowType flowType, std::string const& currency, double amount, std::string const& itemType, std::string const& itemId, const json& fields, bool mergeFields);
            static void addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, bool sendScore, const json& fields, bool mergeFields);
            static void addDesignEvent(std::string const& eventId, double value, bool sendValue, const json& fields, bool mergeFields);
            static void addErrorEvent(EGAErrorSeverity severity, std::string const& message, std::string const& function, int32_t line, const json& fields, bool mergeFields, bool skipAddingFields = false);

            static void addSDKInitEvent();
            static void addHealthEvent();
            
            static std::string progressionStatusString(EGAProgressionStatus progressionStatus);
            static std::string errorSeverityString(EGAErrorSeverity errorSeverity);
            static std::string resourceFlowTypeString(EGAResourceFlowType flowType);

            static void processEvents(std::string const& category, bool performCleanUp);

            bool enableSDKInitEvent{false};
            bool enableHealthEvent{false};

        private:

            static constexpr const char* CategorySessionStart           = "user";
            static constexpr const char* CategorySessionEnd             = "session_end";
            static constexpr const char* CategoryDesign                 = "design";
            static constexpr const char* CategoryBusiness               = "business";
            static constexpr const char* CategoryProgression            = "progression";
            static constexpr const char* CategoryResource               = "resource";
            static constexpr const char* CategoryError                  = "error";
            static constexpr const char* CategorySDKInit                = "sdk_init";
            static constexpr const char* CategoryHealth                 = "health";
            static constexpr int         MaxEventCount                  = 500;

            static constexpr std::chrono::milliseconds PROCESS_EVENTS_INTERVAL{8000};

            GAEvents();
            ~GAEvents();
            GAEvents(const GAEvents&) = delete;
            GAEvents& operator=(const GAEvents&) = delete;

            void processEventQueue();
            void cleanupEvents();
            void fixMissingSessionEndEvents();
            void addEventToStore(json& eventData);
            void addDimensionsToEvent(json& eventData);
            void addCustomFieldsToEvent(json& eventData, json& fields);
            void updateSessionTime();

            bool isRunning  {false};
            bool keepRunning{false};
        };
    }
}
