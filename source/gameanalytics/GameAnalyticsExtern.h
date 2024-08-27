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

typedef struct GAString_
{
    const char* str = 0;
    unsigned int size = 0;

} GAString;

#ifdef __cplusplus
	#define GA_BOOL  bool
	#define GA_TRUE  true
	#define GA_FALSE false
#else
	#define GA_BOOL  char
	#define GA_TRUE  ((char)1)
	#define GA_FALSE ((char)0)
#endif

EXPORT void ga_string_alloc(GAString* s, unsigned int size);
EXPORT void ga_string_free(GAString* s);

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
EXPORT void addBusinessEvent(const char *currency, double amount, const char *itemType, const char *itemId, const char *cartType, const char *customFields, GA_BOOL mergeFields);

EXPORT void addResourceEvent(int flowType, const char *currency, double amount, const char *itemType, const char *itemId, const char *customFields, GA_BOOL mergeFields);

EXPORT void addProgressionEvent(int progressionStatus, const char *progression01, const char *progression02, const char *progression03, const char *customFields, GA_BOOL mergeFields);

EXPORT void addProgressionEventWithScore(int progressionStatus, const char *progression01, const char *progression02, const char *progression03, double score, const char *customFields, GA_BOOL mergeFields);

EXPORT void addDesignEvent(const char *eventId, const char *customFields, GA_BOOL mergeFields);
EXPORT void addDesignEventWithValue(const char *eventId, double value, const char *customFields, GA_BOOL mergeFields);
EXPORT void addErrorEvent(int severity, const char *message, const char *customFields, GA_BOOL mergeFields);

// set calls can be changed at any time (pre- and post-initialize)
// some calls only work after a configure is called (setCustomDimension)
EXPORT void setEnabledInfoLog(GA_BOOL flag);
EXPORT void setEnabledVerboseLog(GA_BOOL flag);
EXPORT void setEnabledManualSessionHandling(GA_BOOL flag);
EXPORT void setEnabledErrorReporting(GA_BOOL flag);
EXPORT void setEnabledEventSubmission(GA_BOOL flag);
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
EXPORT GA_BOOL isRemoteConfigsReady();
EXPORT GAString getRemoteConfigsContentAsString();

EXPORT GAString getABTestingId();
EXPORT GAString getABTestingVariantId();

#ifdef __cplusplus
}
#endif
