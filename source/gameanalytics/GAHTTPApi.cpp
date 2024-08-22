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

namespace gameanalytics
{
    namespace http
    {
        size_t writefunc(void *ptr, size_t size, size_t nmemb, ResponseData *s)
        {
            const size_t new_len = s->len + size*nmemb;

            if (s->ptr)
            {
                std::unique_ptr<char[]> buffer = std::make_unique<char[]>(new_len);

                if (!buffer) // maybe just return instead?
                    throw std::runtime_error("Failed to allocate buffer!");

                std::memcpy(buffer.get(), s->ptr.get(), s->len);
                s->ptr = std::move(buffer);
            }
            else
            {
                s->ptr = std::make_unique<char[]>(new_len);
            }

            std::memcpy(s->ptr.get() + s->len, ptr, size * nmemb);
            s->ptr[new_len] = '\0';
            s->len = new_len;

            return size*nmemb;
        }

        // Constructor - setup the basic information for HTTP
        GAHTTPApi::GAHTTPApi()
        {
            static int i = 0;
            std::cerr << "ctor called: " << i << '\n';
            ++i;
            
            curl_global_init(CURL_GLOBAL_DEFAULT);

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
            curl_global_cleanup();
        }

        GAHTTPApi& GAHTTPApi::getInstance()
        {
            return state::GAState::getInstance()._gaHttp;
        }

        EGAHTTPApiResponse GAHTTPApi::requestInitReturningDict(json& json_out, std::string const& configsHash)
        {
            std::string gameKey = state::GAState::getGameKey();

            // Generate URL
            std::string url = remoteConfigsBaseUrl + "/" + initializeUrlPath + "?game_key=" + gameKey + "&interval_seconds=0&configs_hash=" + configsHash;

            logging::GALogger::d("Sending 'init' URL: %s", url.c_str());

            json initAnnotations;
            state::GAState::getInitAnnotations(initAnnotations);
            
            try
            {
                std::string jsonString = initAnnotations.dump();
                if (jsonString.empty())
                {
                    return; JsonEncodeFailed;
                }

                std::vector<uint8_t> payloadData = createPayloadData(jsonString, useGzip);

                CURL* curl = nullptr;
                CURLcode res;
                curl = curl_easy_init();
                if (!curl)
                {
                    return NoResponse;
                }

                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

                ResponseData s;

                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

                std::vector<uint8_t> authorization = createRequest(curl, url, payloadData, useGzip);

                res = curl_easy_perform(curl);
                if (res != CURLE_OK)
                {
                    logging::GALogger::d(curl_easy_strerror(res));
                    return NoResponse;
                }

                long response_code{};
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                curl_easy_cleanup(curl);

                // process the response
                logging::GALogger::d("init request content: %s, json: %s", s.toString().c_str(), jsonString.c_str());

                json requestJsonDict = json::parse(s.toString());

                EGAHTTPApiResponse requestResponseEnum = processRequestResponse(response_code, s.ptr.get(), "Init");

                // if not 200 result
                if (requestResponseEnum != Ok && requestResponseEnum != Created && requestResponseEnum != BadRequest)
                {
                    logging::GALogger::d("Failed Init Call. URL: %s, JSONString: %s, Authorization: %s", url.c_str(), jsonString.c_str(), authorization.data());

                    return requestResponseEnum;
                }

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

        EGAHTTPApiResponse GAHTTPApi::sendEventsInArray(json& json_out, const json& eventArray)
        {
            if (eventArray.empty())
            {
                logging::GALogger::d("sendEventsInArray called with missing eventArray");
                return JsonEncodeFailed;
            }

            std::string gameKey = state::GAState::getGameKey();

            try
            {
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

                CURL* curl = nullptr;
                CURLcode res{};
                curl = curl_easy_init();
                if (!curl)
                {
                    return NoResponse;
                }

                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

                ResponseData s = {};

                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

                std::vector<uint8_t> authorization = createRequest(curl, url, payloadData, useGzip);

                res = curl_easy_perform(curl);
                if (res != CURLE_OK)
                {
                    logging::GALogger::d(curl_easy_strerror(res));
                    return NoResponse;
                }

                long response_code{};
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                curl_easy_cleanup(curl);

                logging::GALogger::d("body: %s", s.toString().c_str());

                EGAHTTPApiResponse requestResponseEnum = processRequestResponse(response_code, s.ptr.get(), "Events");

                // if not 200 result
                if (requestResponseEnum != Ok && requestResponseEnum != Created && requestResponseEnum != BadRequest)
                {
                    logging::GALogger::d("Failed Events Call. URL: %s, JSONString: %s, Authorization: %s", url.c_str(), jsonString.c_str(), authorization.data());
                    return requestResponseEnum;
                }

                // decode JSON
                json requestJsonDict = json::parse(s.toString());
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

            json eventArray;
            eventArray.push_back(jsonObject);

            std::string payloadJSONString = eventArray.dump();
            if(payloadJSONString.empty())
            {
                logging::GALogger::w("sendSdkErrorEvent: JSON encoding failed.");
                return;
            }

            logging::GALogger::d("sendSdkErrorEvent json: %s", payloadJSONString.c_str());

#if !NO_ASYNC
            ErrorType errorType = std::make_tuple(category, area);

            bool useGzip = this->useGzip;

            std::async(std::launch::async, [=]() -> void
            {
                int64_t now = utilities::getTimestamp();
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

                CURL *curl = nullptr;
                CURLcode res;
                curl = curl_easy_init();
                if(!curl)
                {
                    return;
                }

                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

                ResponseData s = {};

                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

                getInstance().createRequest(curl, url.data(), payloadData, useGzip);

                res = curl_easy_perform(curl);
                if(res != CURLE_OK)
                {
                    logging::GALogger::d(curl_easy_strerror(res));
                    return;
                }

                long statusCode{};
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
                curl_easy_cleanup(curl);

                // process the response
                logging::GALogger::d("sdk error content : %s", s.toString().c_str());;

                // if not 200 result
                if (statusCode != 200)
                {
                    logging::GALogger::d("sdk error failed. response code not 200. status code: %u", CURLE_OK);
                    return;
                }

                countMap[errorType] = countMap[errorType] + 1;
            });
#endif
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

        std::vector<uint8_t> GAHTTPApi::createRequest(CURL *curl, std::string const& url, const std::vector<uint8_t>& payloadData, bool gzip)
        {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            struct curl_slist *header = NULL;

            if (gzip)
            {
                header = curl_slist_append(header, "Content-Encoding: gzip");
            }

            // create authorization hash
            std::string const key = state::GAState::getGameSecret();

            std::vector<uint8_t> authorization;
            utilities::GAUtilities::hmacWithKey(key.c_str(), payloadData, authorization);
            std::string auth = "Authorization: " + std::string(reinterpret_cast<char*>(authorization.data()), authorization.size());

            header = curl_slist_append(header, auth.c_str());

            // always JSON
            header = curl_slist_append(header, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadData.data());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payloadData.size());

            return authorization;
        }

        EGAHTTPApiResponse GAHTTPApi::processRequestResponse(long statusCode, const char* body, const char* requestId)
        {
            // if no result - often no connection
            if (utilities::GAUtilities::isStringNullOrEmpty(body))
            {
                logging::GALogger::d("%s request. failed. Might be no connection. Status code: %ld", requestId, statusCode);
                return NoResponse;
            }

            // ok
            if (statusCode == 200)
            {
                return Ok;
            }
            if (statusCode == 201)
            {
                return Created;
            }

            // 401 can return 0 status
            if (statusCode == 0 || statusCode == 401)
            {
                logging::GALogger::d("%s request. 401 - Unauthorized.", requestId);
                return Unauthorized;
            }

            if (statusCode == 400)
            {
                logging::GALogger::d("%s request. 400 - Bad Request.", requestId);
                return BadRequest;
            }

            if (statusCode == 500)
            {
                logging::GALogger::d("%s request. 500 - Internal Server Error.", requestId);
                return InternalServerError;
            }
            return UnknownResponseCode;
        }

        std::string ResponseData::toString() const
        {
            std::string str;
            if (ptr && len)
            {
                str = std::string(ptr.get(), len);
            }

            return str;
        }
}
}
