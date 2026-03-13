#include "GameAnalyticsExtern.h"

#ifdef GA_SHARED_LIB

#include "GameAnalytics/GameAnalytics.h"
#include "GAUtilities.h"
#include <cstring>
#include <cstdlib>

gameanalytics::StringVector makeStringVector(const char** arr, int size)
{
    if(size > 0 && arr)
    {
        gameanalytics::StringVector v;
        v.reserve(size);

        for(int i = 0; i < size; ++i)
        {
            if(arr[i])
                v.push_back(arr[i]);
        }

        return v;
    }

    return {};
}

GA_API void gameAnalytics_freeString(const char* ptr)
{
    std::free((void*)ptr);
}

// Cross-platform string duplication function
static char* ga_strndup(const char* s, size_t n)
{
    if (!s) return nullptr;
    
    size_t len = std::strlen(s);
    if (len > n) len = n;
    
    char* result = (char*)std::malloc(len + 1);
    if (result)
    {
        std::memcpy(result, s, len);
        result[len] = '\0';
    }
    return result;
}

const char* gameAnalytics_allocString(std::string const& s)
{
    return ga_strndup(s.c_str(), s.size());
}

static inline const char* safeString(const char* str)
{
    return str ? str : "";
}

GA_API void gameAnalytics_configureAvailableCustomDimensions01(const char **customDimensions, int size)
{
    gameanalytics::StringVector values = makeStringVector(customDimensions, size);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions01(values);
}

GA_API void gameAnalytics_configureAvailableCustomDimensions02(const char **customDimensions, int size)
{
    gameanalytics::StringVector values = makeStringVector(customDimensions, size);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions02(values);
}

GA_API void gameAnalytics_configureAvailableCustomDimensions03(const char **customDimensions, int size)
{
    gameanalytics::StringVector values = makeStringVector(customDimensions, size);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions03(values);
}

GA_API void gameAnalytics_configureAvailableResourceCurrencies(const char** currencies, int size)
{
    gameanalytics::StringVector values = makeStringVector(currencies, size);
    gameanalytics::GameAnalytics::configureAvailableResourceCurrencies(values);
}

GA_API void gameAnalytics_configureAvailableResourceItemTypes(const char** resources, int size)
{
    gameanalytics::StringVector values = makeStringVector(resources, size);
    gameanalytics::GameAnalytics::configureAvailableResourceItemTypes(values);
}

GA_API void gameAnalytics_configureBuild(const char *build)
{
    gameanalytics::GameAnalytics::configureBuild(safeString(build));
}

GA_API void gameAnalytics_configureWritablePath(const char *writablePath)
{
    gameanalytics::GameAnalytics::configureWritablePath(safeString(writablePath));
}

GA_API void gameAnalytics_configureBuildPlatform(const char *platform)
{
    gameanalytics::GameAnalytics::configureBuildPlatform(safeString(platform));
}

GA_API void gameAnalytics_configureCustomLogHandler(GALogHandler handler)
{
    if (!handler)
    {
        gameanalytics::GameAnalytics::configureCustomLogHandler(nullptr);
        return;
    }
    gameanalytics::GameAnalytics::configureCustomLogHandler(
        [=](std::string const& message, gameanalytics::EGALoggerMessageType messageType)
        {
            handler(message.c_str(), (GALoggerMessageType)messageType);
        }
    );
}

GA_API void gameAnalytics_disableDeviceInfo()
{
    gameanalytics::GameAnalytics::disableDeviceInfo();
}

GA_API void gameAnalytics_configureDeviceModel(const char *deviceModel)
{
    gameanalytics::GameAnalytics::configureDeviceModel(safeString(deviceModel));
}

GA_API void gameAnalytics_configureDeviceManufacturer(const char *deviceManufacturer)
{
    gameanalytics::GameAnalytics::configureDeviceManufacturer(safeString(deviceManufacturer));
}

// the version of SDK code used in an engine. Used for sdk_version field.
// !! if set then it will override the SdkWrapperVersion.
// example "unity 4.6.9"
GA_API void gameAnalytics_configureSdkGameEngineVersion(const char *sdkGameEngineVersion)
{
    gameanalytics::GameAnalytics::configureSdkGameEngineVersion(safeString(sdkGameEngineVersion));
}

// the version of the game engine (if used and version is available)
GA_API void gameAnalytics_configureGameEngineVersion(const char *engineVersion)
{
    gameanalytics::GameAnalytics::configureGameEngineVersion(safeString(engineVersion));
}

GA_API void gameAnalytics_configureUserId(const char *uId)
{
    gameanalytics::GameAnalytics::configureUserId(safeString(uId));
}

GA_API void gameAnalytics_configureExternalUserId(const char* extId)
{
    gameanalytics::GameAnalytics::configureExternalUserId(safeString(extId));
}

// initialize - starting SDK (need configuration before starting)
GA_API void gameAnalytics_initialize(const char *gameKey, const char *gameSecret)
{
    gameanalytics::GameAnalytics::initialize(safeString(gameKey), safeString(gameSecret));
}

// add events
GA_API void gameAnalytics_addBusinessEvent(const char *currency, double amount, const char *itemType, const char *itemId, const char *cartType, const char *customFields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addBusinessEvent(safeString(currency), (int)amount, safeString(itemType), safeString(itemId), safeString(cartType), safeString(customFields), mergeFields);
}

GA_API void gameAnalytics_addResourceEvent(GAResourceFlowType flowType, const char *currency, double amount, const char *itemType, const char *itemId, const char *customFields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addResourceEvent((gameanalytics::EGAResourceFlowType)flowType, safeString(currency), (float)amount, safeString(itemType), safeString(itemId), safeString(customFields), mergeFields);
}

GA_API void gameAnalytics_addProgressionEvent(GAProgressionStatus progressionStatus, const char *progression01, const char *progression02, const char *progression03, const char *customFields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addProgressionEvent((gameanalytics::EGAProgressionStatus)progressionStatus, safeString(progression01), safeString(progression02), safeString(progression03), safeString(customFields), mergeFields);
}

GA_API void gameAnalytics_addProgressionEventWithScore(GAProgressionStatus progressionStatus, const char *progression01, const char *progression02, const char *progression03, int score, const char *customFields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addProgressionEvent((gameanalytics::EGAProgressionStatus)progressionStatus, score, safeString(progression01), safeString(progression02), safeString(progression03), safeString(customFields), mergeFields);
}

GA_API void gameAnalytics_addDesignEvent(const char *eventId, const char *customFields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addDesignEvent(safeString(eventId), safeString(customFields), (bool)mergeFields);
}

GA_API void gameAnalytics_addDesignEventWithValue(const char *eventId, double value, const char *customFields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addDesignEvent(safeString(eventId), value, safeString(customFields), (bool)mergeFields);
}

GA_API void gameAnalytics_addErrorEvent(GAErrorSeverity severity, const char *message, const char *customFields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addErrorEvent((gameanalytics::EGAErrorSeverity)severity, safeString(message), safeString(customFields), (bool)mergeFields);
}

// set calls can be changed at any time (pre- and post-initialize)
// some calls only work after a configure is called (setCustomDimension)

GA_API void gameAnalytics_setEnabledInfoLog(GAStatus flag)
{
    gameanalytics::GameAnalytics::setEnabledInfoLog(flag);
}

GA_API void gameAnalytics_setEnabledVerboseLog(GAStatus flag)
{
    gameanalytics::GameAnalytics::setEnabledVerboseLog(flag);
}

GA_API void gameAnalytics_setEnabledManualSessionHandling(GAStatus flag)
{
    gameanalytics::GameAnalytics::setEnabledManualSessionHandling(flag);
}

GA_API void gameAnalytics_setEnabledErrorReporting(GAStatus flag)
{
    gameanalytics::GameAnalytics::setEnabledErrorReporting(flag);
}

GA_API void gameAnalytics_setEnabledEventSubmission(GAStatus flag)
{
    gameanalytics::GameAnalytics::setEnabledEventSubmission(flag);
}

GA_API void gameAnalytics_setCustomDimension01(const char *dimension01)
{
    gameanalytics::GameAnalytics::setCustomDimension01(safeString(dimension01));
}

GA_API void gameAnalytics_setCustomDimension02(const char *dimension02)
{
    gameanalytics::GameAnalytics::setCustomDimension02(safeString(dimension02));
}

GA_API void gameAnalytics_setCustomDimension03(const char *dimension03)
{
    gameanalytics::GameAnalytics::setCustomDimension03(safeString(dimension03));
}

GA_API void gameAnalytics_setGlobalCustomEventFields(const char *customFields)
{
    gameanalytics::GameAnalytics::setGlobalCustomEventFields(safeString(customFields));
}

GA_API void gameAnalytics_startSession()
{
    gameanalytics::GameAnalytics::startSession();
}

GA_API void gameAnalytics_endSession()
{
    gameanalytics::GameAnalytics::endSession();
}

// game state changes
// will affect how session is started / ended
GA_API void gameAnalytics_onResume()
{
    gameanalytics::GameAnalytics::onResume();
}

GA_API void gameAnalytics_onSuspend()
{
    gameanalytics::GameAnalytics::onSuspend();
}

GA_API void gameAnalytics_onQuit()
{
    gameanalytics::GameAnalytics::onQuit();
}

GA_API const char* gameAnalytics_getUserId()
{
    std::string returnValue = gameanalytics::GameAnalytics::getUserId();
    return gameAnalytics_allocString(returnValue);
}

GA_API const char* gameAnalytics_getExternalUserId()
{
    std::string returnValue = gameanalytics::GameAnalytics::getExternalUserId();
    return gameAnalytics_allocString(returnValue);
}

GA_API const char* gameAnalytics_getRemoteConfigsValueAsString(const char *key)
{
    std::string returnValue = gameanalytics::GameAnalytics::getRemoteConfigsValueAsString(safeString(key));
    return gameAnalytics_allocString(returnValue);
}

GA_API const char* gameAnalytics_getRemoteConfigsValueAsStringWithDefaultValue(const char *key, const char *defaultValue)
{
    std::string returnValue = gameanalytics::GameAnalytics::getRemoteConfigsValueAsString(safeString(key), safeString(defaultValue));
    return gameAnalytics_allocString(returnValue);
}

GA_API GAStatus gameAnalytics_isRemoteConfigsReady()
{
    return gameanalytics::GameAnalytics::isRemoteConfigsReady() ? EGAEnabled : EGADisabled;
}

GA_API const char* gameAnalytics_getRemoteConfigsContentAsString()
{
    std::string returnValue = gameanalytics::GameAnalytics::getRemoteConfigsContentAsString();
    return gameAnalytics_allocString(returnValue);
}

GA_API const char* gameAnalytics_getRemoteConfigsValueAsJson(const char* key)
{
    std::string returnValue = gameanalytics::GameAnalytics::getRemoteConfigsValueAsJson(safeString(key));
    return gameAnalytics_allocString(returnValue);
}

GA_API const char* gameAnalytics_getABTestingId()
{
    std::string returnValue = gameanalytics::GameAnalytics::getABTestingId();
    return gameAnalytics_allocString(returnValue);
}

GA_API const char* gameAnalytics_getABTestingVariantId()
{
    std::string returnValue = gameanalytics::GameAnalytics::getABTestingVariantId();
    return gameAnalytics_allocString(returnValue);
}

GA_API long long gameAnalytics_getElapsedSessionTime()
{
    return gameanalytics::GameAnalytics::getElapsedSessionTime();
}

GA_API long long gameAnalytics_getElapsedTimeFromAllSessions()
{
    return gameanalytics::GameAnalytics::getElapsedTimeFromAllSessions();
}

GA_API long long gameAnalytics_getElapsedTimeForPreviousSession()
{
    return gameanalytics::GameAnalytics::getElapsedTimeForPreviousSession();
}

GA_API void gameAnalytics_enableSDKInitEvent(GAStatus status)
{
    return gameanalytics::GameAnalytics::enableSDKInitEvent(status);
}

GA_API void gameAnalytics_enableMemoryHistogram(GAStatus status)
{
    return gameanalytics::GameAnalytics::enableMemoryHistogram(status);
}

GA_API void gameAnalytics_enableFPSHistogram(GAFpsTracker tracker, GAStatus status)
{
    gameanalytics::FPSTracker fpsTracker =
    [=]() -> float
    {
        return tracker();
    };
    
    return gameanalytics::GameAnalytics::enableFPSHistogram(fpsTracker, status);
}

GA_API void gameAnalytics_enableHardwareTracking(GAStatus status)
{
    return gameanalytics::GameAnalytics::enableHardwareTracking(status);
}

#endif
