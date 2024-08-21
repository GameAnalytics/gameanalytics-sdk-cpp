#ifdef __cplusplus
extern "C" {
#endif

#if GA_SHARED_LIB
	#if defined(_WIN32)
		#define EXPORT __declspec(dllexport)
	#else
		#define EXPORT __attribute__((visibility("default")))
	#endif
#else
	#define EXPORT
#endif

struct GAString_;
EXPORT void ga_string_free(GAString_* s);

EXPORT struct GAString_
{
    const char* str = 0;
    unsigned int size = 0;
    
#ifdef __cplusplus
    ~GAString_()
	{
		ga_string_free(this);
	}
#endif
};

typedef struct GAString_ GAString;

EXPORT void configureAvailableCustomDimensions01(const char *customDimensionsJson);
EXPORT void configureAvailableCustomDimensions02(const char *customDimensionsJson);
EXPORT void configureAvailableCustomDimensions03(const char *customDimensionsJson);
EXPORT void configureAvailableResourceCurrencies(const char *resourceCurrenciesJson);
EXPORT void configureAvailableResourceItemTypes(const char *resourceItemTypesJson);
EXPORT void configureBuild(const char *build);
EXPORT void configureWritablePath(const char *writablePath);
EXPORT void configureDeviceModel(const char *deviceModel);
EXPORT void configureDeviceManufacturer(const char *deviceManufacturer);

// the version of SDK code used in an engine. Used for sdk_version field.
// !! if set then it will override the SdkWrapperVersion.
// example "unity 4.6.9"
EXPORT void configureSdkGameEngineVersion(const char *sdkGameEngineVersion);
// the version of the game engine (if used and version is available)
EXPORT void configureGameEngineVersion(const char *engineVersion);

EXPORT void configureUserId(const char *uId);

EXPORT void configureExternalUserId(const char* extId);

// initialize - starting SDK (need configuration before starting)
EXPORT void initialize(const char *gameKey, const char *gameSecret);

// add events
EXPORT void addBusinessEvent(const char *currency, double amount, const char *itemType, const char *itemId, const char *cartType, const char *customFields, double mergeFields);

EXPORT void addResourceEvent(double flowType, const char *currency, double amount, const char *itemType, const char *itemId, const char *customFields, double mergeFields);

EXPORT void addProgressionEvent(double progressionStatus, const char *progression01, const char *progression02, const char *progression03, const char *customFields, double mergeFields);

EXPORT void addProgressionEventWithScore(double progressionStatus, const char *progression01, const char *progression02, const char *progression03, double score, const char *customFields, double mergeFields);

EXPORT void addDesignEvent(const char *eventId, const char *customFields, double mergeFields);
EXPORT void addDesignEventWithValue(const char *eventId, double value, const char *customFields, double mergeFields);
EXPORT void addErrorEvent(double severity, const char *message, const char *customFields, double mergeFields);

// set calls can be changed at any time (pre- and post-initialize)
// some calls only work after a configure is called (setCustomDimension)
EXPORT void setEnabledInfoLog(double flag);
EXPORT void setEnabledVerboseLog(double flag);
EXPORT void setEnabledManualSessionHandling(double flag);
EXPORT void setEnabledErrorReporting(double flag);
EXPORT void setEnabledEventSubmission(double flag);
EXPORT void setCustomDimension01(const char *dimension01);
EXPORT void setCustomDimension02(const char *dimension02);
EXPORT void setCustomDimension03(const char *dimension03);

EXPORT void setGlobalCustomEventFields(const char *customFields);

EXPORT void gameAnalyticsStartSession();
EXPORT void gameAnalyticsEndSession();

// game state changes
// will affect how session is started / ended
EXPORT void onResume();
EXPORT void onSuspend();
EXPORT void onQuit();

EXPORT GAString getRemoteConfigsValueAsString(const char *key);
EXPORT GAString getRemoteConfigsValueAsStringWithDefaultValue(const char *key, const char *defaultValue);
EXPORT double isRemoteConfigsReady();
EXPORT GAString getRemoteConfigsContentAsString();

EXPORT GAString getABTestingId();
EXPORT GAString getABTestingVariantId();

#ifdef __cplusplus
}
#endif
