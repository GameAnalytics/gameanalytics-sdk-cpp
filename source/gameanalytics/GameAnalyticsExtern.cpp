#if GA_SHARED_LIB

#include "GameAnalytics.h"
#include "GAUtilities.h"
#include <vector>

void ga_string_alloc(GAString* s, int size)
{
    if(s)
    {
        s->str = (const char*)std::malloc(size);
        if(s->str)
        {
            s->size = size;
        }
    }
}

void ga_string_free(GAString* s)
{
    if(s)
    {
        if(s->str && s->size)
        {
            std::free(s->str);
            s->str = NULL;
            s->size = 0;
        }
    }
}

GAString ga_string_fromString(std::string const& s)
{
    GAString gaStr = {};
    const unsigned int len = s.length() + 1;
    ga_string_alloc(&gaStr, len);

    if(gaStr->str && gaStr->size)
    {
        std::memcpy(gaStr, s.data(), s.length());
        gaStr[len - 1] = '\0';
    }

    return gaStr;
}

void configureAvailableResourceCurrencies(const char *resourceCurrenciesJson)
{
    gameanalytics::GameAnalytics::configureAvailableResourceCurrencies(resourceCurrenciesJson);
}

void configureAvailableResourceItemTypes(const char *resourceItemTypesJson)
{
    gameanalytics::GameAnalytics::configureAvailableResourceItemTypes(resourceItemTypesJson);
}

void configureBuild(const char *build)
{
    gameanalytics::GameAnalytics::configureBuild(build);
}

void configureWritablePath(const char *writablePath)
{
    gameanalytics::GameAnalytics::configureWritablePath(writablePath);
}

void configureDeviceModel(const char *deviceModel)
{
    gameanalytics::GameAnalytics::configureDeviceModel(deviceModel);
}

void configureDeviceManufacturer(const char *deviceManufacturer)
{
    gameanalytics::GameAnalytics::configureDeviceManufacturer(deviceManufacturer);
}

// the version of SDK code used in an engine. Used for sdk_version field.
// !! if set then it will override the SdkWrapperVersion.
// example "unity 4.6.9"
void configureSdkGameEngineVersion(const char *sdkGameEngineVersion)
{
    gameanalytics::GameAnalytics::configureSdkGameEngineVersion(sdkGameEngineVersion);
}

// the version of the game engine (if used and version is available)
void configureGameEngineVersion(const char *engineVersion)
{
    gameanalytics::GameAnalytics::configureGameEngineVersion(engineVersion);
}

void configureUserId(const char *uId)
{
    gameanalytics::GameAnalytics::configureUserId(uId);
}

void configureExternalUserId(const char* extId)
{
    gameanalytics::GameAnalytics::configureExternalUserId(extId);
}

// initialize - starting SDK (need configuration before starting)
void initialize(const char *gameKey, const char *gameSecret)
{
    gameanalytics::GameAnalytics::initialize(gameKey, gameSecret);
}

// add events
void addBusinessEvent(const char *currency, double amount, const char *itemType, const char *itemId, const char *cartType, const char *fields, double mergeFields)
{
    gameanalytics::GameAnalytics::addBusinessEvent(currency, (int)amount, itemType, itemId, cartType, fields, mergeFields != 0.0);
}

void addResourceEvent(double flowType, const char *currency, double amount, const char *itemType, const char *itemId, const char *fields, double mergeFields)
{
    int flowTypeInt = (int)flowType;
    gameanalytics::GameAnalytics::addResourceEvent((gameanalytics::EGAResourceFlowType)flowTypeInt, currency, (float)amount, itemType, itemId, fields, mergeFields != 0.0);
}

void addProgressionEvent(double progressionStatus, const char *progression01, const char *progression02, const char *progression03, const char *fields, double mergeFields)
{
    int progressionStatusInt = (int)progressionStatus;
    gameanalytics::GameAnalytics::addProgressionEvent((gameanalytics::EGAProgressionStatus)progressionStatusInt, progression01, progression02, progression03, fields, mergeFields != 0.0);
}

void addProgressionEventWithScore(double progressionStatus, const char *progression01, const char *progression02, const char *progression03, double score, const char *fields, double mergeFields)
{
    int progressionStatusInt = (int)progressionStatus;
    gameanalytics::GameAnalytics::addProgressionEvent((gameanalytics::EGAProgressionStatus)progressionStatusInt, progression01, progression02, progression03, (int)score, fields, mergeFields != 0.0);
}

void addDesignEvent(const char *eventId, const char *fields, double mergeFields)
{
    gameanalytics::GameAnalytics::addDesignEvent(eventId, fields, mergeFields != 0.0);
}

void addDesignEventWithValue(const char *eventId, double value, const char *fields, double mergeFields)
{
    gameanalytics::GameAnalytics::addDesignEvent(eventId, value, fields, mergeFields != 0.0);
}

void addErrorEvent(double severity, const char *message, const char *fields, double mergeFields)
{
    int severityInt = (int)severity;
    gameanalytics::GameAnalytics::addErrorEvent((gameanalytics::EGAErrorSeverity)severityInt, message, fields, mergeFields != 0.0);
}

// set calls can be changed at any time (pre- and post-initialize)
// some calls only work after a configure is called (setCustomDimension)

void setEnabledInfoLog(double flag)
{
    gameanalytics::GameAnalytics::setEnabledInfoLog(flag != 0.0);
}

void setEnabledVerboseLog(double flag)
{
    gameanalytics::GameAnalytics::setEnabledVerboseLog(flag != 0.0);
}

void setEnabledManualSessionHandling(double flag)
{
    gameanalytics::GameAnalytics::setEnabledManualSessionHandling(flag != 0.0);
}

void setEnabledErrorReporting(double flag)
{
    gameanalytics::GameAnalytics::setEnabledErrorReporting(flag != 0.0);
}

void setEnabledEventSubmission(double flag)
{
    gameanalytics::GameAnalytics::setEnabledEventSubmission(flag != 0.0);
}

void setCustomDimension01(const char *dimension01)
{
    gameanalytics::GameAnalytics::setCustomDimension01(dimension01);
}

void setCustomDimension02(const char *dimension02)
{
    gameanalytics::GameAnalytics::setCustomDimension02(dimension02);
}

void setCustomDimension03(const char *dimension03)
{
    gameanalytics::GameAnalytics::setCustomDimension03(dimension03);
}

void setGlobalCustomEventFields(const char *customFields)
{
    gameanalytics::GameAnalytics::setGlobalCustomEventFields(customFields);
}

void gameAnalyticsStartSession()
{
    gameanalytics::GameAnalytics::startSession();
}

void gameAnalyticsEndSession()
{
    gameanalytics::GameAnalytics::endSession();
}

// game state changes
// will affect how session is started / ended
void onResume()
{
    gameanalytics::GameAnalytics::onResume();
}

void onSuspend()
{
    gameanalytics::GameAnalytics::onSuspend();
}

void onQuit()
{
    gameanalytics::GameAnalytics::onQuit();
}

GAString getRemoteConfigsValueAsString(const char *key)
{
    std::string returnValue = gameanalytics::GameAnalytics::getRemoteConfigsValueAsString(key);
    return ga_string_fromString(returnValue);
}

GAString getRemoteConfigsValueAsStringWithDefaultValue(const char *key, const char *defaultValue)
{
    std::string returnValue = gameanalytics::GameAnalytics::getRemoteConfigsValueAsString(key, defaultValue);
    return ga_string_fromString(returnValue);
}

double isRemoteConfigsReady()
{
    return gameanalytics::GameAnalytics::isRemoteConfigsReady() ? 1 : 0;
}

GAString getRemoteConfigsContentAsString()
{
    std::string returnValue = gameanalytics::GameAnalytics::getRemoteConfigsContentAsString();
    return ga_string_fromString(returnValue);
}

GAString getABTestingId()
{
    std::string returnValue = gameanalytics::GameAnalytics::getABTestingId();
    return ga_string_fromString(returnValue);
}

GAString getABTestingVariantId()
{
    std::string returnValue = gameanalytics::GameAnalytics::getABTestingVariantId();
    return ga_string_fromString(returnValue);
}

#endif
