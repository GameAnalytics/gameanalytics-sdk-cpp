//
// GA-SDK-CPP
// Integration tests for the public GameAnalytics facade, driving the real GA
// thread and asserting on the observable outcome (state, event store, mock http)
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "GameAnalytics/GameAnalytics.h"
#include "GameAnalytics/GAHttpClient.h"

#include <GAState.h>
#include <GAStore.h>
#include <GAEvents.h>
#include <GADevice.h>
#include <GAHTTPApi.h>
#include <GAThreading.h>
#include <GAUtilities.h>

#include "helpers/GAStateTestAccessor.h"

#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace gameanalytics;

namespace
{
    constexpr const char* kGameKey    = "bd624ee6f8e6efb32a054f8d7ba11618";
    constexpr const char* kGameSecret = "7f5c3f682cbd217841efba92e92ffb1b3b6612bc";

    // shared handle so the client installed through the public
    // configureHttpClient<T>() template stays inspectable from the test
    struct MockHttpState
    {
        std::mutex mutex;
        int requestCount = 0;
        std::string lastUrl;
        GAHttpClient::Response response;
    };

    class MockHttpClient : public GAHttpClient
    {
    public:
        explicit MockHttpClient(std::shared_ptr<MockHttpState> state) : _state(std::move(state)) {}

        void initialize() override {}
        void cleanup() override {}

        Response sendRequest(
            std::string const& url,
            std::string const&,
            std::vector<uint8_t> const&,
            bool,
            void*) override
        {
            std::lock_guard<std::mutex> lock(_state->mutex);
            _state->requestCount++;
            _state->lastUrl = url;
            return _state->response;
        }

    private:
        std::shared_ptr<MockHttpState> _state;
    };

    class RecordingConfigsListener : public IRemoteConfigsListener
    {
    public:
        void onRemoteConfigsUpdated(std::string const& remoteConfigs) override
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _updates.push_back(remoteConfigs);
        }

        std::vector<std::string> updates()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            return _updates;
        }

    private:
        std::mutex _mutex;
        std::vector<std::string> _updates;
    };

    class GameAnalyticsApiTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            events::GAEvents::stopEventQueue();
            state::GAStateTestAccessor::forceUninitialized();
            state::GAState::setEnabledEventSubmission(true);

            ASSERT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));
            clearStoredEvents();

            http = std::make_shared<MockHttpState>();
            setHttpResponse(200, json{{"server_ts", utilities::getTimestamp()}}.dump());
            GameAnalytics::configureHttpClient<MockHttpClient>(http);
        }

        void TearDown() override
        {
            drainGAThread();
            events::GAEvents::stopEventQueue();
            state::GAState::setEnabledEventSubmission(false);
            http::GAHTTPApi::setCustomHttpImpl(nullptr);
            state::GAStateTestAccessor::forceUninitialized();
            clearStoredEvents();
        }

        // barrier: the GA thread runs queued blocks in FIFO order, so once this
        // marker task has run every previously queued task has run too
        [[nodiscard]] static bool drainGAThread()
        {
            std::promise<void> done;
            std::future<void> drained = done.get_future();
            threading::GAThreading::performTaskOnGAThread([&done]() { done.set_value(); });
            return drained.wait_for(std::chrono::seconds(10)) == std::future_status::ready;
        }

        void setHttpResponse(long code, std::string const& body)
        {
            std::lock_guard<std::mutex> lock(http->mutex);
            http->response.code = code;
            http->response.packet.assign(body.begin(), body.end());
        }

        int requestCount()
        {
            std::lock_guard<std::mutex> lock(http->mutex);
            return http->requestCount;
        }

        std::string lastRequestUrl()
        {
            std::lock_guard<std::mutex> lock(http->mutex);
            return http->lastUrl;
        }

        void configureDefaults()
        {
            GameAnalytics::configureBuild("1.2.3");
            GameAnalytics::configureAvailableCustomDimensions01({"ninja", "samurai"});
            GameAnalytics::configureAvailableCustomDimensions02({"guild_a", "guild_b"});
            GameAnalytics::configureAvailableCustomDimensions03({"tier1", "tier2"});
            GameAnalytics::configureAvailableResourceCurrencies({"gems", "gold"});
            GameAnalytics::configureAvailableResourceItemTypes({"boost", "weapon"});
        }

        void initializeSdk()
        {
            configureDefaults();
            GameAnalytics::initialize(kGameKey, kGameSecret);
            ASSERT_TRUE(drainGAThread());
            ASSERT_TRUE(state::GAState::isInitialized());
            ASSERT_TRUE(state::GAState::sessionIsStarted());
            events::GAEvents::stopEventQueue();
        }

        // ga_session rows survive until a session_end is successfully sent, and
        // fixMissingSessionEndEvents synthesizes session_end events for stale rows,
        // so both tables must be cleared for tests to stay independent
        static void clearStoredEvents()
        {
            store::GAStore::executeQuerySync("DELETE FROM ga_events;");
            store::GAStore::executeQuerySync("DELETE FROM ga_session;");
        }

        static size_t storedSessionEndCount(std::string const& sessionId)
        {
            size_t count = 0;
            for (const json& ev : storedEvents("session_end"))
            {
                if (ev.value("session_id", "") == sessionId)
                {
                    ++count;
                }
            }
            return count;
        }

        static std::vector<json> storedEvents(std::string const& category = "")
        {
            std::string sql = "SELECT event FROM ga_events";
            if (!category.empty())
            {
                sql += " WHERE category='" + category + "'";
            }
            sql += ";";

            json rows;
            store::GAStore::executeQuerySync(sql, rows);

            std::vector<json> events;
            if (rows.is_array())
            {
                for (auto& row : rows)
                {
                    events.push_back(json::parse(row["event"].get<std::string>()));
                }
            }
            return events;
        }

        std::shared_ptr<MockHttpState> http;
    };
}

// ---- configuration before initialize ----

TEST_F(GameAnalyticsApiTest, PreInitConfigurationIsApplied)
{
    configureDefaults();
    GameAnalytics::configureWritablePath(device::GADevice::getWritablePath());
    GameAnalytics::configureBuildPlatform("windows");
    GameAnalytics::configureDeviceModel("TestDeviceModel");
    GameAnalytics::configureDeviceManufacturer("TestManufacturer");
    GameAnalytics::configureGameEngineVersion("unity 2021.3");
    GameAnalytics::configureSdkGameEngineVersion("unity 6.1.0");
    GameAnalytics::configureUserId("custom_user");
    GameAnalytics::configureExternalUserId("ext-42");
    ASSERT_TRUE(drainGAThread());

    EXPECT_EQ("1.2.3", state::GAStateTestAccessor::build());
    EXPECT_TRUE(state::GAState::hasAvailableCustomDimensions01("ninja"));
    EXPECT_FALSE(state::GAState::hasAvailableCustomDimensions01("pirate"));
    EXPECT_TRUE(state::GAState::hasAvailableCustomDimensions02("guild_b"));
    EXPECT_TRUE(state::GAState::hasAvailableCustomDimensions03("tier2"));
    EXPECT_TRUE(state::GAState::hasAvailableResourceCurrency("gems"));
    EXPECT_FALSE(state::GAState::hasAvailableResourceCurrency("diamonds"));
    EXPECT_TRUE(state::GAState::hasAvailableResourceItemType("boost"));

    EXPECT_TRUE(device::GADevice::getWritablePathStatus());
    EXPECT_EQ("windows", device::GADevice::getBuildPlatform());
    EXPECT_EQ("TestDeviceModel", device::GADevice::getDeviceModel());
    EXPECT_EQ("TestManufacturer", device::GADevice::getDeviceManufacturer());
    EXPECT_EQ("unity 2021.3", device::GADevice::getGameEngineVersion());
    EXPECT_EQ("unity 6.1.0", device::GADevice::getRelevantSdkVersion());

    EXPECT_EQ("custom_user", GameAnalytics::getUserId());
    EXPECT_EQ("ext-42", GameAnalytics::getExternalUserId());
}

TEST_F(GameAnalyticsApiTest, PreInitConfigurationRejectsInvalidValues)
{
    GameAnalytics::configureBuild("1.0.0");
    ASSERT_TRUE(drainGAThread());
    ASSERT_EQ("1.0.0", state::GAStateTestAccessor::build());

    const std::string userIdBefore     = GameAnalytics::getUserId();
    const std::string engineBefore     = device::GADevice::getGameEngineVersion();
    const std::string sdkVersionBefore = device::GADevice::getRelevantSdkVersion();
    const std::string platformBefore   = device::GADevice::getBuildPlatform();

    GameAnalytics::configureBuild(std::string(33, 'b'));
    GameAnalytics::configureUserId("");
    GameAnalytics::configureGameEngineVersion("notanengine 1.0");
    GameAnalytics::configureSdkGameEngineVersion("bogus");
    GameAnalytics::configureBuildPlatform(std::string(33, 'p'));
    ASSERT_TRUE(drainGAThread());

    EXPECT_EQ("1.0.0", state::GAStateTestAccessor::build());
    EXPECT_EQ(userIdBefore, GameAnalytics::getUserId());
    EXPECT_EQ(engineBefore, device::GADevice::getGameEngineVersion());
    EXPECT_EQ(sdkVersionBefore, device::GADevice::getRelevantSdkVersion());
    EXPECT_EQ(platformBefore, device::GADevice::getBuildPlatform());
}

// ---- initialize ----

TEST_F(GameAnalyticsApiTest, InitializeWithInvalidKeysIsRejected)
{
    GameAnalytics::initialize("invalid", "keys");
    ASSERT_TRUE(drainGAThread());

    EXPECT_FALSE(state::GAState::isInitialized());
    EXPECT_EQ(0, requestCount());
}

TEST_F(GameAnalyticsApiTest, InitializeStartsSessionAndRequestsInit)
{
    initializeSdk();

    EXPECT_GE(requestCount(), 1);
    EXPECT_NE(std::string::npos, lastRequestUrl().find(kGameKey));

    const std::string sessionId = state::GAState::getSessionId();
    EXPECT_EQ(36u, sessionId.size());
    EXPECT_EQ(utilities::toLowerCase(sessionId), sessionId);

    EXPECT_FALSE(GameAnalytics::getUserId().empty());
    EXPECT_FALSE(GameAnalytics::isThreadEnding());

    EXPECT_GE(GameAnalytics::getElapsedSessionTime(), 0);
    EXPECT_GE(GameAnalytics::getElapsedTimeFromAllSessions(), 0);

    // the session start event is dispatched immediately: init request first,
    // then the events request that carries it
    EXPECT_GE(requestCount(), 2);
    EXPECT_NE(std::string::npos, lastRequestUrl().find("/events"));
}

TEST_F(GameAnalyticsApiTest, InitializeTwiceKeepsFirstSession)
{
    initializeSdk();
    const std::string firstSessionId = state::GAState::getSessionId();

    GameAnalytics::initialize(kGameKey, kGameSecret);
    ASSERT_TRUE(drainGAThread());

    EXPECT_TRUE(state::GAState::isInitialized());
    EXPECT_EQ(firstSessionId, state::GAState::getSessionId());
}

TEST_F(GameAnalyticsApiTest, ConfigureAfterInitializeIsIgnored)
{
    initializeSdk();
    const std::string userIdBefore = GameAnalytics::getUserId();

    GameAnalytics::configureBuild("9.9.9");
    GameAnalytics::configureAvailableCustomDimensions01({"pirate"});
    GameAnalytics::configureUserId("late_user");
    ASSERT_TRUE(drainGAThread());

    EXPECT_EQ("1.2.3", state::GAStateTestAccessor::build());
    EXPECT_FALSE(state::GAState::hasAvailableCustomDimensions01("pirate"));
    EXPECT_EQ(userIdBefore, GameAnalytics::getUserId());
}

// ---- adding events ----

TEST_F(GameAnalyticsApiTest, AddDesignEventIsStoredWithValueAndFields)
{
    initializeSdk();

    GameAnalytics::addDesignEvent("level:complete", 42.5, R"({"difficulty":"hard"})");
    ASSERT_TRUE(drainGAThread());

    auto stored = storedEvents("design");
    ASSERT_EQ(1u, stored.size());
    EXPECT_EQ("level:complete", stored[0]["event_id"].get<std::string>());
    EXPECT_DOUBLE_EQ(42.5, stored[0]["value"].get<double>());
    EXPECT_EQ("hard", stored[0]["custom_fields"]["difficulty"].get<std::string>());
}

TEST_F(GameAnalyticsApiTest, AddDesignEventWithMalformedFieldsIsStoredWithoutFields)
{
    initializeSdk();

    GameAnalytics::addDesignEvent("level:skip", "{not valid json");
    ASSERT_TRUE(drainGAThread());

    auto stored = storedEvents("design");
    ASSERT_EQ(1u, stored.size());
    EXPECT_FALSE(stored[0].contains("custom_fields"));
}

TEST_F(GameAnalyticsApiTest, AddBusinessEventIsStored)
{
    initializeSdk();

    GameAnalytics::addBusinessEvent("USD", 499, "weapon", "sword", "shop");
    ASSERT_TRUE(drainGAThread());

    auto stored = storedEvents("business");
    ASSERT_EQ(1u, stored.size());
    EXPECT_EQ("weapon:sword", stored[0]["event_id"].get<std::string>());
    EXPECT_EQ("USD", stored[0]["currency"].get<std::string>());
    EXPECT_EQ(499, stored[0]["amount"].get<int>());
    EXPECT_EQ("shop", stored[0]["cart_type"].get<std::string>());
}

TEST_F(GameAnalyticsApiTest, AddResourceEventValidatesConfiguredCurrenciesAndItemTypes)
{
    initializeSdk();

    GameAnalytics::addResourceEvent(EGAResourceFlowType::Source, "gems", 100.0f, "boost", "starter");
    GameAnalytics::addResourceEvent(EGAResourceFlowType::Sink, "gems", 25.0f, "boost", "starter");
    GameAnalytics::addResourceEvent(EGAResourceFlowType::Source, "diamonds", 10.0f, "boost", "starter");
    GameAnalytics::addResourceEvent(EGAResourceFlowType::Source, "gems", 10.0f, "hat", "starter");
    ASSERT_TRUE(drainGAThread());

    auto stored = storedEvents("resource");
    ASSERT_EQ(2u, stored.size());
    EXPECT_EQ("Source:gems:boost:starter", stored[0]["event_id"].get<std::string>());
    EXPECT_DOUBLE_EQ(100.0, stored[0]["amount"].get<double>());
    EXPECT_EQ("Sink:gems:boost:starter", stored[1]["event_id"].get<std::string>());
    EXPECT_DOUBLE_EQ(-25.0, stored[1]["amount"].get<double>());
}

TEST_F(GameAnalyticsApiTest, AddProgressionEventTracksAttempts)
{
    initializeSdk();

    GameAnalytics::addProgressionEvent(EGAProgressionStatus::Fail, "world1", "level2");
    GameAnalytics::addProgressionEvent(EGAProgressionStatus::Fail, "world1", "level2");
    GameAnalytics::addProgressionEvent(EGAProgressionStatus::Complete, 100, "world1", "level2");
    ASSERT_TRUE(drainGAThread());

    auto stored = storedEvents("progression");
    ASSERT_EQ(3u, stored.size());

    const json& complete = stored[2];
    EXPECT_EQ("Complete:world1:level2", complete["event_id"].get<std::string>());
    EXPECT_EQ(100, complete["score"].get<int>());
    EXPECT_EQ(3, complete["attempt_num"].get<int>());
    EXPECT_EQ(0, state::GAState::getProgressionTries("world1:level2"));
}

TEST_F(GameAnalyticsApiTest, AddErrorEventStoresSeverityAndTrimsMessage)
{
    initializeSdk();

    const std::string longMessage(9000, 'x');
    GameAnalytics::addErrorEvent(EGAErrorSeverity::Critical, longMessage);
    ASSERT_TRUE(drainGAThread());

    auto stored = storedEvents("error");
    ASSERT_EQ(1u, stored.size());
    EXPECT_EQ("critical", stored[0]["severity"].get<std::string>());
    EXPECT_EQ(8182u, stored[0]["message"].get<std::string>().size());
}

TEST_F(GameAnalyticsApiTest, EventsBeforeInitializeAreNotStored)
{
    GameAnalytics::addDesignEvent("too:early");
    GameAnalytics::addBusinessEvent("USD", 100, "weapon", "sword", "shop");
    GameAnalytics::addErrorEvent(EGAErrorSeverity::Info, "too early");
    ASSERT_TRUE(drainGAThread());

    EXPECT_TRUE(storedEvents().empty());
}

TEST_F(GameAnalyticsApiTest, OversizedCustomFieldsRejectTheEvent)
{
    initializeSdk();

    const std::string oversizedFields = R"({"k":")" + std::string(5000, 'v') + R"("})";
    GameAnalytics::addDesignEvent("a:b", oversizedFields);
    GameAnalytics::addProgressionEvent(EGAProgressionStatus::Start, "world1", "", "", oversizedFields);
    GameAnalytics::addErrorEvent(EGAErrorSeverity::Info, "message", oversizedFields);
    ASSERT_TRUE(drainGAThread());

    EXPECT_TRUE(storedEvents("design").empty());
    EXPECT_TRUE(storedEvents("progression").empty());
    EXPECT_TRUE(storedEvents("error").empty());
}

// ---- state changes while running ----

TEST_F(GameAnalyticsApiTest, SetCustomDimensionsAreValidatedAgainstAvailable)
{
    initializeSdk();

    GameAnalytics::setCustomDimension01("ninja");
    GameAnalytics::setCustomDimension02("guild_a");
    GameAnalytics::setCustomDimension03("tier1");
    ASSERT_TRUE(drainGAThread());

    EXPECT_EQ("ninja", state::GAState::getCurrentCustomDimension01());
    EXPECT_EQ("guild_a", state::GAState::getCurrentCustomDimension02());
    EXPECT_EQ("tier1", state::GAState::getCurrentCustomDimension03());

    GameAnalytics::setCustomDimension01("pirate");
    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ("ninja", state::GAState::getCurrentCustomDimension01());

    GameAnalytics::setCustomDimension01("");
    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ("", state::GAState::getCurrentCustomDimension01());
}

TEST_F(GameAnalyticsApiTest, GlobalCustomEventFieldsAreMergedIntoEvents)
{
    initializeSdk();

    GameAnalytics::setGlobalCustomEventFields(R"({"team":"red","run":7})");
    ASSERT_TRUE(drainGAThread());

    GameAnalytics::addDesignEvent("uses:globals");
    GameAnalytics::addDesignEvent("overrides:globals", R"({"team":"blue"})");
    ASSERT_TRUE(drainGAThread());

    auto stored = storedEvents("design");
    ASSERT_EQ(2u, stored.size());
    EXPECT_EQ("red", stored[0]["custom_fields"]["team"].get<std::string>());
    EXPECT_EQ(7, stored[0]["custom_fields"]["run"].get<int>());
    EXPECT_EQ("blue", stored[1]["custom_fields"]["team"].get<std::string>());
}

TEST_F(GameAnalyticsApiTest, EventSubmissionToggleThroughFacade)
{
    initializeSdk();

    GameAnalytics::setEnabledEventSubmission(false);
    ASSERT_TRUE(drainGAThread());
    EXPECT_FALSE(state::GAState::isEventSubmissionEnabled());

    GameAnalytics::addDesignEvent("blocked:event");
    ASSERT_TRUE(drainGAThread());
    EXPECT_TRUE(storedEvents("design").empty());

    GameAnalytics::setEnabledEventSubmission(true);
    ASSERT_TRUE(drainGAThread());
    EXPECT_TRUE(state::GAState::isEventSubmissionEnabled());

    GameAnalytics::addDesignEvent("allowed:event");
    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(1u, storedEvents("design").size());
}

TEST_F(GameAnalyticsApiTest, LoggingAndErrorReportingToggles)
{
    GameAnalytics::setEnabledInfoLog(true);
    GameAnalytics::setEnabledVerboseLog(true);
    GameAnalytics::setEnabledErrorReporting(false);
    ASSERT_TRUE(drainGAThread());
    EXPECT_FALSE(state::GAState::useErrorReporting());

    GameAnalytics::setEnabledInfoLog(false);
    GameAnalytics::setEnabledVerboseLog(false);
    GameAnalytics::setEnabledErrorReporting(true);
    ASSERT_TRUE(drainGAThread());
    EXPECT_TRUE(state::GAState::useErrorReporting());
}

// ---- session lifecycle ----

TEST_F(GameAnalyticsApiTest, AutomaticSessionHandlingOnSuspendAndResume)
{
    initializeSdk();
    const std::string firstSessionId = state::GAState::getSessionId();

    // fail the http dispatch so the session end event stays queued in the store
    setHttpResponse(-1, "");

    GameAnalytics::onSuspend();
    ASSERT_TRUE(drainGAThread());
    EXPECT_FALSE(state::GAState::sessionIsStarted());
    EXPECT_EQ(1u, storedSessionEndCount(firstSessionId));
    EXPECT_GE(GameAnalytics::getElapsedTimeForPreviousSession(), 0);

    GameAnalytics::onResume();
    ASSERT_TRUE(drainGAThread());
    EXPECT_TRUE(state::GAState::sessionIsStarted());
    EXPECT_NE(firstSessionId, state::GAState::getSessionId());

    // resuming an already running session must not start another one
    const std::string currentSessionId = state::GAState::getSessionId();
    GameAnalytics::onResume();
    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(currentSessionId, state::GAState::getSessionId());
}

TEST_F(GameAnalyticsApiTest, ManualSessionHandlingControlsSessionExplicitly)
{
    initializeSdk();
    GameAnalytics::setEnabledManualSessionHandling(true);
    ASSERT_TRUE(drainGAThread());
    ASSERT_TRUE(state::GAState::useManualSessionHandling());

    const std::string firstSessionId = state::GAState::getSessionId();

    // fail the http dispatch so the session end event stays queued in the store
    setHttpResponse(-1, "");

    GameAnalytics::endSession();
    ASSERT_TRUE(drainGAThread());
    EXPECT_FALSE(state::GAState::sessionIsStarted());
    EXPECT_EQ(1u, storedSessionEndCount(firstSessionId));

    GameAnalytics::startSession();
    ASSERT_TRUE(drainGAThread());
    EXPECT_TRUE(state::GAState::sessionIsStarted());
    EXPECT_NE(firstSessionId, state::GAState::getSessionId());
}

// ---- remote configs ----

TEST_F(GameAnalyticsApiTest, RemoteConfigsFromInitReachListenerAndGetters)
{
    auto listener = std::make_shared<RecordingConfigsListener>();
    GameAnalytics::addRemoteConfigsListener(listener);

    const json initResponse = {
        {"server_ts", utilities::getTimestamp()},
        {"configs", json::array({
            {{"key", "speed"}, {"value", "fast"}, {"id", "cfg1"}, {"vsn", 1}}
        })},
        {"configs_hash", "hash-1"},
        {"ab_id", "ab1"},
        {"ab_variant_id", "var1"}
    };
    setHttpResponse(201, initResponse.dump());
    initializeSdk();

    EXPECT_TRUE(GameAnalytics::isRemoteConfigsReady());
    EXPECT_EQ("fast", GameAnalytics::getRemoteConfigsValueAsString("speed"));
    EXPECT_EQ("slow", GameAnalytics::getRemoteConfigsValueAsString("missing", "slow"));
    EXPECT_NE(std::string::npos, GameAnalytics::getRemoteConfigsContentAsString().find("speed"));
    EXPECT_EQ("ab1", GameAnalytics::getABTestingId());
    EXPECT_EQ("var1", GameAnalytics::getABTestingVariantId());
    EXPECT_EQ("hash-1", state::GAStateTestAccessor::configsHash());

    auto updates = listener->updates();
    ASSERT_EQ(1u, updates.size());
    EXPECT_NE(std::string::npos, updates[0].find("speed"));

    // a removed listener is not notified by the next config refresh
    GameAnalytics::removeRemoteConfigsListener(listener);
    GameAnalytics::onSuspend();
    GameAnalytics::onResume();
    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(1u, listener->updates().size());
}

TEST_F(GameAnalyticsApiTest, RemoteConfigsFallBackToDefaultsWhenAbsent)
{
    initializeSdk();

    EXPECT_TRUE(GameAnalytics::isRemoteConfigsReady());
    EXPECT_EQ("fallback", GameAnalytics::getRemoteConfigsValueAsString("absent", "fallback"));
    EXPECT_EQ("", GameAnalytics::getRemoteConfigsValueAsJson("absent"));
}

// ---- health tracking facade ----

TEST_F(GameAnalyticsApiTest, HealthTrackingtogglesAreForwardedToTracker)
{
    initializeSdk();

    GAHealth* tracker = device::GADevice::getHealthTracker();
    ASSERT_NE(nullptr, tracker);

    GameAnalytics::enableSDKInitEvent(true);
    GameAnalytics::enableMemoryHistogram(true);
    GameAnalytics::enableFPSHistogram([]() { return 60.0f; }, true);
    GameAnalytics::enableHardwareTracking(true);

    EXPECT_TRUE(tracker->enableAppBootTimeTracking);
    EXPECT_TRUE(tracker->enableMemoryTracking);
    EXPECT_TRUE(tracker->enableFPSTracking);
    EXPECT_TRUE(tracker->enableHardwareTracking);
}
