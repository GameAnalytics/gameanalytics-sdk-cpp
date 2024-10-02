#if GA_SHARED_LIB

#include "GameAnalytics/GameAnalytics.h"
#include "GAUtilities.h"

void ga_string_alloc(GAString* s, unsigned int size)
{
    if(s)
    {
        s->str = (const char*)std::malloc(size);
        if(s->str)
        {
            s->size = size;
            std::memset(s->str, '\0', s->size);
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

StringVector makeStringVector(const char** arr, int size)
{
    if(size > 0 && arr)
    {
        StringVector v;
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

void configureAvailableCustomDimensions01(const char **customDimensions, int size)
{
    StringVector values = makeStringVector(customDimensions, size);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions01(values);
}

void configureAvailableCustomDimensions02(const char **customDimensions, int size)
{
    StringVector values = makeStringVector(customDimensions, size);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions02(values);
}

void configureAvailableCustomDimensions03(const char **customDimensions, int size)
{
    StringVector values = makeStringVector(customDimensions, size);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions03(values);
}

void configureAvailableResourceCurrencies(const char** currencies, int size)
{
    StringVector values = makeStringVector(currencies, size);
    gameanalytics::GameAnalytics::configureAvailableResourceCurrencies(values);
}

void configureAvailableResourceItemTypes(const char** resources, int size)
{
    StringVector values = makeStringVector(resources, size);
    gameanalytics::GameAnalytics::configureAvailableResourceItemTypes(values);
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
void addBusinessEvent(const char *currency, double amount, const char *itemType, const char *itemId, const char *cartType, const char *fields, GA_BOOL mergeFields)
{
    gameanalytics::GameAnalytics::addBusinessEvent(currency, (int)amount, itemType, itemId, cartType, fields, mergeFields);
}

void addResourceEvent(int flowType, const char *currency, double amount, const char *itemType, const char *itemId, const char *fields, GA_BOOL mergeFields)
{
    gameanalytics::GameAnalytics::addResourceEvent((gameanalytics::EGAResourceFlowType)flowType, currency, (float)amount, itemType, itemId, fields, mergeFields);
}

void addProgressionEvent(int progressionStatus, const char *progression01, const char *progression02, const char *progression03, const char *fields, GA_BOOL mergeFields)
{
    gameanalytics::GameAnalytics::addProgressionEvent((gameanalytics::EGAProgressionStatus)progressionStatus, progression01, progression02, progression03, fields, mergeFields);
}

void addProgressionEventWithScore(int progressionStatus, const char *progression01, const char *progression02, const char *progression03, double score, const char *fields, GA_BOOL mergeFields)
{
    gameanalytics::GameAnalytics::addProgressionEvent((gameanalytics::EGAProgressionStatus)progressionStatus, progression01, progression02, progression03, (int)score, fields, mergeFields);
}

void addDesignEvent(const char *eventId, const char *fields, GA_BOOL mergeFields)
{
    gameanalytics::GameAnalytics::addDesignEvent(eventId, fields, mergeFields);
}

void addDesignEventWithValue(const char *eventId, double value, const char *fields, GA_BOOL mergeFields)
{
    gameanalytics::GameAnalytics::addDesignEvent(eventId, value, fields, mergeFields);
}

void addErrorEvent(int severity, const char *message, const char *fields, GA_BOOL mergeFields)
{
    gameanalytics::GameAnalytics::addErrorEvent((gameanalytics::EGAErrorSeverity)severity, message, fields, mergeFields);
}

// set calls can be changed at any time (pre- and post-initialize)
// some calls only work after a configure is called (setCustomDimension)

void setEnabledInfoLog(GA_BOOL flag)
{
    gameanalytics::GameAnalytics::setEnabledInfoLog(flag);
}

void setEnabledVerboseLog(GA_BOOL flag)
{
    gameanalytics::GameAnalytics::setEnabledVerboseLog(flag);
}

void setEnabledManualSessionHandling(GA_BOOL flag)
{
    gameanalytics::GameAnalytics::setEnabledManualSessionHandling(flag);
}

void setEnabledErrorReporting(GA_BOOL flag)
{
    gameanalytics::GameAnalytics::setEnabledErrorReporting(flag);
}

void setEnabledEventSubmission(GA_BOOL flag)
{
    gameanalytics::GameAnalytics::setEnabledEventSubmission(flag);
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

GA_BOOL isRemoteConfigsReady()
{
    return gameanalytics::GameAnalytics::isRemoteConfigsReady() ? GA_TRUE : GA_FALSE;
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
