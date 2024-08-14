//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#include "GAState.h"
#include "GAEvents.h"
#include "GAStore.h"
#include "GAUtilities.h"
#include "GAValidator.h"
#include "GAHTTPApi.h"
#include "GAThreading.h"
#include "GALogger.h"
#include "GADevice.h"
#include "GAThreading.h"
#include <utility>
#include <algorithm>
#include <array>
#include <climits>
#include <string.h>
#include <stdio.h>


namespace gameanalytics
{
    constexpr int MAX_COUNT = 10;
    namespace state
    {
        std::unique_ptr<GAState> GAState::_instance;

        GAState::GAState()
        {
        }

        GAState::~GAState()
        {
        }


        void GAState::setUserId(std::string const& id)
        {
            auto& i = getInstance();
            if(!i || id.empty())
            {
                return;
            }

            i->_userId = id;
            i->cacheIdentifier();
        }

        std::string GAState::getIdentifier()
        {
            return getInstance()->_identifier;
        }

        bool GAState::isInitialized()
        {
            return getInstance()->_initialized;
        }

        int64_t GAState::getSessionStart()
        {
            return getInstance()->_sessionStart;
        }

        int GAState::getSessionNum()
        {
            getInstance()->_sessionNum;
        }

        int GAState::getTransactionNum()
        {
            getInstance()->_transactionNum;
        }

        std::string GAState::getSessionId()
        {
            getInstance()->_sessionId;
        }

        std::string GAState::getCurrentCustomDimension01()
        {
            getInstance()->_currentCustomDimension01;
        }

        std::string GAState::getCurrentCustomDimension02()
        {
            getInstance()->_currentCustomDimension02;
        }

        std::string GAState::getCurrentCustomDimension03()
        {
            getInstance()->_currentCustomDimension03;
        }

        void GAState::getGlobalCustomEventFields(json& out)
        {
            auto& i = getInstance();
            if (!i)
            {
                return;
            }

            out = i->_currentGlobalCustomEventFields;
        }

        void GAState::setAvailableCustomDimensions01(const StringVector& availableCustomDimensions)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            // Validate
            if (!validators::GAValidator::validateCustomDimensions(availableCustomDimensions))
            {
                return;
            }
            i->_availableCustomDimensions01 = availableCustomDimensions;

            // validate current dimension values
            i->validateAndFixCurrentDimensions();

            utilities::GAUtilities::printJoinStringArray(availableCustomDimensions, "Set available custom01 dimension values: (%s)");
        }

        void GAState::setAvailableCustomDimensions02(const StringVector& availableCustomDimensions)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            // Validate
            if (!validators::GAValidator::validateCustomDimensions(availableCustomDimensions))
            {
                return;
            }
            i->_availableCustomDimensions02 = availableCustomDimensions;

            // validate current dimension values
            i->validateAndFixCurrentDimensions();

            utilities::GAUtilities::printJoinStringArray(availableCustomDimensions, "Set available custom02 dimension values: (%s)");
        }

        void GAState::setAvailableCustomDimensions03(const StringVector& availableCustomDimensions)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }
            // Validate
            if (!validators::GAValidator::validateCustomDimensions(availableCustomDimensions))
            {
                return;
            }
            i->_availableCustomDimensions03 = availableCustomDimensions;

            // validate current dimension values
            i->validateAndFixCurrentDimensions();

            utilities::GAUtilities::printJoinStringArray(availableCustomDimensions, "Set available custom03 dimension values: (%s)");
        }

        void GAState::setAvailableResourceCurrencies(const StringVector& availableResourceCurrencies)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            // Validate
            if (!validators::GAValidator::validateResourceCurrencies(availableResourceCurrencies)) {
                return;
            }
            i->_availableResourceCurrencies = availableResourceCurrencies;

            utilities::GAUtilities::printJoinStringArray(availableResourceCurrencies, "Set available resource currencies: (%s)");
        }

        void GAState::setAvailableResourceItemTypes(const StringVector& availableResourceItemTypes)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            // Validate
            if (!validators::GAValidator::validateResourceItemTypes(availableResourceItemTypes)) {
                return;
            }
            i->_availableResourceItemTypes = availableResourceItemTypes;

            utilities::GAUtilities::printJoinStringArray(availableResourceItemTypes, "Set available resource item types: (%s)");
        }

        void GAState::setBuild(std::string const& build)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            i->_build = build;
            logging::GALogger::i("Set build: %s", build.c_str());
        }

        void GAState::setDefaultUserId(std::string const& id)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            i->_defaultUserId = id;
            cacheIdentifier();
        }

        json& GAState::getSdkConfig()
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            if (i->_sdkConfig.is_object())
            {
                return _sdkConfig;
            }
            else if (i->_sdkConfigCached.is_object())
            {
                return _sdkConfigCached;
            }

            return _sdkConfigDefault;
        }

        bool GAState::isEnabled()
        {
            return getInstance()->_enabled;
        }

        void GAState::setCustomDimension01(std::string const& dimension)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            i->_currentCustomDimension01 = dimension;
            if (store::GAStore::getTableReady())
            {
                store::GAStore::setState("dimension01", dimension.c_str());
            }
            logging::GALogger::i("Set custom01 dimension value: %s", dimension.c_str());
        }

        void GAState::setCustomDimension02(std::string const& dimension)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            i->_currentCustomDimension02 = dimension;
            if (store::GAStore::getTableReady())
            {
                store::GAStore::setState("dimension02", dimension.c_str());
            }
            logging::GALogger::i("Set custom02 dimension value: %s", dimension.c_str());
        }

        void GAState::setCustomDimension03(std::string const& dimension)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            i->_currentCustomDimension03 = dimension;
            if (store::GAStore::getTableReady())
            {
                store::GAStore::setState("dimension03", dimension.c_str());
            }
            logging::GALogger::i("Set custom03 dimension value: %s", dimension.c_str());
        }

        void GAState::setGlobalCustomEventFields(std::string const& customFields)
        {
            auto& i = getInstance();
            if (!i)
            {
                return;
            }

            try
            {
                i->_currentGlobalCustomEventFields = json::parse(customFields);
                logging::GALogger::i("Set global custom event fields: %s", customFields.c_str());
            }
            catch(std::exception& e)
            {
                logging::GALogger::e(e.what());
            }
        }

        void GAState::incrementSessionNum()
        {
            getInstance()->_sessionNum++;
        }

        void GAState::incrementTransactionNum()
        {
            getInstance()->_transactionNum++;
        }

        void GAState::incrementProgressionTries(std::string const& progression)
        {
            int tries = getInstance()->_progressionTries.incrementTries(progression);

            // Persist
            std::string triesString = std::to_string(tries);

            const char* parms[] = {progression.c_str(), triesString.c_str()};
            store::GAStore::executeQuerySync("INSERT OR REPLACE INTO ga_progression (progression, tries) VALUES(?, ?);", parms, 2);
        }

        int GAState::getProgressionTries(std::string const& progression)
        {
            return getInstance()->_progressionTries.getTries(progression);
        }

        void GAState::clearProgressionTries(std::string const& progression)
        {
            getInstance()->_progressionTries.remove(progression);

            // Delete
            const char* parms[] = {progression.c_str()};
            store::GAStore::executeQuerySync("DELETE FROM ga_progression WHERE progression = ?;", parms, 1);
        }

        bool GAState::hasAvailableCustomDimensions01(std::string const& dimension1)
        {
            auto& i = getInstance();
            if(!i)
            {
                return false;
            }
            return utilities::GAUtilities::stringVectorContainsString(i->_availableCustomDimensions01, dimension1);
        }

        bool GAState::hasAvailableCustomDimensions02(std::string const& dimension2)
        {
            auto& i = getInstance();
            if(!i)
            {
                return false;
            }
            return utilities::GAUtilities::stringVectorContainsString(i->_availableCustomDimensions02, dimension2);
        }

        bool GAState::hasAvailableCustomDimensions03(std::string const& dimension3)
        {
            auto& i = getInstance();
            if(!i)
            {
                return false;
            }
            return utilities::GAUtilities::stringVectorContainsString(i->_availableCustomDimensions03, dimension3);
        }

        bool GAState::hasAvailableResourceCurrency(std::string const& currency)
        {
            auto& i = getInstance();
            if(!i)
            {
                return false;
            }
            return utilities::GAUtilities::stringVectorContainsString(i->_availableResourceCurrencies, currency);
        }

        bool GAState::hasAvailableResourceItemType(std::string const& itemType)
        {
            auto& i = getInstance();
            if(!i)
            {
                return false;
            }
            return utilities::GAUtilities::stringVectorContainsString(i->_availableResourceItemTypes, itemType);
        }

        void GAState::setKeys(std::string const& gameKey, std::string const& gameSecret)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }
            
            i->_gameKey     = gameKey;
            i->_gameSecret  = gameSecret;
        }

        std::string GAState::getGameKey()
        {
            auto& i = getInstance();
            if(!i)
            {
                return "";
            }
            return i->_gameKey;
        }

        std::string GAState::getGameSecret()
        {
            auto& i = getInstance();
            if(!i)
            {
                return "";
            }

            return i->_gameSecret;
        }

        void GAState::internalInitialize()
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            // Make sure database is ready
            if (!store::GAStore::getTableReady())
            {
                return;
            }

            // Make sure persisted states are loaded
            i->ensurePersistedStates();
            store::GAStore::setState("default_user_id", i->_defaultUserId.c_str());

            i->_initialized = true;
            i->startNewSession();

            if (isEnabled())
            {
                events::GAEvents::ensureEventQueueIsRunning();
            }
        }

        void GAState::resumeSessionAndStartQueue()
        {
            if(!GAState::isInitialized())
            {
                return;
            }
            logging::GALogger::i("Resuming session.");
            if(!GAState::sessionIsStarted())
            {
                getInstance()->startNewSession();
            }
            events::GAEvents::ensureEventQueueIsRunning();
        }

        void GAState::endSessionAndStopQueue(bool endThread)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            if(GAState::isInitialized())
            {
                logging::GALogger::i("Ending session.");
                events::GAEvents::stopEventQueue();
                if (GAState::isEnabled() && GAState::sessionIsStarted())
                {
                    events::GAEvents::addSessionEndEvent();
                    i->_sessionStart = 0;
                }
            }

            if(endThread)
            {
                threading::GAThreading::endThread();
            }
        }

        void GAState::getEventAnnotations(json& out)
        {
            try
            {

                auto& i = getInstance();
                if (!i)
                {
                    return;
                }

                // ---- REQUIRED ---- //

                // collector event API version
                out["v"] = 2;
                out["event_uuid"] = utilities::GAUtilities::generateUUID();

                // User identifier
                out["user_id"] = i->getIdentifier();

                // remote configs configurations
                out["configurations"] = i->_configurations;

                out["sdk_version"] = device::GADevice::getRelevantSdkVersion();
                out["client_ts"] = utilities::getTimestamp();
                out["os_version"] = device::GADevice::getOSVersion();
                out["manufacturer"] = device::GADevice::getDeviceManufacturer();
                out["device"] = device::GADevice::getDeviceModel();
                out["platform"] = device::GADevice::getBuildPlatform();
                out["session_id"] = i->_sessionId;
                out["session_num"] = i->_sessionNum;
                out["connection_type"] = device::GADevice::getConnectionType();

                // ---- OPTIONAL ---- //

                // A/B testing
                utilities::addIfNotEmpty(out, "ab_id", i->_abId);
                utilities::addIfNotEmpty(out, "ab_variant_id", i->_abVariantId);

                utilities::addIfNotEmpty(out, "build", i->_build);
                utilities::addIfNotEmpty(out, "engine_version", device::GADevice::getGameEngineVersion());

#if USE_UWP
                utilities::addIfNotEmpty(out, "uwp_aid", device::GADevice::getAdvertisingId());
                utilities::addIfNotEmpty(out, "uwp_id", device::GADevice::getDeviceId());
#endif
            }
            catch (json::exception& e)
            {
                logging::GALogger::e(e.what());
            }
        }

        void GAState::getSdkErrorEventAnnotations(json& out)
        {
            auto& i = getInstance();
            if (!i)
            {
                return;
            }

            // ---- REQUIRED ---- //

            // collector event API version
            out["v"] = 2;
            out["event_uuid"]       = utilities::GAUtilities::generateUUID();

            out["sdk_version"]      = device::GADevice::getRelevantSdkVersion();
            out["client_ts"]        = utilities::getTimestamp();
            out["os_version"]       = device::GADevice::getOSVersion();
            out["manufacturer"]     = device::GADevice::getDeviceManufacturer();
            out["device"]           = device::GADevice::getDeviceModel();
            out["platform"]         = device::GADevice::getBuildPlatform();
            out["connection_type"]  = device::GADevice::getConnectionType();
            out["category"]         = "sdk_error";

            // ---- OPTIONAL ---- //

            utilities::addIfNotEmpty(out, "build", i->_build);
            utilities::addIfNotEmpty(out, "engine_version", device::GADevice::getGameEngineVersion());
        }

        void GAState::getInitAnnotations(json& out)
        {
            try
            {
                auto& i = getInstance();
                if (!i)
                {
                    return;
                }

                const std::string id = i->_identifier;

                if (!id.empty())
                {
                    i->cacheIdentifier();
                    store::GAStore::setState("last_used_identifier", id);
                }

                out["user_id"]      = id;
                out["sdk_version"]  = device::GADevice::getRelevantSdkVersion();
                out["os_version"]   = device::GADevice::getOSVersion();
                out["manufacturer"] = device::GADevice::getDeviceManufacturer();
                out["device"]       = device::GADevice::getDeviceModel();
                out["platform"]     = device::GADevice::getBuildPlatform();
                out["session_id"]   = i->_sessionId;
                out["session_num"]  = out["random_salt"] = i->getSessionNum();

                utilities::addIfNotEmpty(out, "build", i->_build);
                utilities::addIfNotEmpty(out, "engine_version", device::GADevice::getGameEngineVersion());
            }
            catch (std::exception& e)
            {
                logging::GALogger::e(e.what());
            }
        }

        void GAState::cacheIdentifier()
        {
            if(_userId.empty())
            {
                _userId = device::GADevice::getAdvertisingId();
            }

            if (_userId.empty())
            {
                _userId = device::GADevice::getDeviceId();
            }

            logging::GALogger::d("identifier, {clean:%s}", _identifier.c_str());
        
        }

        std::string setStateFromCache(json& dict, std::string const& key, std::string const& value)
        {
            if (!value.empty())
            {
                store::GAStore::setState(key.c_str(), value.c_str());
                return value;
            }
            else if(dict.contains(key) && dict[key].is_string())
            {
                std::string cachedValue = dict[key].get<std::string>();
                logging::GALogger::d("%s found in cache: %s", key.c_str(), cachedValue.c_str());
                return cachedValue;
            }

            return "";
        }

        int64_t getNumberFromCache(json& dict, std::string const& key, int64_t defaultVal = 0ll)
        {
            return (dict.contains(key) && dict[key].is_number_integer()) ? 
                std::stoll(dict[key].get<std::string>()) : defaultVal;
        }

        std::string getOptionalString(json& dict, std::string const& key)
        {
            if (dict.contains(key) && dict[key].is_string())
            {
                return dict[key].get<std::string>();
            }

            return "";
        }

        void GAState::ensurePersistedStates()
        {
            try
            {
                // get and extract stored states
                json state_dict;
                json results_ga_state;

                store::GAStore::executeQuerySync("SELECT * FROM ga_state;", results_ga_state);

                if (!results_ga_state.empty())
                {
                    for (auto it = results_ga_state.begin(); it != results_ga_state.end(); ++it)
                    {
                        if (it->contains("key") && it->contains("value"))
                        {
                            state_dict[it.key()] = it.value();
                        }
                    }
                }

                // insert into GAState instance
                std::string defaultId = state_dict.contains("default_user_id") ? state_dict["default_user_id"].get<std::string>() : "";
                if (defaultId.empty())
                {
                    const std::string id = utilities::GAUtilities::generateUUID();
                    setDefaultUserId(id);
                }
                else
                {
                    setDefaultUserId(defaultId);
                }

                _sessionNum     = getNumberFromCache(state_dict, "session_num");
                _transactionNum = getNumberFromCache(state_dict, "transaction_num");

                // restore dimension settings
                _currentCustomDimension01 = setStateFromCache(state_dict, "dimension01", _currentCustomDimension01);
                _currentCustomDimension02 = setStateFromCache(state_dict, "dimension02", _currentCustomDimension02);
                _currentCustomDimension03 = setStateFromCache(state_dict, "dimension03", _currentCustomDimension03);

                // get cached init call values
                if (state_dict.contains("sdk_config_cached") && state_dict["sdk_config_cached"].is_string())
                {
                    try
                    {
                        std::string sdkConfigCachedString = state_dict["sdk_config_cached"].get<std::string>();

                        // decode JSON
                        json d = json::parse(sdkConfigCachedString);
                        if (!d.empty())
                        {
                            std::string lastUsedIdentifier = state_dict.contains("last_used_identifier") ?
                                state_dict["last_used_identifier"].get<std::string>() : "";

                            if (!lastUsedIdentifier.empty())
                            {
                                if (d.contains("configs_hash"))
                                {
                                    d.erase("configs_hash");
                                }
                            }

                            _sdkConfigCached.merge_patch(d);
                        }
                    }
                    catch (json::exception& e)
                    {
                        logging::GALogger::e(e.what());
                    }
                }

                {
                    json currentSdkConfig = getSdkConfig();

                    _configsHash = getOptionalString("configs_hash");
                    _abId        = getOptionalString("ab_id");
                    _abVariantId = getOptionalString("ab_variant_id");
                }

                json gaProgression;
                store::GAStore::executeQuerySync("SELECT * FROM ga_progression;", gaProgression);

                if (!gaProgression.empty())
                {
                    for (auto& node : gaProgression)
                    {
                        if (node.contains("progression") && node.contains("tries"))
                        {
                            std::string name = node["progression"].get<std::string>();
                            _progressionTries.addOrUpdate(name, getNumberFromCache(node["tries"]));
                        }
                    }
                }
            }
            catch (json::exception& e)
            {
                logging::GALogger::e("Failed to parse json: %s", e.what());
            }
            catch (std::exception& e)
            {
                logging::GALogger::e(e.what());
            }
        }

        void GAState::startNewSession()
        {
            try
            {
                logging::GALogger::i("Starting a new session.");

                // make sure the current custom dimensions are valid
                GAState::validateAndFixCurrentDimensions();

                // call the init call
                http::GAHTTPApi* httpApi = http::GAHTTPApi::getInstance();
                if (!httpApi)
                {
                    return;
                }

                json initResponseDict;
                http::EGAHTTPApiResponse initResponse;

                httpApi->requestInitReturningDict(initResponse, initResponseDict, _configsHash);

                // init is ok
                if ((initResponse == http::Ok || initResponse == http::Created) && !initResponseDict.empty())
                {
                    // set the time offset - how many seconds the local time is different from servertime
                    int64_t timeOffsetSeconds = 0;
                    int64_t server_ts = initResponseDict.contains("server_ts") ? initResponseDict["server_ts"].get<int64_t>() : -1ll;
                    if (server_ts > 0)
                    {
                        timeOffsetSeconds = calculateServerTimeOffset(server_ts);
                    }

                    // insert timeOffset in received init config (so it can be used when offline)
                    initResponseDict["time_offset"] = timeOffsetSeconds;

                    if (initResponse != http::Created)
                    {
                        json& currentSdkConfig = GAState::getSdkConfig();

                        // use cached if not Created
                        if (currentSdkConfig.contains("configs") && currentSdkConfig["configs"].is_array())
                        {
                            initResponseDict["configs"].merge_patch(currentSdkConfig["configs"]);
                        }
                        if (currentSdkConfig.contains("configs_hash") && currentSdkConfig["configs_hash"].is_string())
                        {
                            initResponseDict["configs_hash"] = currentSdkConfig["configs_hash"];
                        }
                        if (currentSdkConfig.contains("ab_id") && currentSdkConfig["ab_id"].is_string())
                        {
                            initResponseDict["ab_id"] = currentSdkConfig["ab_id"];
                        }
                        if (currentSdkConfig.HasMember("ab_variant_id") && currentSdkConfig["ab_variant_id"].IsString())
                        {
                            initResponseDict["ab_variant_id"] = currentSdkConfig["ab_variant_id"];
                        }
                    }

                    _configsHash = getOptionalString(initResponseDict, "configs_hash");
                    _abId        = getOptionalString(initResponseDict, "ab_id");
                    _abVariantId = getOptionalString(initResponseDict, "ab_variant_id");

                    // insert new config in sql lite cross session storage
                    store::GAStore::setState("sdk_config_cached", initResponseDict.dump());

                    // set new config and cache in memory
                    _sdkConfigCached.merge_patch(initResponseDict);
                    _sdkConfig.merge_patch(initResponseDict);

                    _initAuthorized = true;
                }
                else if (initResponse == http::Unauthorized) 
                {
                    logging::GALogger::w("Initialize SDK failed - Unauthorized");
                    _initAuthorized = false;
                }
                else
                {
                    // log the status if no connection
                    if (initResponse == http::NoResponse || initResponse == http::RequestTimeout)
                    {
                        logging::GALogger::i("Init call (session start) failed - no response. Could be offline or timeout.");
                    }
                    else if (initResponse == http::BadResponse || initResponse == http::JsonEncodeFailed || initResponse == http::JsonDecodeFailed)
                    {
                        logging::GALogger::i("Init call (session start) failed - bad response. Could be bad response from proxy or GA servers.");
                    }
                    else if (initResponse == http::BadRequest || initResponse == http::UnknownResponseCode)
                    {
                        logging::GALogger::i("Init call (session start) failed - bad request or unknown response.");
                    }

                    // init call failed (perhaps offline)
                    if (_sdkConfig.empty())
                    {
                        if (!_sdkConfigCached.empty())
                        {
                            logging::GALogger::i("Init call (session start) failed - using cached init values.");

                            // set last cross session stored config init values
                            _sdkConfig = _sdkConfigCached;
                        }
                        else
                        {
                            logging::GALogger::i("Init call (session start) failed - using default init values.");

                            // set default init values
                           _sdkConfig = _sdkConfigDefault;
                        }
                    }
                    else
                    {
                        logging::GALogger::i("Init call (session start) failed - using cached init values.");
                    }
                    _initAuthorized = true;
                }

                json& currentSdkConfig = GAState::getSdkConfig(currentSdkConfig);

                {
                    if (currentSdkConfig.is_object() && 
                        ((currentSdkConfig.contains("enabled") && currentSdkConfig["enabled"].is_boolean()) ? currentSdkConfig["enabled"].get<bool>() : true) == false)
                    {
                        _enabled = false;
                    }
                    else if (_initAuthorized)
                    {
                        _enabled = false;
                    }
                    else
                    {
                        _enabled = true;
                    }
                }

                // set offset in state (memory) from current config (config could be from cache etc.)
                _clientServerTimeOffset = utilities::getOptionalValue(currentSdkConfig, "time_offset", 0ll);

                // populate configurations
                populateConfigurations(currentSdkConfig);

                // if SDK is disabled in config
                if (!GAState::isEnabled())
                {
                    logging::GALogger::w("Could not start session: SDK is disabled.");
                    // stop event queue
                    // + make sure it's able to restart if another session detects it's enabled again
                    events::GAEvents::stopEventQueue();
                    return;
                }
                else
                {
                    events::GAEvents::ensureEventQueueIsRunning();
                }

                // generate the new session
                std::string newSessionId = utilities::GAUtilities::generateUUID();

                // Set session id
                _sessionId = utilities::toLowerCase(newSessionId);

                // Set session start
                _sessionStart = getClientTsAdjusted();

                // Add session start event
                events::GAEvents::addSessionStartEvent();
            }
            catch (json::exception& e)
            {
                logging::GALogger::e("Failed to parse json: %s", e.what());
            }
            catch (std::exception& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
        }

        void GAState::validateAndFixCurrentDimensions()
        {
            // validate that there are no current dimension01 not in list
            if (!validators::GAValidator::validateDimension01(_currentCustomDimension01))
            {
                logging::GALogger::d("Invalid dimension01 found in variable. Setting to nil. Invalid dimension: %s", _currentCustomDimension01.c_str());
                _currentCustomDimension01 = "";
            }

            // validate that there are no current dimension02 not in list
            if (!validators::GAValidator::validateDimension02(_currentCustomDimension02))
            {
                logging::GALogger::d("Invalid dimension02 found in variable. Setting to nil. Invalid dimension: %s", _currentCustomDimension02.c_str());
                _currentCustomDimension02 = "";
            }

            // validate that there are no current dimension03 not in list
            if (!validators::GAValidator::validateDimension03(_currentCustomDimension03))
            {
                logging::GALogger::d("Invalid dimension03 found in variable. Setting to nil. Invalid dimension: %s", _currentCustomDimension03.c_str());
                _currentCustomDimension03 = "";
            }
        }

        bool GAState::sessionIsStarted()
        {
            auto& i = getInstance();
            if(!i)
            {
                return false;
            }
            return i->_sessionStart != 0;
        }

        std::string GAState::getRemoteConfigsStringValue(std::string const& key, std::string const& defaultValue)
        {
            auto& i = getInstance();
            if(!i)
            {
                return defaultValue;
            }

            std::lock_guard<std::mutex> lg(i->_mtx);
            std::string value = i->_configurations.contains(key) ? i->_configurations[key].get<std::string>() : defaultValue;

            return value;
        }

        bool GAState::isRemoteConfigsReady()
        {
            auto& i = getInstance();
            if(!i)
            {
                return false;
            }
            return i->_remoteConfigsIsReady;
        }

        void GAState::addRemoteConfigsListener(const std::shared_ptr<IRemoteConfigsListener>& listener)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            if(std::find(i->_remoteConfigsListeners.begin(), i->_remoteConfigsListeners.end(), listener) == i->_remoteConfigsListeners.end())
            {
                i->_remoteConfigsListeners.push_back(listener);
            }
        }

        void GAState::removeRemoteConfigsListener(const std::shared_ptr<IRemoteConfigsListener>& listener)
        {
            auto& i = getInstance();
            if(!i)
            {
                return;
            }

            if(std::find(i->_remoteConfigsListeners.begin(), i->_remoteConfigsListeners.end(), listener) != i->_remoteConfigsListeners.end())
            {
                i->_remoteConfigsListeners.erase(
                    std::remove(i->_remoteConfigsListeners.begin(), i->_remoteConfigsListeners.end(), listener), 
                    i->_remoteConfigsListeners.end()
                );
            }
        }

        std::string GAState::getRemoteConfigsContentAsString()
        {
            auto& i = getInstance();
            if(!i)
            {
                return "";
            }

            return i->_configurations.dump(JSON_PRINT_INDENT);
        }

        void GAState::populateConfigurations(json& sdkConfig)
        {
            std::lock_guard<std::mutex> guard(_mtx);

            try
            {
                if (sdkConfig.contains("configs") && sdkConfig["configs"].is_array())
                {
                    json& configurations = sdkConfig["configs"];

                    for (auto& configuration : configurations)
                    {
                        if (!configuration.empty())
                        {
                            std::string key = utilities::getOptionalValue<std::string>(configuration, "key", "");

                            int64_t start_ts = utilities::getOptionalValue<int64_t>(configuration, "start_ts", -1ll);

                            int64_t end_ts  = utilities::getOptionalValue<int64_t>(configuration, "end_ts", -1ll);

                            int64_t client_ts_adjusted = getClientTsAdjusted();

                            if (!key.empty() && configuration.contains("value") && client_ts_adjusted > start_ts && client_ts_adjusted < end_ts)
                            {
                                json& value = configuration["value"];
                                if (value.is_string() || value.is_number())
                                {
                                    _configurations[key] = value;
                                    logging::GALogger::d("configuration added: %s", configuration.dump(JSON_PRINT_INDENT).c_str());
                                }
                            }
                        }
                    }
                }

                _remoteConfigsIsReady = true;
                for (auto& listener : _remoteConfigsListeners)
                {
                    listener->onRemoteConfigsUpdated();
                }
            }
            catch (json::exception& e)
            {
                logging::GALogger::e("Failed to parse json: %s", e.what());
            }
            catch (std::exception& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
        }

        void GAState::addErrorEvent(EGAErrorSeverity severity, std::string const& message)
        {
            if(!GAState::isEventSubmissionEnabled())
            {
                return;
            }

            threading::GAThreading::performTaskOnGAThread([baseMessage, severity, message]()
            {
                events::GAEvents::addErrorEvent(severity, message, json(), true);
            });
        }

        void GAState::validateAndCleanCustomFields(const json& fields, json& out)
        {
            try
            {
                json result;

                if (fields.is_object() && fields.size() > 0)
                {
                    int count = 0;

                    for (auto itr = fields.begin(); itr != fields.end(); ++itr)
                    {
                        std::string key = itr.key();

                        if(fields[key].is_null())
                        {
                            std::string msg = "validateAndCleanCustomFields: entry with key=" + key + ", value = null has been omitted because its key or value is null";

                            logging::GALogger::w(msg.c_str());
                            addErrorEvent(baseMessage, EGAErrorSeverity::Warning, msg);
                        }
                        else if(count < MAX_CUSTOM_FIELDS_COUNT)
                        {
                            char pattern[MAX_CUSTOM_FIELDS_KEY_LENGTH + 1] = "";
                            snprintf(pattern, std::size(pattern), "^[a-zA-Z0-9_]{1,%d}$", MAX_CUSTOM_FIELDS_KEY_LENGTH);

                            if(utilities::GAUtilities::stringMatch(key, pattern))
                            {
                                json& value = fields[key];

                                if(value.is_number())
                                {
                                    result[key] = value;
                                    ++count;
                                }
                                else if(value.is_string())
                                {
                                    std::string valueAsString = value.get<std::string>();

                                    if(valueAsString.length() <= MAX_CUSTOM_FIELDS_VALUE_STRING_LENGTH && valueAsString.length() > 0)
                                    {
                                        result[key] = value;
                                        ++count;
                                    }
                                    else
                                    {
                                        constexpr const char* fmt = "validateAndCleanCustomFields: entry with key=%s, value=%s has been omitted because its value is an empty string or exceeds the max number of characters (%d)";

                                        const std::string value = fields[key].get<std::string>();
                                        LogAndAddErrorEvent(EGAErrorSeverity::Warning, fmt, key.c_str(), value.c_str(), MAX_CUSTOM_FIELDS_VALUE_STRING_LENGTH);
                                    }
                                }
                                else
                                {
                                    constexpr const char* fmt = "validateAndCleanCustomFields: entry with key=%s has been omitted because its value is not a string or number";
                                    LogAndAddErrorEvent(EGAErrorSeverity::Warning, fmt, key.c_str());
                                }
                            }
                            else
                            {
                                constexpr const char* fmt = "validateAndCleanCustomFields: entry with key=%s, value=%s has been omitted because its key contains illegal character, is empty or exceeds the max number of characters (%d)";
                            
                                const std::string value = fields[key].get<std::string>();
                                LogAndAddErrorEvent(EGAErrorSeverity::Warning, fmt, key.c_str(), value.c_str(), MAX_CUSTOM_FIELDS_KEY_LENGTH);
                            }
                        }
                        else
                        {
                            constexpr const char* fmt = "validateAndCleanCustomFields: entry with key=%s has been omitted because it exceeds the max number of custom fields (%d)";
                            LogAndAddErrorEvent(EGAErrorSeverity::Warning, fmt, key.c_str(), MAX_CUSTOM_FIELDS_COUNT);
                        }
                    }
                }

                out.merge_patch(result);
            }
            catch (json::exception& e)
            {
                logging::GALogger::e(e.what());
            }
            catch (std::exception& e)
            {
                logging::GALogger::e(e.what());
            }
        }

        int64_t GAState::getClientTsAdjusted()
        {
            return utilities::getTimestamp();
        }

        std::string GAState::getBuild()
        {
            getInstance()->_build;
        }

        int64_t GAState::calculateServerTimeOffset(int64_t serverTs)
        {
            int64_t clientTs = utilities::getTimestamp();
            return _adjustTimestamp ? (serverTs - clientTs) : clientTs;
        }

        void GAState::setManualSessionHandling(bool flag)
        {
            getInstance()->_useManualSessionHandling = flag;
            logging::GALogger::i("Use manual session handling: %s", flag ? "true" : "false");
        }

        bool GAState::useManualSessionHandling()
        {
            getInstance()->_useManualSessionHandling;
        }

        void GAState::setEnableErrorReporting(bool flag)
        {
            getInstance()->_enableErrorReporting = flag;
            logging::GALogger::i("Use error reporting:: %s", flag ? "true" : "false");
        }

        bool GAState::useErrorReporting()
        {
            getInstance()->_enableErrorReporting;
        }

        void GAState::setEnabledEventSubmission(bool flag)
        {
            getInstance()->_enableEventSubmission = flag;
        }

        bool GAState::isEventSubmissionEnabled()
        {
            getInstance()->_enableEventSubmission;
        }

        void GAState::setConfigsHash(std::string const& configsHash)
        {
            getInstance()->_configsHash = configsHash;
        }

        void GAState::setAbId(std::string const& abId)
        {
            getInstance()->_abId = abId;
        }

        void GAState::setAbVariantId(std::string const& abVariantId)
        {
            getInstance()->_abVariantId = abVariantId;
        }

        std::string GAState::getAbId()
        {
            getInstance()->_abId;
        }

        std::string GAState::getAbVariantId()
        {
            return getInstance()->_abVariantId;
        }

        void GAState::getValidatedCustomFields()
        {
            json cleanedFields, d;
            getGlobalCustomEventFields(d);
            validateAndCleanCustomFields(d, cleanedFields);

            return cleanedFields;
        }

        void GAState::getValidatedCustomFields(const json& withEventFields)
        {
            json cleanedFields, d;
            getGlobalCustomEventFields(d);

            if(!withEventFields.empty())
                d.merge_patch(withEventFields);
            
            validateAndCleanCustomFields(d, cleanedFields);

            return cleanedFields;
        }
    }
}
