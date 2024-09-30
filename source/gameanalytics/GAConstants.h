#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(GA_UWP_BUILD)

    #ifndef GA_UWP_BUILD
        #define IS_WIN32 1
        #define IS_UWP 0
    #else
        #define IS_WIN32 0
        #define IS_UWP 1
    #endif

    #define _WIN32_DCOM

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

#else
    #define IS_WIN32 0
    #define IS_UWP   0
#endif

#if defined(__linux__) || defined(__unix__) || defined(__unix) || defined(unix)
    #define IS_LINUX 1
#else
    #define IS_LINUX 0
#endif

#if defined(__MACH__) || defined(__APPLE__)
    #define IS_MAC 1
#else
    #define IS_MAC 0
#endif

namespace gameanalytics
{
    constexpr const char* GA_VERSION_STR = "cpp 4.0.0-alpha";

    constexpr int MAX_CUSTOM_FIELDS_COUNT				 = 50;
    constexpr int MAX_CUSTOM_FIELDS_KEY_LENGTH			 = 64;
    constexpr int MAX_CUSTOM_FIELDS_VALUE_STRING_LENGTH  = 256;

    constexpr int UUID_STR_LENGTH		= 128;
    constexpr int TEXT_BUFFER_LENGTH	= 256;

    constexpr const char* UNKNOWN_VALUE = "unknown";

    constexpr int MAX_ERROR_TYPE_COUNT = 10;
    constexpr int MAX_ERROR_MSG_LEN	= 8192;

    constexpr int JSON_PRINT_INDENT = 4;

    constexpr const char* CONNECTION_OFFLINE = "offline";
    constexpr const char* CONNECTION_LAN = "lan";
    constexpr const char* CONNECTION_WIFI = "wifi";
    constexpr const char* CONNECTION_WWAN = "wwan";

    /*!
     @enum
     @discussion
     This enum is used to specify flow in resource events
     @constant GAResourceFlowTypeSource
     Used when adding to a resource currency
     @constant GAResourceFlowTypeSink
     Used when subtracting from a resource currency
     */
    enum EGAResourceFlowType
    {
        Source = 1,
        Sink = 2
    };

    /*!
     @enum
     @discussion
     this enum is used to specify status for progression event
     @constant GAProgressionStatusStart
     User started progression
     @constant GAProgressionStatusComplete
     User succesfully ended a progression
     @constant GAProgressionStatusFail
     User failed a progression
     */
    enum EGAProgressionStatus
    {
        Start = 1,
        Complete = 2,
        Fail = 3
    };

    /*!
     @enum
     @discussion
     this enum is used to specify severity of an error event
     @constant GAErrorSeverityDebug
     @constant GAErrorSeverityInfo
     @constant GAErrorSeverityWarning
     @constant GAErrorSeverityError
     @constant GAErrorSeverityCritical
     */
    enum EGAErrorSeverity
    {
        Debug       = 1,
        Info        = 2,
        Warning     = 3,
        Error       = 4,
        Critical    = 5
    };

    enum EGALoggerMessageType
    {
        LogError    = 0,
        LogWarning  = 1,
        LogInfo     = 2,
        LogDebug    = 3,
        LogVerbose  = 4
    };
}
