#pragma once

#include <GAState.h>

namespace gameanalytics
{
    namespace state
    {
        // friend of GAState (see GAState.h) exposing the private bits needed
        // to exercise state transitions deterministically from tests
        struct GAStateTestAccessor
        {
            static void resetConfigState(std::string const& customUserId)
            {
                GAState& s = GAState::getInstance();

                s._sdkConfig       = json();
                s._sdkConfigCached = json();
                s._configsHash.clear();
                s._defaultUserId.clear();

                s._customUserId = customUserId;
                s.cacheIdentifier();
            }

            static void ensurePersistedStates()
            {
                GAState::getInstance().ensurePersistedStates();
            }

            static std::string configsHash()
            {
                return GAState::getInstance()._configsHash;
            }

            static std::string build()
            {
                return GAState::getInstance()._build;
            }

            static json validateAndCleanCustomFields(const json& fields)
            {
                json out;
                GAState::getInstance().validateAndCleanCustomFields(fields, out);
                return out;
            }

            // returns the SDK to its pre-initialize() state so every test can
            // drive the public API from a known starting point
            static void forceUninitialized()
            {
                GAState& s = GAState::getInstance();
                std::lock_guard<std::recursive_mutex> lg(s._mtx);

                s._initialized    = false;
                s._initAuthorized = false;
                s._enabled        = false;

                s._sessionStart = 0;
                s._sessionId.clear();

                s._build.clear();
                s._customUserId.clear();
                s._externalUserId.clear();
                s._identifier.clear();

                s._configsHash.clear();
                s._abId.clear();
                s._abVariantId.clear();
                s._sdkConfig       = json();
                s._sdkConfigCached = json();

                s._gameRemoteConfigsJson     = json::array();
                s._trackingRemoteConfigsJson = json::array();
                s._remoteConfigsIsReady      = false;
                s._remoteConfigsListeners.clear();

                s._currentCustomDimension01.clear();
                s._currentCustomDimension02.clear();
                s._currentCustomDimension03.clear();
                s._currentGlobalCustomEventFields = json();

                s._availableCustomDimensions01.clear();
                s._availableCustomDimensions02.clear();
                s._availableCustomDimensions03.clear();
                s._availableResourceCurrencies.clear();
                s._availableResourceItemTypes.clear();

                s._useManualSessionHandling = false;
            }
        };
    }
}
