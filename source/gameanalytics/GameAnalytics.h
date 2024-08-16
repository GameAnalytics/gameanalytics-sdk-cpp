//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include "GACommon.h"
#include "GALogger.h"


#include <vector>
#include <memory>
#include <future>

#if GA_SHARED_LIB
#include "GameAnalyticsExtern.h"
#endif

namespace gameanalytics
{
    class IRemoteConfigsListener
    {
        public:
            virtual void onRemoteConfigsUpdated() = 0;
    };

    class GameAnalytics
    {
     public:

         // configure calls should be used before initialize
         static void configureAvailableCustomDimensions01(const StringVector &customDimensions);
         static void configureAvailableCustomDimensions02(const StringVector &customDimensions);
         static void configureAvailableCustomDimensions03(const StringVector &customDimensions);

         static void configureAvailableResourceCurrencies(const StringVector &resourceCurrencies);
         static void configureAvailableResourceItemTypes(const StringVector &resourceItemTypes);

         static void configureBuild(std::string const& build);
         static void configureWritablePath(std::string const& writablePath);
         static void configureBuildPlatform(std::string const& platform);
         static void configureCustomLogHandler(const LogHandler &logHandler);
         static void disableDeviceInfo();
         static void configureDeviceModel(std::string const& deviceModel);
         static void configureDeviceManufacturer(std::string const& deviceManufacturer);

         // the version of SDK code used in an engine. Used for sdk_version field.
         // !! if set then it will override the SdkWrapperVersion.
         // example "unity 4.6.9"
         static void configureSdkGameEngineVersion(std::string const& sdkGameEngineVersion);
         // the version of the game engine (if used and version is available)
         static void configureGameEngineVersion(std::string const& engineVersion);

         static void configureUserId(std::string const& uId);

         // initialize - starting SDK (need configuration before starting)
         static void initialize(std::string const& gameKey, std::string const& gameSecret);

         // add events
         static void addBusinessEvent(std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType);
         static void addBusinessEvent(std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, std::string const& customFields);
         static void addBusinessEvent(std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, std::string const& customFields, bool mergeFields);

         static void addResourceEvent(EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId);
         static void addResourceEvent(EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, std::string const& customFields);
         static void addResourceEvent(EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, std::string const& customFields, bool mergeFields);

         static void addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03);
         static void addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, std::string const& customFields);
         static void addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, std::string const& customFields, bool mergeFields);

         static void addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score);
         static void addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, std::string const& customFields);
         static void addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, std::string const& customFields, bool mergeFields);

         static void addDesignEvent(std::string const& eventId);
         static void addDesignEvent(std::string const& eventId, std::string const& customFields);
         static void addDesignEvent(std::string const& eventId, std::string const& customFields, bool mergeFields);

         static void addDesignEvent(std::string const& eventId, double value);
         static void addDesignEvent(std::string const& eventId, double value, std::string const& customFields);
         static void addDesignEvent(std::string const& eventId, double value, std::string const& customFields, bool mergeFields);

         static void addErrorEvent(EGAErrorSeverity severity, std::string const& message);
         static void addErrorEvent(EGAErrorSeverity severity, std::string const& message, std::string const& customFields);
         static void addErrorEvent(EGAErrorSeverity severity, std::string const& message, std::string const& customFields, bool mergeFields);

         // set calls can be changed at any time (pre- and post-initialize)
         // some calls only work after a configure is called (setCustomDimension)
         static void setEnabledInfoLog(bool flag);
         static void setEnabledVerboseLog(bool flag);
         static void setEnabledManualSessionHandling(bool flag);
         static void setEnabledErrorReporting(bool flag);
         static void setEnabledEventSubmission(bool flag);
         static void setCustomDimension01(std::string const& dimension01);
         static void setCustomDimension02(std::string const& dimension02);
         static void setCustomDimension03(std::string const& dimension03);

         static void setGlobalCustomEventFields(std::string const& customFields);

         static void startSession();
         static void endSession();

         static std::string getRemoteConfigsValueAsString(std::string const& key);
         static std::string getRemoteConfigsValueAsString(std::string const& key, std::string const& defaultValue);

         static bool        isRemoteConfigsReady();
         static void        addRemoteConfigsListener(const std::shared_ptr<IRemoteConfigsListener> &listener);
         static void        removeRemoteConfigsListener(const std::shared_ptr<IRemoteConfigsListener> &listener);
         
         static std::string getRemoteConfigsContentAsString();
         static std::string getRemoteConfigsContentAsJson();

         static std::string getABTestingId();
         static std::string getABTestingVariantId();

         // game state changes
         // will affect how session is started / ended
         static void onResume();
         static void onSuspend();
         static void onQuit();

         static bool isThreadEnding();

     private:
         static bool _endThread;

        static bool isSdkReady(bool needsInitialized);
        static bool isSdkReady(bool needsInitialized, bool warn);
        static bool isSdkReady(bool needsInitialized, bool warn, std::string const& message);

#if USE_UWP
        static void OnAppSuspending(Platform::Object ^sender, Windows::ApplicationModel::SuspendingEventArgs ^e);
        static void OnAppResuming(Platform::Object ^sender, Platform::Object ^args);
#endif

    };
} // namespace gameanalytics
