//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#include "GACommon.h"
#include "GAHTTPApi.h"
#include "GAState.h"
#include "GALogger.h"
#include "GAUtilities.h"
#include "GAValidator.h"

#include "Http/GAHttpCurl.h"

namespace gameanalytics
{
    namespace http
    {
        constexpr int HTTP_RESPONSE_OK = 200;
        constexpr int HTTP_RESPONSE_CREATED = 201;
        constexpr int HTTP_RESPONSE_NO_CONTENT = 204;
        constexpr int HTTP_RESPONSE_BAD_REQUEST = 400;
        constexpr int HTTP_RESPONSE_UNAUTHORIZED = 401;
        constexpr int HTTP_RESPONSE_INTERNAL_ERROR = 500;

        std::unique_ptr<GAHttpClient> GAHTTPApi::pendingCustomImpl = nullptr;

        // Constructor - setup the basic information for HTTP
        GAHTTPApi::GAHTTPApi():
            impl(pendingCustomImpl ? std::move(pendingCustomImpl) : std::make_unique<GAHttpCurl>())
        {
            if(impl)
            {
                impl->initialize();
            }

            baseUrl              = protocol + "://" + hostName + "/" + version;
            remoteConfigsBaseUrl = protocol + "://" + hostName + "/remote_configs/" + remoteConfigsVersion;

            // use gzip compression on JSON body
#if defined(_DEBUG)
            useGzip = false;
#else
            useGzip = true;
#endif
        }

        GAHTTPApi::~GAHTTPApi()
        {
            if(impl)
            {
                impl->cleanup();
            }
        }

        GAHTTPApi& GAHTTPApi::getInstance()
        {
            return state::GAState::getInstance()._gaHttp;
        }

        void GAHTTPApi::setCustomHttpImpl(std::unique_ptr<GAHttpClient> customImpl)
        {
            pendingCustomImpl = std::move(customImpl);
        }

        EGAHTTPApiResponse GAHTTPApi::requestInitReturningDict(json& json_out, std::string const& configsHash)
        {
            if(!impl)
            {
                logging::GALogger::e("Invalid http implmentation");
                return SdkError;
            }

            std::string gameKey = state::GAState::getGameKey();

            // Generate URL
            std::string url = remoteConfigsBaseUrl + "/" + initializeUrlPath + "?game_key=" + gameKey + "&interval_seconds=0&configs_hash=" + configsHash + "&config_vsn_supported=3";

            logging::GALogger::d("Sending 'init' URL: %s", url.c_str());

            json initAnnotations;
            state::GAState::getInitAnnotations(initAnnotations);
            
            try
            {
                std::string jsonString = initAnnotations.dump();
                if (jsonString.empty())
                {
                    return JsonEncodeFailed;
                }

                std::vector<uint8_t> payloadData = createPayloadData(jsonString, useGzip);

                std::string const auth = createAuth(payloadData);
                GAHttpClient::Response response = impl->sendRequest(url, auth, payloadData, useGzip, nullptr);

                if(response.code < 0)
                {
                    logging::GALogger::e("Request failed: %s", url.c_str());
                    return EGAHTTPApiResponse::NoResponse;
                }

                std::string_view content = response.toString();

                // process the response
                logging::GALogger::d("init request content: %.*s, json: %s", (int)content.size(), content.data(), jsonString.c_str());

                EGAHTTPApiResponse requestResponseEnum = processRequestResponse(response, "Init");

                // if not 200 result
                if (requestResponseEnum != Ok && requestResponseEnum != Created && requestResponseEnum != BadRequest)
                {
                    logging::GALogger::d("Failed Init Call. URL: %s, JSONString: %s, Authorization: %s", url.c_str(), jsonString.c_str(), auth.c_str());

                    return requestResponseEnum;
                }

                json requestJsonDict = json::parse(content);
                if (requestJsonDict.is_null())
                {
                    logging::GALogger::d("Failed Init Call. Json decoding failed");
                    return JsonDecodeFailed;
                }

                // print reason if bad request
                if (requestResponseEnum == BadRequest)
                {
                    logging::GALogger::d("Failed Init Call. Bad request. Response: %s", requestJsonDict.dump().c_str());

                    // return bad request result
                    return requestResponseEnum;
                }

                // validate Init call values
                validators::GAValidator::validateAndCleanInitRequestResponse(requestJsonDict, json_out, requestResponseEnum == Created);

                if (json_out.is_null())
                {
                    return BadResponse;
                }

                // all ok
                return requestResponseEnum;
            }
            catch (json::exception& e)
            {
                logging::GALogger::e("Failed to parse json: %s", e.what());
                return InternalError;
            }
            catch (std::exception& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
                return InternalError;
            }
        }

        std::string GAHTTPApi::createAuth(std::vector<uint8_t> const& payloadData)
        {        
            const std::string key = state::GAState::getGameSecret();

            std::vector<uint8_t> authorization;
            utilities::GAUtilities::hmacWithKey(key.c_str(), payloadData, authorization);
            std::string auth = "Authorization: " + std::string(reinterpret_cast<char*>(authorization.data()), authorization.size());

            return auth;
        }

        EGAHTTPApiResponse GAHTTPApi::sendEventsInArray(json& json_out, const json& eventArray)
        {
            if(!impl)
            {
                logging::GALogger::e("Invalid http implmentation");
                return SdkError;
            }

            if (eventArray.empty())
            {
                logging::GALogger::d("sendEventsInArray called with missing eventArray");
                return JsonEncodeFailed;
            }

            try
            {
                const std::string gameKey = state::GAState::getGameKey();

                // Generate URL
                const std::string url = baseUrl + '/' + gameKey + '/' + eventsUrlPath;
                logging::GALogger::d("Sending 'events' URL: %s", url.c_str());

                std::string const jsonString = eventArray.dump();
                if (jsonString.empty())
                {
                    logging::GALogger::d("sendEventsInArray JSON encoding failed of eventArray");
                    return JsonEncodeFailed;
                }

                std::vector<uint8_t> payloadData = createPayloadData(jsonString, useGzip);

                std::string const auth = createAuth(payloadData);
                GAHttpClient::Response response = impl->sendRequest(url, auth, payloadData, useGzip, nullptr);

                if(response.code < 0)
                {
                    logging::GALogger::e("Request failed: %s", url.c_str());
                    return EGAHTTPApiResponse::NoResponse;
                }

                std::string_view content = response.toString();
                logging::GALogger::d("body: %.*s", (int)content.size(), content.data());

                EGAHTTPApiResponse requestResponseEnum = processRequestResponse(response, "Events");

                const bool isValidResponse = 
                    requestResponseEnum == Ok || requestResponseEnum == Created || requestResponseEnum == NoContent;

                // if not 200 result
                if (!isValidResponse && requestResponseEnum != BadRequest)
                {
                    logging::GALogger::d("Failed Events Call. URL: %s, JSONString: %s, Authorization: %s", url.c_str(), jsonString.c_str(), auth.c_str());
                    return requestResponseEnum;
                }

                if(requestResponseEnum == NoContent)
                {
                    return requestResponseEnum;
                }

                // decode JSON
                json requestJsonDict = json::parse(response.toString());
                if (requestJsonDict.is_null())
                {
                    return JsonDecodeFailed;
                }

                // print reason if bad request
                if (requestResponseEnum == BadRequest)
                {
                    logging::GALogger::d("Failed Events Call. Bad request. Response: %s", 
                        requestJsonDict.dump(JSON_PRINT_INDENT).c_str());

                    return requestResponseEnum;
                }

                json_out.merge_patch(requestJsonDict);

                // return response
                return requestResponseEnum;
            }
            catch (json::exception& e)
            {
                logging::GALogger::e("Json exception: %s", e.what());
                return JsonDecodeFailed;
            }
            catch (std::exception& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
                return InternalError;
            }
        }

        void GAHTTPApi::sendSdkErrorEvent(EGASdkErrorCategory category, EGASdkErrorArea area, EGASdkErrorAction action, EGASdkErrorParameter parameter, std::string const& reason, std::string const& gameKey, const std::string& secretKey)
        {
            if(!impl)
            {
                logging::GALogger::e("Invalid http implmentation");
                return;
            }

            if(!state::GAState::isEventSubmissionEnabled())
            {
                return;
            }

            // Validate
            if (!validators::GAValidator::validateSdkErrorEvent(gameKey, secretKey, category, area, action))
            {
                return;
            }

            // Generate URL
            const std::string url = baseUrl + "/" + gameKey + "/" + eventsUrlPath;

            logging::GALogger::d("Sending 'events' URL: %s", url.c_str());

            json jsonObject;
            state::GAState::getSdkErrorEventAnnotations(jsonObject);

            jsonObject["error_category"] = sdkErrorCategoryString(category);
            jsonObject["error_area"]     = sdkErrorAreaString(area);
            jsonObject["error_action"]   = sdkErrorActionString(action);
            
            utilities::addIfNotEmpty(jsonObject, "error_parameter", sdkErrorParameterString(parameter));
            utilities::addIfNotEmpty(jsonObject, "reason", reason);

            json eventArray = json::array();
            eventArray.push_back(jsonObject);

            std::string payloadJSONString = eventArray.dump();
            if(payloadJSONString.empty())
            {
                logging::GALogger::w("sendSdkErrorEvent: JSON encoding failed.");
                return;
            }

            logging::GALogger::d("sendSdkErrorEvent json: %s", payloadJSONString.c_str());

            ErrorType errorType = std::make_tuple(category, area);

            bool useGzip = this->useGzip;

            auto task = std::async(std::launch::async, [=]() -> void
            {
                int64_t now = utilities::GAUtilities::timeIntervalSince1970();
                if(timestampMap.count(errorType) == 0)
                {
                    timestampMap[errorType] = now;
                }
                if(countMap.count(errorType) == 0)
                {
                    countMap[errorType] = 0;
                }
                
                constexpr int64_t FREQUENCY = 3600; // 1h

                int64_t diff = now - timestampMap[errorType];
                if(diff >= FREQUENCY)
                {
                    countMap[errorType] = 0;
                    timestampMap[errorType] = now;
                }

                if(countMap[errorType] >= MaxCount)
                {
                    return;
                }

                std::vector<uint8_t> payloadData = getInstance().createPayloadData(payloadJSONString, useGzip);

                std::string auth = createAuth(payloadData);
                GAHttpClient::Response response = impl->sendRequest(url, auth, payloadData, useGzip, nullptr);

                if(response.code < 0)
                {
                    logging::GALogger::e("Request failed: %s", url.c_str());
                    return;
                }

                std::string_view content = response.toString();

                // process the response
                logging::GALogger::d("sdk error content : %.*s", (int)content.size(), content.data());

                // if not 200 result
                if (response.code != HTTP_RESPONSE_OK && response.code != HTTP_RESPONSE_NO_CONTENT)
                {
                    logging::GALogger::d("sdk error failed. response code not 200 or 204. status code: %ld", response.code);
                    return;
                }

                countMap[errorType] = countMap[errorType] + 1;
            });
        }

        std::vector<uint8_t> GAHTTPApi::createPayloadData(std::string const& payload, bool gzip)
        {
            if (payload.empty())
            {
                return {};
            }

            std::vector<uint8_t> payloadData;

            if (gzip)
            {
                payloadData = utilities::GAUtilities::gzipCompress(payload.c_str());
                logging::GALogger::d("Gzip stats. Size: %lu, Compressed: %lu", payload.size(), payloadData.size());
            }
            else
            {
                payloadData = std::vector<uint8_t>(payload.begin(), payload.end());
            }

            return payloadData;
        }

        EGAHTTPApiResponse GAHTTPApi::processRequestResponse(GAHttpClient::Response const& response, std::string const& requestId)
        {
            // if no result - often no connection
            if (response.packet.empty() && response.code != HTTP_RESPONSE_NO_CONTENT)
            {
                logging::GALogger::d("%s request. failed. Might be no connection. Status code: %ld", requestId.c_str(), response.code);
                return NoResponse;
            }

            // ok
            if (response.code == HTTP_RESPONSE_OK)
            {
                return Ok;
            }
            if (response.code == HTTP_RESPONSE_CREATED)
            {
                return Created;
            }
            if(response.code == HTTP_RESPONSE_NO_CONTENT)
            {
                return NoContent;
            }

            // 401 can return 0 status
            if (response.code == 0 || response.code == HTTP_RESPONSE_UNAUTHORIZED)
            {
                logging::GALogger::d("%s request. 401 - Unauthorized.", requestId.c_str());
                return Unauthorized;
            }

            if (response.code == HTTP_RESPONSE_BAD_REQUEST)
            {
                logging::GALogger::d("%s request. 400 - Bad Request.", requestId.c_str());
                return BadRequest;
            }

            if (response.code == HTTP_RESPONSE_INTERNAL_ERROR)
            {
                logging::GALogger::d("%s request. 500 - Internal Server Error.", requestId.c_str());
                return InternalServerError;
            }

            return UnknownResponseCode;
        }
}
}
