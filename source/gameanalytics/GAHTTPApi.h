//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include "GACommon.h"
#include <curl/curl.h>

#include <vector>
#include <map>
#include <mutex>
#include <cstdlib>
#include <tuple>

namespace gameanalytics
{
    namespace http
    {

        enum EGAHTTPApiResponse
        {
            // client
            NoResponse = 0,
            BadResponse = 1,
            RequestTimeout = 2, // 408
            JsonEncodeFailed = 3,
            JsonDecodeFailed = 4,
            // server
            InternalServerError = 5,
            BadRequest = 6, // 400
            Unauthorized = 7, // 401
            UnknownResponseCode = 8,
            Ok = 9,
            Created = 10,
            InternalError
        };

        enum EGASdkErrorCategory
        {
            EventValidation = 1,
            Database = 2,
            Init = 3,
            Http = 4,
            Json = 5
        };

        enum EGASdkErrorArea
        {
            BusinessEvent = 1,
            ResourceEvent = 2,
            ProgressionEvent = 3,
            DesignEvent = 4,
            ErrorEvent = 5,
            InitHttp = 9,
            EventsHttp = 10,
            ProcessEvents = 11,
            AddEventsToStore = 12
        };

        enum EGASdkErrorAction
        {
            InvalidCurrency = 1,
            InvalidShortString = 2,
            InvalidEventPartLength = 3,
            InvalidEventPartCharacters = 4,
            InvalidStore = 5,
            InvalidFlowType = 6,
            StringEmptyOrNull = 7,
            NotFoundInAvailableCurrencies = 8,
            InvalidAmount = 9,
            NotFoundInAvailableItemTypes = 10,
            WrongProgressionOrder = 11,
            InvalidEventIdLength = 12,
            InvalidEventIdCharacters = 13,
            InvalidProgressionStatus = 15,
            InvalidSeverity = 16,
            InvalidLongString = 17,
            DatabaseTooLarge = 18,
            DatabaseOpenOrCreate = 19,
            JsonError = 25,
            FailHttpJsonDecode = 29,
            FailHttpJsonEncode = 30
        };

        enum EGASdkErrorParameter
        {
            Currency = 1,
            CartType = 2,
            ItemType = 3,
            ItemId = 4,
            Store = 5,
            FlowType = 6,
            Amount = 7,
            Progression01 = 8,
            Progression02 = 9,
            Progression03 = 10,
            EventId = 11,
            ProgressionStatus = 12,
            Severity = 13,
            Message = 14
        };

        struct ResponseData
        {
            std::vector<char> packet;
            std::string toString() const;
        };

        typedef std::tuple<EGASdkErrorCategory, EGASdkErrorArea> ErrorType;

        class GAHTTPApi
        {
            friend class state::GAState;
            
            static constexpr const char* PROTOCOL               = "https";
            static constexpr const char* HOST_NAME              = "api.gameanalytics.com";
            static constexpr const char* VERSION                = "v2";
            static constexpr const char* REMOTE_CONFIG_VERSION  = "v1";
            static constexpr const char* INIT_URL_PATH          = "init";
            static constexpr const char* EVENT_URL_PATH         = "events";

        public:

            static constexpr const char* sdkErrorCategoryString(EGASdkErrorCategory value);
            static constexpr const char* sdkErrorAreaString(EGASdkErrorArea value);
            static constexpr const char* sdkErrorActionString(EGASdkErrorAction value);
            static constexpr const char* sdkErrorParameterString(EGASdkErrorParameter value);

            static GAHTTPApi& getInstance();

            EGAHTTPApiResponse requestInitReturningDict(json& json_out, std::string const& configsHash);
            EGAHTTPApiResponse sendEventsInArray(json& json_out, const json& eventArray);
            void sendSdkErrorEvent(EGASdkErrorCategory category, EGASdkErrorArea area, EGASdkErrorAction action, EGASdkErrorParameter parameter, std::string const& reason, std::string const& gameKey, std::string const& secretKey);            

        private:

            GAHTTPApi();
            ~GAHTTPApi();
            GAHTTPApi(const GAHTTPApi&) = delete;
            GAHTTPApi& operator=(const GAHTTPApi&) = delete;
            std::vector<uint8_t> createPayloadData(std::string const& payload, bool gzip);

            std::vector<uint8_t> createRequest(CURL *curl, std::string const& url, const std::vector<uint8_t>& payloadData, bool gzip);
            EGAHTTPApiResponse processRequestResponse(long statusCode, const char* body, const char* requestId);

            std::string protocol                = PROTOCOL;
            std::string hostName                = HOST_NAME;
            std::string version                 = VERSION;
            std::string remoteConfigsVersion    = REMOTE_CONFIG_VERSION;

            std::string initializeUrlPath = INIT_URL_PATH;
            std::string eventsUrlPath     = EVENT_URL_PATH;

            std::string baseUrl;
            std::string remoteConfigsBaseUrl;

            bool useGzip;
            
            static constexpr int MaxCount = 10;
            std::map<ErrorType, int> countMap;
            std::map<ErrorType, int64_t> timestampMap;

#if USE_UWP && defined(USE_UWP_HTTP)
            Windows::Web::Http::HttpClient^ httpClient;
#endif
        };

#if USE_UWP && defined(USE_UWP_HTTP)
        ref class GANetworkStatus sealed
        {
        internal:
            static void NetworkInformationOnNetworkStatusChanged(Platform::Object^ sender);
            static void CheckInternetAccess();
            static bool hasInternetAccess;
        };
#endif

        constexpr const char* GAHTTPApi::sdkErrorCategoryString(EGASdkErrorCategory value)
        {
            switch (value)
            {
                case EventValidation:
                    return "event_validation";

                case Database:
                    return "db";

                case Init:
                    return "init";

                case Http:
                    return "http";

                case Json:
                    return "json";

                default:
                    return "";
            }
        }

        constexpr const char* GAHTTPApi::sdkErrorAreaString(EGASdkErrorArea value)
        {
            switch (value)
            {
                case BusinessEvent:
                    return "business";

                case ResourceEvent:
                    return "resource";

                case ProgressionEvent:
                    return "progression";

                case DesignEvent:
                    return "design";

                case ErrorEvent:
                    return "error";

                case InitHttp:
                    return "init_http";

                case EventsHttp:
                    return "events_http";

                case ProcessEvents:
                    return "process_events";

                case AddEventsToStore:
                    return "add_events_to_store";

                default:
                    return "";
            }
        }

        constexpr const char* GAHTTPApi::sdkErrorActionString(EGASdkErrorAction value)
        {
            switch (value)
            {
                case InvalidCurrency:
                    return "invalid_currency";

                case InvalidShortString:
                    return "invalid_short_string";

                case InvalidEventPartLength:
                    return "invalid_event_part_length";

                case InvalidEventPartCharacters:
                    return "invalid_event_part_characters";

                case InvalidStore:
                    return "invalid_store";

                case InvalidFlowType:
                    return "invalid_flow_type";

                case StringEmptyOrNull:
                    return "string_empty_or_null";

                case NotFoundInAvailableCurrencies:
                    return "not_found_in_available_currencies";

                case InvalidAmount:
                    return "invalid_amount";

                case NotFoundInAvailableItemTypes:
                    return "not_found_in_available_item_types";

                case WrongProgressionOrder:
                    return "wrong_progression_order";

                case InvalidEventIdLength:
                    return "invalid_event_id_length";

                case InvalidEventIdCharacters:
                    return "invalid_event_id_characters";

                case InvalidProgressionStatus:
                    return "invalid_progression_status";

                case InvalidSeverity:
                    return "invalid_severity";

                case InvalidLongString:
                    return "invalid_long_string";

                case DatabaseTooLarge:
                    return "db_too_large";

                case DatabaseOpenOrCreate:
                    return "db_open_or_create";

                case JsonError:
                    return "json_error";

                case FailHttpJsonDecode:
                    return "fail_http_json_decode";

                case FailHttpJsonEncode:
                    return "fail_http_json_encode";

                default:
                    return "";
            }
        }

        constexpr const char* GAHTTPApi::sdkErrorParameterString(EGASdkErrorParameter value)
        {
            switch (value)
            {
                case Currency:
                    return "currency";

                case CartType:
                    return "cart_type";

                case ItemType:
                    return "item_type";

                case ItemId:
                    return "item_id";

                case Store:
                    return "store";

                case FlowType:
                    return "flow_type";

                case Amount:
                    return "amount";

                case Progression01:
                    return "progression01";

                case Progression02:
                    return "progression02";

                case Progression03:
                    return "progression03";

                case EventId:
                    return "event_id";

                case ProgressionStatus:
                    return "progression_status";

                case Severity:
                    return "severity";

                case Message:
                    return "message";

                default:
                    return "";
                }
        }
    }

}
