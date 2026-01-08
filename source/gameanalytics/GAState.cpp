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
        GAState& GAState::getInstance()
        {
            static GAState instance;
            return instance;
        }

        GAState::GAState()
        {
        }

        GAState::~GAState()
        {
            _gaThread.queueBlock(
                [this]()
                {
                    if(!_useManualSessionHandling)
                        endSessionAndStopQueue(true);
                }
            );

            _gaThread.flush();
        }

        void GAState::setUserId(std::string const& id)
        {
            if(id.empty())
            {
                return;
            }

            getInstance()._customUserId = id;
            getInstance().cacheIdentifier();
        }

        std::string GAState::getUserId()
        {
            return getInstance()._identifier;
        }

        std::string GAState::getExternalUserId()
        {
            return getInstance()._externalUserId;
        }

        bool GAState::isInitialized()
        {
            return getInstance()._initialized;
        }

        int64_t GAState::getSessionStart()
        {
            return getInstance()._sessionStart;
        }

        int64_t GAState::getSessionNum()
        {
            return getInstance()._sessionNum;
        }

        int64_t GAState::getTransactionNum()
        {
            return getInstance()._transactionNum;
        }
    
        void GAState::setExternalUserId(std::string const& id)
        {
            getInstance()._externalUserId = id;
        }

        std::string GAState::getSessionId()
        {
            return getInstance()._sessionId;
        }

        std::string GAState::getCurrentCustomDimension01()
        {
            return getInstance()._currentCustomDimension01;
        }

        std::string GAState::getCurrentCustomDimension02()
        {
            return getInstance()._currentCustomDimension02;
        }

        std::string GAState::getCurrentCustomDimension03()
        {
            return getInstance()._currentCustomDimension03;
        }

        void GAState::getGlobalCustomEventFields(json& out)
        {
            out.merge_patch(getInstance()._currentGlobalCustomEventFields);
        }

        void GAState::setAvailableCustomDimensions01(const StringVector& availableCustomDimensions)
        {
            // Validate
            if (!validators::GAValidator::validateCustomDimensions(availableCustomDimensions))
            {
                return;
            }
            getInstance()._availableCustomDimensions01 = availableCustomDimensions;

            // validate current dimension values
            getInstance().validateAndFixCurrentDimensions();

            logging::GALogger::i("Set available custom01 dimension values: (%s)", utilities::printArray(availableCustomDimensions).c_str());
        }

        void GAState::setAvailableCustomDimensions02(const StringVector& availableCustomDimensions)
        {
            // Validate
            if (!validators::GAValidator::validateCustomDimensions(availableCustomDimensions))
            {
                return;
            }
            getInstance()._availableCustomDimensions02 = availableCustomDimensions;

            // validate current dimension values
            getInstance().validateAndFixCurrentDimensions();

            logging::GALogger::i("Set available custom02 dimension values: (%s)", utilities::printArray(availableCustomDimensions).c_str());
        }

        void GAState::setAvailableCustomDimensions03(const StringVector& availableCustomDimensions)
        {
            // Validate
            if (!validators::GAValidator::validateCustomDimensions(availableCustomDimensions))
            {
                return;
            }
            getInstance()._availableCustomDimensions03 = availableCustomDimensions;

            // validate current dimension values
            getInstance().validateAndFixCurrentDimensions();

            logging::GALogger::i("Set available custom03 dimension values: (%s)", utilities::printArray(availableCustomDimensions).c_str());
        }

        void GAState::setAvailableResourceCurrencies(const StringVector& availableResourceCurrencies)
        {
            // Validate
            if (!validators::GAValidator::validateResourceCurrencies(availableResourceCurrencies)) {
                return;
            }
            getInstance()._availableResourceCurrencies = availableResourceCurrencies;

            logging::GALogger::i("Set available resource currencies: (%s)", utilities::printArray(availableResourceCurrencies).c_str());
        }

        void GAState::setAvailableResourceItemTypes(const StringVector& availableResourceItemTypes)
        {
            // Validate
            if (!validators::GAValidator::validateResourceItemTypes(availableResourceItemTypes)) {
                return;
            }
            getInstance()._availableResourceItemTypes = availableResourceItemTypes;

            logging::GALogger::i("Set available resource item types: (%s)", utilities::printArray(availableResourceItemTypes).c_str());
        }

        void GAState::setBuild(std::string const& build)
        {
            getInstance()._build = build;
            logging::GALogger::i("Set build: %s", build.c_str());
        }

        void GAState::setDefaultUserId(std::string const& id)
        {
            getInstance()._defaultUserId = id;
            cacheIdentifier();
        }

        json& GAState::getSdkConfig()
        {
            if (getInstance()._sdkConfig.is_object())
            {
                return _sdkConfig;
            }
            else if (getInstance()._sdkConfigCached.is_object())
            {
                return _sdkConfigCached;
            }

            return _sdkConfigDefault;
        }

        bool GAState::isEnabled()
        {
            return getInstance()._enabled;
        }

        void GAState::setCustomDimension01(std::string const& dimension)
        {
            getInstance()._currentCustomDimension01 = dimension;
            if (store::GAStore::getTableReady())
            {
                store::GAStore::setState("dimension01", dimension.c_str());
            }
            logging::GALogger::i("Set custom01 dimension value: %s", dimension.c_str());
        }

        void GAState::setCustomDimension02(std::string const& dimension)
        {
            getInstance()._currentCustomDimension02 = dimension;
            if (store::GAStore::getTableReady())
            {
                store::GAStore::setState("dimension02", dimension.c_str());
            }
            logging::GALogger::i("Set custom02 dimension value: %s", dimension.c_str());
        }

        void GAState::setCustomDimension03(std::string const& dimension)
        {
            getInstance()._currentCustomDimension03 = dimension;
            if (store::GAStore::getTableReady())
            {
                store::GAStore::setState("dimension03", dimension.c_str());
            }
            logging::GALogger::i("Set custom03 dimension value: %s", dimension.c_str());
        }

        void GAState::setGlobalCustomEventFields(std::string const& customFields)
        {
            try
            {
                getInstance()._currentGlobalCustomEventFields = json::parse(customFields);
                logging::GALogger::i("Set global custom event fields: %s", customFields.c_str());
            }
            catch(std::exception& e)
            {
                logging::GALogger::e(e.what());
            }
        }

        void GAState::incrementSessionNum()
        {
            getInstance()._sessionNum++;
        }

        void GAState::incrementTransactionNum()
        {
            getInstance()._transactionNum++;
        }

        void GAState::incrementProgressionTries(std::string const& progression)
        {
            int tries = getInstance()._progressionTries.incrementTries(progression);

            // Persist
            std::string triesString = std::to_string(tries);

            StringVector parms = {progression, triesString};
            store::GAStore::executeQuerySync("INSERT OR REPLACE INTO ga_progression (progression, tries) VALUES(?, ?);", parms);
        }

        int GAState::getProgressionTries(std::string const& progression)
        {
            return getInstance()._progressionTries.getTries(progression);
        }

        void GAState::clearProgressionTries(std::string const& progression)
        {
            getInstance()._progressionTries.remove(progression);

            // Delete
            StringVector parms = {progression};
            store::GAStore::executeQuerySync("DELETE FROM ga_progression WHERE progression = ?;", parms);
        }

        bool GAState::hasAvailableCustomDimensions01(std::string const& dimension1)
        {
            return utilities::GAUtilities::stringVectorContainsString(getInstance()._availableCustomDimensions01, dimension1);
        }

        bool GAState::hasAvailableCustomDimensions02(std::string const& dimension2)
        {
            return utilities::GAUtilities::stringVectorContainsString(
                getInstance()._availableCustomDimensions02, dimension2);
        }

        bool GAState::hasAvailableCustomDimensions03(std::string const& dimension3)
        {
            return utilities::GAUtilities::stringVectorContainsString(getInstance()._availableCustomDimensions03, dimension3);
        }

        bool GAState::hasAvailableResourceCurrency(std::string const& currency)
        {
            return utilities::GAUtilities::stringVectorContainsString(getInstance()._availableResourceCurrencies, currency);
        }

        bool GAState::hasAvailableResourceItemType(std::string const& itemType)
        {
            return utilities::GAUtilities::stringVectorContainsString(getInstance()._availableResourceItemTypes, itemType);
        }

        void GAState::setKeys(std::string const& gameKey, std::string const& gameSecret)
        {
            getInstance()._gameKey     = gameKey;
            getInstance()._gameSecret  = gameSecret;
        }

        std::string GAState::getGameKey()
        {
            return getInstance()._gameKey;
        }

        std::string GAState::getGameSecret()
        {
            return getInstance()._gameSecret;
        }

        void GAState::internalInitialize()
        {
            // Make sure database is ready
            if (!store::GAStore::getTableReady())
            {
                return;
            }

            // Make sure persisted states are loaded
            getInstance().ensurePersistedStates();
            store::GAStore::setState("default_user_id", getInstance()._defaultUserId.c_str());

            getInstance()._initialized = true;
            getInstance().startNewSession();

            events::GAEvents::addSDKInitEvent();

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
                getInstance().startNewSession();
            }
            events::GAEvents::ensureEventQueueIsRunning();
        }

        void GAState::endSessionAndStopQueue(bool endThread)
        {
            if(GAState::isInitialized())
            {
                logging::GALogger::i("Ending session.");
                if (GAState::isEnabled() && GAState::sessionIsStarted())
                {
                    getInstance().updateTotalSessionTime();
                    events::GAEvents::addHealthEvent();
                    events::GAEvents::addSessionEndEvent();
                    getInstance()._sessionStart = 0;
                }
                events::GAEvents::stopEventQueue();
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
                // ---- REQUIRED ---- //

                // collector event API version
                out["v"] = 2;
                out["event_uuid"] = utilities::GAUtilities::generateUUID();
                
                // User identifier
                out["user_id"] = getUserId();
                
                // remote configs configurations
                if(getInstance()._trackingRemoteConfigsJson.is_array() && !getInstance()._trackingRemoteConfigsJson.empty())
                {
                    out["configurations_v3"] = getInstance().getRemoteConfigAnnotations();
                }

                out["sdk_version"] = device::GADevice::getRelevantSdkVersion();
                out["client_ts"] = utilities::GAUtilities::timeIntervalSince1970();
                out["os_version"] = device::GADevice::getOSVersion();
                out["manufacturer"] = device::GADevice::getDeviceManufacturer();
                out["device"] = device::GADevice::getDeviceModel();
                out["platform"] = device::GADevice::getBuildPlatform();
                out["session_id"] = getInstance()._sessionId;
                out["session_num"] = getInstance()._sessionNum;
                out["connection_type"] = device::GADevice::getConnectionType();

                // playtime metrics
                out["current_session_length"] = getInstance().calculateSessionLength();
                out["lifetime_session_length"] = getInstance().getTotalSessionLength();

                // ---- OPTIONAL ---- //

                // A/B testing
                utilities::addIfNotEmpty(out, "ab_id", getInstance()._abId);
                utilities::addIfNotEmpty(out, "ab_variant_id", getInstance()._abVariantId);
                
                utilities::addIfNotEmpty(out, "user_id_ext", getInstance()._externalUserId);

                utilities::addIfNotEmpty(out, "build", getInstance()._build);
                utilities::addIfNotEmpty(out, "engine_version", device::GADevice::getGameEngineVersion());

#if USE_UWP
                utilities::addIfNotEmpty(out, "uwp_aid", device::GADevice::getAdvertisingId());
                utilities::addIfNotEmpty(out, "uwp_id", device::GADevice::getDeviceId());
#endif
            }
            catch (json::exception const& e)
            {
                logging::GALogger::e("getEventAnnotations - json error: %s", e.what());
            }
            catch(std::exception const& e)
            {
                logging::GALogger::e("getEventAnnotations - exception thrown: %s", e.what());
            }
        }

        void GAState::getSdkErrorEventAnnotations(json& out)
        {
            // ---- REQUIRED ---- //

            // collector event API version
            out["v"] = 2;
            out["event_uuid"]       = utilities::GAUtilities::generateUUID();

            out["sdk_version"]      = device::GADevice::getRelevantSdkVersion();
            out["os_version"]       = device::GADevice::getOSVersion();
            out["manufacturer"]     = device::GADevice::getDeviceManufacturer();
            out["device"]           = device::GADevice::getDeviceModel();
            out["platform"]         = device::GADevice::getBuildPlatform();
            out["connection_type"]  = device::GADevice::getConnectionType();
            out["category"]         = "sdk_error";

            // ---- OPTIONAL ---- //

            utilities::addIfNotEmpty(out, "build", getInstance()._build);
            utilities::addIfNotEmpty(out, "engine_version", device::GADevice::getGameEngineVersion());
        }

        void GAState::getInitAnnotations(json& out)
        {
            try
            {
                const std::string id = getInstance()._identifier;

                if (!id.empty())
                {
                    getInstance().cacheIdentifier();
                    store::GAStore::setState("last_used_identifier", id);
                }

                out["user_id"]      = id;
                out["sdk_version"]  = device::GADevice::getRelevantSdkVersion();
                out["os_version"]   = device::GADevice::getOSVersion();
                out["manufacturer"] = device::GADevice::getDeviceManufacturer();
                out["device"]       = device::GADevice::getDeviceModel();
                out["platform"]     = device::GADevice::getBuildPlatform();
                out["session_id"]   = getInstance()._sessionId;
                out["session_num"]  = out["random_salt"] = getInstance().getSessionNum();

                utilities::addIfNotEmpty(out, "build", getInstance()._build);
                utilities::addIfNotEmpty(out, "engine_version", device::GADevice::getGameEngineVersion());
            }
            catch (std::exception const& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
        }

        void GAState::cacheIdentifier()
        {
            if(!_customUserId.empty())
            {
                _identifier = _customUserId;
            }
            else
            {
                _identifier = _defaultUserId;
            }

            logging::GALogger::d("identifier, {clean:%s}", _identifier.c_str());
        }

        std::string setStateFromCache(json& dict, std::string const& key, std::string const& value)
        {
            if (!value.empty())
            {
                store::GAStore::setState(key, value);
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

        int64_t GAState::getLastSessionLength() const
        {
            return _lastSessionTime;
        }

        int64_t GAState::getTotalSessionLength() const
        {
            return _totalElapsedSessionTime + calculateSessionLength<std::chrono::seconds>();
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
                            state_dict[(*it)["key"].get<std::string>()] = (*it)["value"];
                        }
                    }
                }
                
                std::string s = state_dict.dump();
                _gaLogger.d("state_dict: %s", s.c_str());

                // insert into GAState instance
                std::string defaultId = utilities::getOptionalValue<std::string>(state_dict, "default_user_id");
                if (defaultId.empty())
                {
                    const std::string id = utilities::GAUtilities::generateUUID();
                    setDefaultUserId(id);
                }
                else
                {
                    setDefaultUserId(defaultId);
                }

                _sessionNum     = utilities::getNumberFromCache(state_dict, "session_num", 0ll);
                _transactionNum = utilities::getNumberFromCache(state_dict, "transaction_num", 0ll);

                // restore dimension settings
                _currentCustomDimension01 = setStateFromCache(state_dict, "dimension01", _currentCustomDimension01);
                _currentCustomDimension02 = setStateFromCache(state_dict, "dimension02", _currentCustomDimension02);
                _currentCustomDimension03 = setStateFromCache(state_dict, "dimension03", _currentCustomDimension03);
                
                try
                {
                    std::string cachedLastSessionTime = utilities::getOptionalValue<std::string>(state_dict, "last_session_time", "0");
                    std::string cachedTotalSessionTime = utilities::getOptionalValue<std::string>(state_dict, "total_session_time", "0");
                    
                    _lastSessionTime = std::stoull(cachedLastSessionTime);
                    _totalElapsedSessionTime = std::stoull(cachedTotalSessionTime);
                }
                catch(const std::exception& e)
                {
                    _gaLogger.w("Failed to read total_session_time from cache!");
                    _totalElapsedSessionTime = 0;
                }
                

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

                    _configsHash = utilities::getOptionalValue<std::string>(currentSdkConfig, "configs_hash");
                    _abId        = utilities::getOptionalValue<std::string>(currentSdkConfig, "ab_id");
                    _abVariantId = utilities::getOptionalValue<std::string>(currentSdkConfig, "ab_variant_id");
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

                            int tries = utilities::getOptionalValue<int>(node, "tries", 0);
                            _progressionTries.addOrUpdate(name, tries);
                        }
                    }
                }
            }
            catch (json::exception& e)
            {
                logging::GALogger::e("ensurePersistedStates - Failed to parse json: %s", e.what());
            }
            catch (std::exception& e)
            {
                logging::GALogger::e("ensurePersistedStates - Exception thrown: %s", e.what());
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
                http::GAHTTPApi& httpApi = http::GAHTTPApi::getInstance();

                json initResponseDict;
                http::EGAHTTPApiResponse initResponse = httpApi.requestInitReturningDict(initResponseDict, _configsHash);

                // init is ok
                if ((initResponse == http::Ok || initResponse == http::Created) && !initResponseDict.empty())
                {
                    // set the time offset - how many seconds the local time is different from servertime
                    int64_t timeOffsetSeconds = 0;
                    int64_t server_ts = utilities::getOptionalValue<int64_t>(initResponseDict, "server_ts", -1ll);
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
                        if (currentSdkConfig.contains("ab_variant_id") && currentSdkConfig["ab_variant_id"].is_string())
                        {
                            initResponseDict["ab_variant_id"] = currentSdkConfig["ab_variant_id"];
                        }
                    }

                    _configsHash = utilities::getOptionalValue<std::string>(initResponseDict, "configs_hash");
                    _abId        = utilities::getOptionalValue<std::string>(initResponseDict, "ab_id");
                    _abVariantId = utilities::getOptionalValue<std::string>(initResponseDict, "ab_variant_id");

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

                json& currentSdkConfig = GAState::getSdkConfig();
                {
                    if (!utilities::getOptionalValue<bool>(currentSdkConfig, "enabled", true))
                    {
                        _enabled = false;
                    }
                    else if (!_initAuthorized)
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

                // to acurrately measure time
                _startTimepoint = std::chrono::high_resolution_clock::now();

                // Add session start event
                events::GAEvents::addSessionStartEvent();
            }
            catch (json::exception& e)
            {
                logging::GALogger::e("startNewSession - Failed to parse json: %s", e.what());
            }
            catch (std::exception& e)
            {
                logging::GALogger::e("startNewSession - Exception thrown: %s", e.what());
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
            return getInstance()._sessionStart != 0;
        }

        bool GAState::isRemoteConfigsReady()
        {
            return getInstance()._remoteConfigsIsReady;
        }

        void GAState::addRemoteConfigsListener(const std::shared_ptr<IRemoteConfigsListener>& listener)
        {
            if(std::find(getInstance()._remoteConfigsListeners.begin(), getInstance()._remoteConfigsListeners.end(), listener) == getInstance()._remoteConfigsListeners.end())
            {
                getInstance()._remoteConfigsListeners.push_back(listener);
            }
        }

        void GAState::removeRemoteConfigsListener(const std::shared_ptr<IRemoteConfigsListener>& listener)
        {
            if(std::find(getInstance()._remoteConfigsListeners.begin(), getInstance()._remoteConfigsListeners.end(), listener) != getInstance()._remoteConfigsListeners.end())
            {
                getInstance()._remoteConfigsListeners.erase(
                    std::remove(getInstance()._remoteConfigsListeners.begin(), getInstance()._remoteConfigsListeners.end(), listener), 
                    getInstance()._remoteConfigsListeners.end()
                );
            }
        }

        std::string GAState::getRemoteConfigsContentAsString()
        {
            std::lock_guard<std::recursive_mutex> lg(getInstance()._mtx);

            json contents;

            for(auto& obj : getInstance()._gameRemoteConfigsJson)
            {
                if(obj.contains("key") && obj.contains("value"))
                {
                    std::string key = utilities::getOptionalValue<std::string>(obj, "key", "");
                    if(!key.empty())
                    {
                        contents[key] = obj["value"];
                    }
                }
            }

            return contents.dump(JSON_PRINT_INDENT);
        }

        void GAState::buildRemoteConfigsJsons(const json& remoteCfgs)
        {
            _gameRemoteConfigsJson = json::array();
            _trackingRemoteConfigsJson = json::array();

            for (const auto& configuration : remoteCfgs)
            {
                _gameRemoteConfigsJson.push_back({
                        {"key", configuration["key"]},
                        {"value", configuration["value"]}
                });

                _trackingRemoteConfigsJson.push_back({
                    {"key", configuration["key"]},
                    {"id", configuration["id"]},
                    {"vsn", configuration["vsn"]}
                });
            }

            logging::GALogger::d("Remote configs: %s", _gameRemoteConfigsJson.dump(JSON_PRINT_INDENT).c_str());
            logging::GALogger::d("Remote configs for tracking: %s", _trackingRemoteConfigsJson.dump(JSON_PRINT_INDENT).c_str());
            logging::GALogger::i("Remote configs ready with %zu configurations", _gameRemoteConfigsJson.size());
        }

        void GAState::populateConfigurations(json& sdkConfig)
        {
    
            json _tempRemoteConfigsJson = {};

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
                            int64_t start_ts = utilities::getOptionalValue<int64_t>(configuration, "start_ts", std::numeric_limits<int64_t>::min());
                            int64_t end_ts  = utilities::getOptionalValue<int64_t>(configuration, "end_ts", std::numeric_limits<int64_t>::max());

                            int64_t client_ts_adjusted = getClientTsAdjusted();

                            if (!key.empty() && configuration.contains("value") && client_ts_adjusted > start_ts && client_ts_adjusted < end_ts)
                            {
                                _tempRemoteConfigsJson[key] = configuration;
                                logging::GALogger::d("configuration added: %s", configuration.dump(JSON_PRINT_INDENT).c_str());
                            }
                        }
                    }
                }

                buildRemoteConfigsJsons(_tempRemoteConfigsJson);

                _remoteConfigsIsReady = true;
                
                std::string const configStr = _gameRemoteConfigsJson.dump();
                for (auto& listener : _remoteConfigsListeners)
                {
                    listener->onRemoteConfigsUpdated(configStr);
                }
            }
            catch (json::exception& e)
            {
                logging::GALogger::e("populateConfigurations - Failed to parse json: %s", e.what());
            }
            catch (std::exception& e)
            {
                logging::GALogger::e("populateConfigurations - Exception thrown: %s", e.what());
            }
        }

        void GAState::addErrorEvent(EGAErrorSeverity severity, std::string const& message)
        {
            if(!GAState::isEventSubmissionEnabled())
            {
                return;
            }

            threading::GAThreading::performTaskOnGAThread([=]()
            {
                events::GAEvents::addErrorEvent(severity, message, "", -1, json(), true);
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
                            addErrorEvent(EGAErrorSeverity::Warning, msg);
                        }
                        else if(count < MAX_CUSTOM_FIELDS_COUNT)
                        {
                            char pattern[MAX_CUSTOM_FIELDS_KEY_LENGTH + 1] = "";
                            snprintf(pattern, std::size(pattern), "^[a-zA-Z0-9_]{1,%d}$", MAX_CUSTOM_FIELDS_KEY_LENGTH);

                            if(utilities::GAUtilities::stringMatch(key, pattern))
                            {
                                const json& value = fields[key];

                                if(value.is_number() || value.is_boolean())
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
                logging::GALogger::e("validateAndCleanCustomFields - Failed to parse json: %s", e.what());
            }
            catch (std::exception& e)
            {
                logging::GALogger::e("validateAndCleanCustomFields - Exception thrown: %s", e.what());
            }
        }

        int64_t GAState::getClientTsAdjusted()
        {
            return utilities::GAUtilities::timeIntervalSince1970();
        }

        void GAState::updateTotalSessionTime()
        {
            _lastSessionTime = calculateSessionLength();
            _totalElapsedSessionTime += _lastSessionTime;
            
            _gaStore.setState("last_session_time",  std::to_string(_lastSessionTime));
            _gaStore.setState("total_session_time", std::to_string(_totalElapsedSessionTime));
        }

        std::string GAState::getBuild()
        {
            return getInstance()._build;
        }

        int64_t GAState::calculateServerTimeOffset(int64_t serverTs)
        {
            int64_t clientTs = utilities::GAUtilities::timeIntervalSince1970();
            return _adjustTimestamp ? (serverTs - clientTs) : clientTs;
        }

        void GAState::setManualSessionHandling(bool flag)
        {
            getInstance()._useManualSessionHandling = flag;
            logging::GALogger::i("Use manual session handling: %s", flag ? "true" : "false");
        }

        bool GAState::useManualSessionHandling()
        {
            return getInstance()._useManualSessionHandling;
        }

        void GAState::setEnableErrorReporting(bool flag)
        {
            getInstance()._enableErrorReporting = flag;
            logging::GALogger::i("Use error reporting:: %s", flag ? "true" : "false");
        }

        bool GAState::useErrorReporting()
        {
            return getInstance()._enableErrorReporting;
        }

        void GAState::setEnabledEventSubmission(bool flag)
        {
            getInstance()._enableEventSubmission = flag;
        }

        bool GAState::isEventSubmissionEnabled()
        {
            return getInstance()._enableEventSubmission;
        }

        void GAState::setConfigsHash(std::string const& configsHash)
        {
            getInstance()._configsHash = configsHash;
        }

        void GAState::setAbId(std::string const& abId)
        {
            getInstance()._abId = abId;
        }

        void GAState::setAbVariantId(std::string const& abVariantId)
        {
            getInstance()._abVariantId = abVariantId;
        }

        std::string GAState::getAbId()
        {
            return getInstance()._abId;
        }

        std::string GAState::getAbVariantId()
        {
            return getInstance()._abVariantId;
        }

        json GAState::getValidatedCustomFields()
        {
            json cleanedFields, d;
            getGlobalCustomEventFields(d);
            getInstance().validateAndCleanCustomFields(d, cleanedFields);

            return cleanedFields;
        }

        json GAState::getValidatedCustomFields(const json& withEventFields)
        {
            json cleanedFields, d;
            getGlobalCustomEventFields(d);

            if(!withEventFields.empty())
                d.merge_patch(withEventFields);
            
            getInstance().validateAndCleanCustomFields(d, cleanedFields);

            return cleanedFields;
        }

        json GAState::getRemoteConfigAnnotations()
        {
            return _trackingRemoteConfigsJson;
        }
    }
}
