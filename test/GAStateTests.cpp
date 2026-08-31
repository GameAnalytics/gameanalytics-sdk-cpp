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

            static json validateAndCleanCustomFields(const json& fields)
            {
                json out;
                GAState::getInstance().validateAndCleanCustomFields(fields, out);
                return out;
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

// validateAndCleanCustomFields: keys must match ^[a-zA-Z0-9_]{1,64}$, values must be
// a number, a boolean or a non-empty string of at most 256 chars, capped at 50 fields

static json cleanFields(const json& fields)
{
    return state::GAStateTestAccessor::validateAndCleanCustomFields(fields);
}

TEST(GAStateTest, testCustomFieldsCappedAtMaxCount)
{
    json fields;
    for (int i = 0; i < MAX_CUSTOM_FIELDS_COUNT * 2; ++i)
    {
        fields["key_" + std::to_string(i)] = "value";
    }
    ASSERT_EQ(MAX_CUSTOM_FIELDS_COUNT, static_cast<int>(cleanFields(fields).size()));

    fields.clear();
    for (int i = 0; i < MAX_CUSTOM_FIELDS_COUNT; ++i)
    {
        fields["key_" + std::to_string(i)] = "value";
    }
    ASSERT_EQ(MAX_CUSTOM_FIELDS_COUNT, static_cast<int>(cleanFields(fields).size()));
}

TEST(GAStateTest, testCustomFieldsKeyValidation)
{
    ASSERT_EQ(1u, cleanFields({{"___", "value"}}).size());
    ASSERT_EQ(1u, cleanFields({{std::string(MAX_CUSTOM_FIELDS_KEY_LENGTH, 'k'), "value"}}).size());

    ASSERT_TRUE(cleanFields({{"", "value"}}).empty());
    ASSERT_TRUE(cleanFields({{"_&_", "value"}}).empty());
    ASSERT_TRUE(cleanFields({{std::string(MAX_CUSTOM_FIELDS_KEY_LENGTH + 1, 'k'), "value"}}).empty());
}

TEST(GAStateTest, testCustomFieldsValueValidation)
{
    ASSERT_EQ(1u, cleanFields({{"key", 100}}).size());
    ASSERT_EQ(1u, cleanFields({{"key", 3.14}}).size());
    ASSERT_EQ(1u, cleanFields({{"key", true}}).size());
    ASSERT_EQ(1u, cleanFields({{"key", std::string(MAX_CUSTOM_FIELDS_VALUE_STRING_LENGTH, 'v')}}).size());

    ASSERT_TRUE(cleanFields({{"key", ""}}).empty());
    ASSERT_TRUE(cleanFields({{"key", std::string(MAX_CUSTOM_FIELDS_VALUE_STRING_LENGTH + 1, 'v')}}).empty());
    ASSERT_TRUE(cleanFields({{"key", nullptr}}).empty());
    ASSERT_TRUE(cleanFields({{"key", json::object()}}).empty());
    ASSERT_TRUE(cleanFields({{"key", json::array()}}).empty());
}

// regression: a non-string value under an illegal key used to throw while logging
// the rejection, discarding every other field in the payload
TEST(GAStateTest, testCustomFieldsIllegalKeyWithNumberValueKeepsOtherFields)
{
    json out = cleanFields({{"bad&key", 100}, {"good_key", "value"}});

    ASSERT_EQ(1u, out.size());
    ASSERT_TRUE(out.contains("good_key"));
}
