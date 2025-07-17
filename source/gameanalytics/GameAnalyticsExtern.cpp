#if GA_SHARED_LIB

#include "GameAnalytics/GameAnalytics.h"
#include "GAUtilities.h"

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

GAErrorCode copyStringBuffer(std::string const& s, char* out, int* size)
{
    if(size && (*size > 0))
    {
        if(out && (*size >= s.size()))
        {
            std::memcpy(out, s.data(), s.size());
        }
        else
        {
            return EGABufferError;
        }

        *size = s.size();
        return EGANoError;
    }

    return EGAFailure;
}

void gameAnalytics_configureAvailableCustomDimensions01(const char **customDimensions, int size)
{
    StringVector values = makeStringVector(customDimensions, size);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions01(values);
}

void gameAnalytics_configureAvailableCustomDimensions02(const char **customDimensions, int size)
{
    StringVector values = makeStringVector(customDimensions, size);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions02(values);
}

void gameAnalytics_configureAvailableCustomDimensions03(const char **customDimensions, int size)
{
    StringVector values = makeStringVector(customDimensions, size);
    gameanalytics::GameAnalytics::configureAvailableCustomDimensions03(values);
}

void gameAnalytics_configureAvailableResourceCurrencies(const char** currencies, int size)
{
    StringVector values = makeStringVector(currencies, size);
    gameanalytics::GameAnalytics::configureAvailableResourceCurrencies(values);
}

void gameAnalytics_configureAvailableResourceItemTypes(const char** resources, int size)
{
    StringVector values = makeStringVector(resources, size);
    gameanalytics::GameAnalytics::configureAvailableResourceItemTypes(values);
}

void gameAnalytics_configureBuild(const char *build)
{
    gameanalytics::GameAnalytics::configureBuild(build);
}

void gameAnalytics_configureWritablePath(const char *writablePath)
{
    gameanalytics::GameAnalytics::configureWritablePath(writablePath);
}

void gameAnalytics_configureDeviceModel(const char *deviceModel)
{
    gameanalytics::GameAnalytics::configureDeviceModel(deviceModel);
}

void gameAnalytics_configureDeviceManufacturer(const char *deviceManufacturer)
{
    gameanalytics::GameAnalytics::configureDeviceManufacturer(deviceManufacturer);
}

// the version of SDK code used in an engine. Used for sdk_version field.
// !! if set then it will override the SdkWrapperVersion.
// example "unity 4.6.9"
void gameAnalytics_configureSdkGameEngineVersion(const char *sdkGameEngineVersion)
{
    gameanalytics::GameAnalytics::configureSdkGameEngineVersion(sdkGameEngineVersion);
}

// the version of the game engine (if used and version is available)
void gameAnalytics_configureGameEngineVersion(const char *engineVersion)
{
    gameanalytics::GameAnalytics::configureGameEngineVersion(engineVersion);
}

void gameAnalytics_configureUserId(const char *uId)
{
    gameanalytics::GameAnalytics::configureUserId(uId);
}

void gameAnalytics_configureExternalUserId(const char* extId)
{
    gameanalytics::GameAnalytics::configureExternalUserId(extId);
}

// initialize - starting SDK (need configuration before starting)
void gameAnalytics_initialize(const char *gameKey, const char *gameSecret)
{
    gameanalytics::GameAnalytics::initialize(gameKey, gameSecret);
}

// add events
void gameAnalytics_addBusinessEvent(const char *currency, double amount, const char *itemType, const char *itemId, const char *cartType, const char *fields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addBusinessEvent(currency, (int)amount, itemType, itemId, cartType, fields, mergeFields);
}

void gameAnalytics_addResourceEvent(int flowType, const char *currency, double amount, const char *itemType, const char *itemId, const char *fields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addResourceEvent((gameanalytics::EGAResourceFlowType)flowType, currency, (float)amount, itemType, itemId, fields, mergeFields);
}

void gameAnalytics_addProgressionEvent(int progressionStatus, const char *progression01, const char *progression02, const char *progression03, const char *fields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addProgressionEvent((gameanalytics::EGAProgressionStatus)progressionStatus, progression01, progression02, progression03, fields, mergeFields);
}

void gameAnalytics_addProgressionEventWithScore(int progressionStatus, const char *progression01, const char *progression02, const char *progression03, double score, const char *fields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addProgressionEvent((gameanalytics::EGAProgressionStatus)progressionStatus, progression01, progression02, progression03, (int)score, fields, mergeFields);
}

void gameAnalytics_addDesignEvent(const char *eventId, const char *fields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addDesignEvent(eventId, fields, (bool)mergeFields);
}

void gameAnalytics_addDesignEventWithValue(const char *eventId, double value, const char *fields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addDesignEvent(eventId, value, fields, (bool)mergeFields);
}

void gameAnalytics_addErrorEvent(int severity, const char *message, const char *fields, GAStatus mergeFields)
{
    gameanalytics::GameAnalytics::addErrorEvent((gameanalytics::EGAErrorSeverity)severity, message, fields, (bool)mergeFields);
}

// set calls can be changed at any time (pre- and post-initialize)
// some calls only work after a configure is called (setCustomDimension)

void gameAnalytics_setEnabledInfoLog(GAStatus flag)
{
    gameanalytics::GameAnalytics::setEnabledInfoLog(flag);
}

void gameAnalytics_setEnabledVerboseLog(GAStatus flag)
{
    gameanalytics::GameAnalytics::setEnabledVerboseLog(flag);
}

void gameAnalytics_setEnabledManualSessionHandling(GAStatus flag)
{
    gameanalytics::GameAnalytics::setEnabledManualSessionHandling(flag);
}

void gameAnalytics_setEnabledErrorReporting(GAStatus flag)
{
    gameanalytics::GameAnalytics::setEnabledErrorReporting(flag);
}

void gameAnalytics_setEnabledEventSubmission(GAStatus flag)
{
    gameanalytics::GameAnalytics::setEnabledEventSubmission(flag);
}

void gameAnalytics_setCustomDimension01(const char *dimension01)
{
    gameanalytics::GameAnalytics::setCustomDimension01(dimension01);
}

void gameAnalytics_setCustomDimension02(const char *dimension02)
{
    gameanalytics::GameAnalytics::setCustomDimension02(dimension02);
}

void gameAnalytics_setCustomDimension03(const char *dimension03)
{
    gameanalytics::GameAnalytics::setCustomDimension03(dimension03);
}

void gameAnalytics_setGlobalCustomEventFields(const char *customFields)
{
    gameanalytics::GameAnalytics::setGlobalCustomEventFields(customFields);
}

void gameAnalytics_startSession()
{
    gameanalytics::GameAnalytics::startSession();
}

void gameAnalytics_endSession()
{
    gameanalytics::GameAnalytics::endSession();
}

// game state changes
// will affect how session is started / ended
void gameAnalytics_onResume()
{
    gameanalytics::GameAnalytics::onResume();
}

void gameAnalytics_onSuspend()
{
    gameanalytics::GameAnalytics::onSuspend();
}

void gameAnalytics_onQuit()
{
    gameanalytics::GameAnalytics::onQuit();
}

void gameAnalytics_getRemoteConfigsValueAsString(const char *key, char* out, int* size)
{
    std::string returnValue = gameanalytics::GameAnalytics::getRemoteConfigsValueAsString(key);
    return copyStringBuffer(returnValue, out, size);
}

GAErrorCode gameAnalytics_getRemoteConfigsValueAsStringWithDefaultValue(const char *key, const char *defaultValue, char* out, int* size)
{
    std::string returnValue = gameanalytics::GameAnalytics::getRemoteConfigsValueAsString(key, defaultValue);
    return copyStringBuffer(returnValue, out, size);
}

GAStatus gameAnalytics_isRemoteConfigsReady()
{
    return gameanalytics::GameAnalytics::isRemoteConfigsReady() ? GAEnabled : GADisabled;
}

GAErrorCode gameAnalytics_getRemoteConfigsContentAsString(char* out, int* size)
{
    std::string returnValue = gameanalytics::GameAnalytics::getRemoteConfigsContentAsString();
    return copyStringBuffer(returnValue, out, size);
}

GAErrorCode gameAnalytics_getABTestingId(char* out, int* size)
{
    std::string returnValue = gameanalytics::GameAnalytics::getABTestingId();
    return copyStringBuffer(returnValue, out, size);
}

GAErrorCode gameAnalytics_getABTestingVariantId(char* out, int* size)
{
    std::string returnValue = gameanalytics::GameAnalytics::getABTestingVariantId();
    return copyStringBuffer(returnValue, out, size);
}

#endif
