//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include <vector>
#include <map>
#include <functional>
#include "GameAnalytics.h"
#include <mutex>
#include <cstdlib>
#include <unordered_map>

#include "GACommon.h"
#include "GAUtilities.h"
#include "GALogger.h"

namespace gameanalytics
{
    namespace state
    {
        struct ProgressionTries
        {
            public:

                inline void addOrUpdate(std::string const& s, int tries)
                {
                    _tries[s] = tries;
                }

                inline void remove(std::string const& s)
                {
                    if (_tries.count(s))
                    {
                        _tries.erase(s);
                    }
                }

                inline int getTries(std::string const& s) const
                {
                    if (_tries.count(s))
                    {
                        return _tries.at(s);
                    }

                    return 0;
                }

                inline int incrementTries(std::string const& s)
                {
                    if (_tries.count(s))
                    {
                        _tries[s]++;
                    }
                    else
                    {
                        _tries[s] = 1;
                    }

                    return _tries[s];
                }

            private:
            
                std::unordered_map<std::string, int> _tries;
        };

        class GAState
        {
            public:

                static GAState& getInstance();

                static bool isDestroyed();
                static void setUserId(std::string const& id);
                static bool isInitialized();
                static int64_t getSessionStart();
                static int getSessionNum();
                static int getTransactionNum();
                static std::string getSessionId();
                static std::string getCurrentCustomDimension01();
                static std::string getCurrentCustomDimension02();
                static std::string getCurrentCustomDimension03();
                static void getGlobalCustomEventFields(json& out);
                static std::string getGameKey();
                static std::string getGameSecret();
                static void setAvailableCustomDimensions01(const StringVector& dimensions);
                static void setAvailableCustomDimensions02(const StringVector& dimensions);
                static void setAvailableCustomDimensions03(const StringVector& dimensions);
                static void setAvailableResourceCurrencies(const StringVector& availableResourceCurrencies);
                static void setAvailableResourceItemTypes(const StringVector& availableResourceItemTypes);
                static void setBuild(std::string const& build);
                static bool isEnabled();
                static void setCustomDimension01(std::string const& dimension);
                static void setCustomDimension02(std::string const& dimension);
                static void setCustomDimension03(std::string const& dimension);
                static void setGlobalCustomEventFields(std::string const& customFields);
                static void incrementSessionNum();
                static void incrementTransactionNum();
                static void incrementProgressionTries(std::string const& progression);
                static int getProgressionTries(std::string const& progression);
                static void clearProgressionTries(std::string const& progression);
                static bool hasAvailableCustomDimensions01(std::string const& dimension1);
                static bool hasAvailableCustomDimensions02(std::string const& dimension2);
                static bool hasAvailableCustomDimensions03(std::string const& dimension3);
                static bool hasAvailableResourceCurrency(std::string const& currency);
                static bool hasAvailableResourceItemType(std::string const& itemType);
                static void setKeys(std::string const& gameKey, std::string const& gameSecret);
                static void endSessionAndStopQueue(bool endThread);
                static void resumeSessionAndStartQueue();
                static void getEventAnnotations(json& out);
                static void getSdkErrorEventAnnotations(json& out);
                static void getInitAnnotations(json& out);
                static void internalInitialize();
                static int64_t getClientTsAdjusted();
                static void setManualSessionHandling(bool flag);
                static bool useManualSessionHandling();
                static void setEnableErrorReporting(bool flag);
                static bool useErrorReporting();
                static void setEnabledEventSubmission(bool flag);
                static bool isEventSubmissionEnabled();
                static bool sessionIsStarted();
                static std::string getRemoteConfigsStringValue(std::string const& key, std::string const& defaultValue);
                static bool isRemoteConfigsReady();
                static void addRemoteConfigsListener(const std::shared_ptr<IRemoteConfigsListener>& listener);
                static void removeRemoteConfigsListener(const std::shared_ptr<IRemoteConfigsListener>& listener);
                static std::string getRemoteConfigsContentAsString();
                static std::string getAbId();
                static std::string getAbVariantId();

                static json getValidatedCustomFields();
                static json getValidatedCustomFields(const json& withEventFields);

        private:

            GAState();
            ~GAState();
            GAState(const GAState&) = delete;
            GAState& operator=(const GAState&) = delete;

            static constexpr const char* CategorySdkError = "sdk_error";

            template<typename ...args_t>
            void LogAndAddErrorEvent(EGAErrorSeverity severity, std::string const& fmt, args_t&&... args)
            {
                const std::string msg = utilities::printString(fmt, std::forward<args_t>(args)...);
                logging::GALogger::w(msg.c_str());
                addErrorEvent(severity, msg);
            }

            std::string getIdentifier();
            void  setDefaultUserId(std::string const& id);
            json& getSdkConfig();
            void  cacheIdentifier();
            void  ensurePersistedStates();
            void  startNewSession();
            void  validateAndFixCurrentDimensions();
            std::string getBuild();

            int64_t  calculateServerTimeOffset(int64_t serverTs);
            void     populateConfigurations(json& sdkConfig);

            void validateAndCleanCustomFields(const json& fields, json& out);

            void setConfigsHash(std::string const& configsHash);
            void setAbId(std::string const& abId);
            void setAbVariantId(std::string const& abVariantId);

            void addErrorEvent(EGAErrorSeverity severity, std::string const& message);

            std::string _userId;
            std::string _identifier;

            bool _initialized = false;
            bool _adjustTimestamp = true;

            int64_t _sessionStart = 0;
            int64_t _sessionNum = 0;
            int64_t _transactionNum = 0;

            std::string _sessionId;

            std::string _currentCustomDimension01;
            std::string _currentCustomDimension02;
            std::string _currentCustomDimension03;

            json _currentGlobalCustomEventFields;

            std::string _gameKey;
            std::string _gameSecret;

            StringVector _availableCustomDimensions01;
            StringVector _availableCustomDimensions02;
            StringVector _availableCustomDimensions03;
            StringVector _availableResourceCurrencies;
            StringVector _availableResourceItemTypes;

            std::string _build;

            bool _initAuthorized = false;
            bool _enabled = false;

            int64_t _clientServerTimeOffset = 0;

            std::string _defaultUserId;
            std::string _configsHash;
            std::string _abId;
            std::string _abVariantId;

            ProgressionTries    _progressionTries;
            json _sdkConfigDefault;
            json _sdkConfig;
            json _sdkConfigCached;

            bool _useManualSessionHandling  = false;
            bool _enableErrorReporting      = true;
            bool _enableEventSubmission     = true;

            json _configurations;
            bool _remoteConfigsIsReady;
            std::vector<std::shared_ptr<IRemoteConfigsListener>> _remoteConfigsListeners;
            std::mutex _mtx;
        };
    }
}
