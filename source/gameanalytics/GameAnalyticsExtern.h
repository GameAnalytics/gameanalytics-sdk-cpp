#pragma once
#ifndef _GA_EXTERNAL_H_
#define _GA_EXTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#if GA_SHARED_LIB
	#if defined(_WIN32)
		#define GA_EXPORT __declspec(dllexport)
	#else
		#define GA_EXPORT __attribute__((visibility("default")))
	#endif
#else
	#define GA_EXPORT
#endif

enum GAErrorCode
{
	EGANoError = 0,
	EGAFailure,
	EGABufferError
};

enum GAStatus: char
{
	EGADisabled = 0,
	EGAEnabled
};

GA_EXPORT void gameAnalytics_configureAvailableCustomDimensions01(const char **customDimensions, int size);
GA_EXPORT void gameAnalytics_configureAvailableCustomDimensions02(const char **customDimensions, int size);
GA_EXPORT void gameAnalytics_configureAvailableCustomDimensions03(const char **customDimensions, int size);
GA_EXPORT void gameAnalytics_configureAvailableResourceCurrencies(const char **resourceCurrencies, int size);
GA_EXPORT void gameAnalytics_configureAvailableResourceItemTypes(const char **resourceItemTypes, int size);
GA_EXPORT void gameAnalytics_configureBuild(const char *build);
GA_EXPORT void gameAnalytics_configureWritablePath(const char *writablePath);
GA_EXPORT void gameAnalytics_configureDeviceModel(const char *deviceModel);
GA_EXPORT void gameAnalytics_configureDeviceManufacturer(const char *deviceManufacturer);

// the version of SDK code used in an engine. Used for sdk_version field.
// !! if set then it will override the SdkWrapperVersion.
// example "unity 4.6.9"
GA_EXPORT void gameAnalytics_configureSdkGameEngineVersion(const char *sdkGameEngineVersion);

// the version of the game engine (if used and version is available)
GA_EXPORT void gameAnalytics_configureGameEngineVersion(const char *engineVersion);

GA_EXPORT void gameAnalytics_configureUserId(const char *uId);

GA_EXPORT void gameAnalytics_configureExternalUserId(const char* extId);

// initialize - starting SDK (need configuration before starting)
GA_EXPORT void gameAnalytics_initialize(const char *gameKey, const char *gameSecret);

// add events
GA_EXPORT void gameAnalytics_addBusinessEvent(const char *currency, double amount, const char *itemType, const char *itemId, const char *cartType, const char *customFields, GAStatus mergeFields);
GA_EXPORT void gameAnalytics_addResourceEvent(int flowType, const char *currency, double amount, const char *itemType, const char *itemId, const char *customFields, GAStatus mergeFields);
GA_EXPORT void gameAnalytics_addProgressionEvent(int progressionStatus, const char *progression01, const char *progression02, const char *progression03, const char *customFields, GAStatus mergeFields);
GA_EXPORT void gameAnalytics_addProgressionEventWithScore(int progressionStatus, const char *progression01, const char *progression02, const char *progression03, double score, const char *customFields, GAStatus mergeFields);
GA_EXPORT void gameAnalytics_addDesignEvent(const char *eventId, const char *customFields, GAStatus mergeFields);
GA_EXPORT void gameAnalytics_addDesignEventWithValue(const char *eventId, double value, const char *customFields, GAStatus mergeFields);
GA_EXPORT void gameAnalytics_addErrorEvent(int severity, const char *message, const char *customFields, GAStatus mergeFields);

// set calls can be changed at any time (pre- and post-initialize)
// some calls only work after a configure is called (setCustomDimension)
GA_EXPORT void gameAnalytics_setEnabledInfoLog(GAStatus flag);
GA_EXPORT void gameAnalytics_setEnabledVerboseLog(GAStatus flag);
GA_EXPORT void gameAnalytics_setEnabledManualSessionHandling(GAStatus flag);
GA_EXPORT void gameAnalytics_setEnabledErrorReporting(GAStatus flag);
GA_EXPORT void gameAnalytics_setEnabledEventSubmission(GAStatus flag);
GA_EXPORT void gameAnalytics_setCustomDimension01(const char *dimension01);
GA_EXPORT void gameAnalytics_setCustomDimension02(const char *dimension02);
GA_EXPORT void gameAnalytics_setCustomDimension03(const char *dimension03);

GA_EXPORT void gameAnalytics_setGlobalCustomEventFields(const char *customFields);

GA_EXPORT void gameAnalytics_startSession();
GA_EXPORT void gameAnalytics_endSession();

// game state changes
// will affect how session is started / ended
GA_EXPORT void gameAnalytics_onResume();
GA_EXPORT void gameAnalytics_onSuspend();
GA_EXPORT void gameAnalytics_onQuit();

GA_EXPORT GAErrorCode gameAnalytics_getRemoteConfigsValueAsString(const char *key, char* out, int* bufferSize);
GA_EXPORT GAErrorCode gameAnalytics_getRemoteConfigsValueAsStringWithDefaultValue(const char *key, const char *defaultValue, char* out, int* bufferSize);
GA_EXPORT GAStatus    gameAnalytics_isRemoteConfigsReady();
GA_EXPORT GAErrorCode gameAnalytics_getRemoteConfigsContentAsString(char* out, int* size);

GA_EXPORT GAErrorCode gameAnalytics_getABTestingId(char* out, int* size);
GA_EXPORT GAErrorCode gameAnalytics_getABTestingVariantId(char* out, int* size);

#ifdef __cplusplus
}
#endif

#endif // _GA_EXTERNAL_H_
