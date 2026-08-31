//
// GA-SDK-CPP
// Copyright 2015 GameAnalytics. All rights reserved.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <GAState.h>
#include <GAStore.h>

namespace gameanalytics
{
    namespace state
    {
        // friend of GAState (see GAState.h) exposing the private bits needed
        // to exercise ensurePersistedStates() in isolation
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
        };
    }
}

using namespace gameanalytics;

namespace
{
    constexpr const char* kGameKey = "bd624ee6f8e6efb32a054f8d7ba11618";

    void seedCachedConfig(std::string const& lastUsedIdentifier, std::string const& configsHash)
    {
        ASSERT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));

        store::GAStore::setState("last_used_identifier", lastUsedIdentifier);
        store::GAStore::setState("sdk_config_cached", std::string("{\"configs_hash\":\"") + configsHash + "\"}");
    }
}

// Regression test: the cached configs_hash must survive a relaunch with the
// same user identifier, so the init request can tell the backend which config
// version it already has. It must only be cleared when the identifier changed
// since the config was cached (matches the iOS/C# SDK behavior).

TEST(GAStateTest, testConfigsHashKeptWhenIdentifierUnchanged)
{
    seedCachedConfig("user-a", "hash-abc123");

    state::GAStateTestAccessor::resetConfigState("user-a");
    state::GAStateTestAccessor::ensurePersistedStates();

    ASSERT_EQ("hash-abc123", state::GAStateTestAccessor::configsHash());
}

TEST(GAStateTest, testConfigsHashClearedWhenIdentifierChanged)
{
    seedCachedConfig("user-a", "hash-abc123");

    state::GAStateTestAccessor::resetConfigState("user-b");
    state::GAStateTestAccessor::ensurePersistedStates();

    ASSERT_TRUE(state::GAStateTestAccessor::configsHash().empty());
}
