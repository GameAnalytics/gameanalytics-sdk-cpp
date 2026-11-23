//
// GA-SDK-CPP
// Integration tests for the server-side facade: player sessions, callbacks
// and player events, driving the real GA thread against a mock HTTP client
//

#include <gtest/gtest.h>

#include "GameAnalytics/Server/GAServer.h"
#include "GameAnalytics/GAHttpClient.h"

#include <GAState.h>
#include <GAStore.h>
#include <GAEvents.h>
#include <GAHTTPApi.h>
#include <GAThreading.h>
#include <GAUtilities.h>

#include "helpers/GAStateTestAccessor.h"

#include <chrono>
#include <future>
#include <memory>
#include <mutex>

using namespace gameanalytics;

namespace
{
    constexpr const char* kGameKey    = "bd624ee6f8e6efb32a054f8d7ba11618";
    constexpr const char* kGameSecret = "7f5c3f682cbd217841efba92e92ffb1b3b6612bc";

    struct MockHttpState
    {
        std::mutex mutex;
        GAHttpClient::Response response;
    };

    class MockHttpClient : public GAHttpClient
    {
    public:
        explicit MockHttpClient(std::shared_ptr<MockHttpState> state) : _state(std::move(state)) {}

        void initialize() override {}
        void cleanup() override {}

        Response sendRequest(std::string const&, std::string const&, std::vector<uint8_t> const&, bool, void*) override
        {
            std::lock_guard<std::mutex> lock(_state->mutex);
            return _state->response;
        }

    private:
        std::shared_ptr<MockHttpState> _state;
    };

    struct RecordingCallbacks : public PlayerCallbacks
    {
        int newPlayer = 0;
        int joined    = 0;
        int exited    = 0;
        bool newUserFlagInNewPlayer = false;

        void onNewPlayer(Player& player) override
        {
            ++newPlayer;
            newUserFlagInNewPlayer = player.isNewUser();
        }

        void onPlayerJoin(Player&) override { ++joined; }
        void onPlayerExit(Player&) override { ++exited; }
    };

    class GameAnalyticsServerTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            events::GAEvents::stopEventQueue();
            state::GAStateTestAccessor::forceUninitialized();
            state::GAState::setEnabledEventSubmission(true);

            ASSERT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));
            clearStore();

            http = std::make_shared<MockHttpState>();
            setHttpResponse(200, json{
                {"server_ts", utilities::GAUtilities::timeIntervalSince1970()},
                {"configs_hash", "hash1"},
                {"ab_id", "ab1"},
                {"ab_variant_id", "var1"}
            }.dump());
            GameAnalytics::configureHttpClient<MockHttpClient>(http);

            server    = std::make_unique<GameAnalyticsServer>("server_1", "ext_server_1", "1.0.0");
            callbacks = std::make_shared<RecordingCallbacks>();
            server->setPlayerCallbacks(callbacks);
            server->configureAvailableCurrencyTypes({"gems", "gold"});
            server->configureAvailableResourceItemTypes({"boost", "weapon"});
            GameAnalytics::configureAvailableCustomDimensions01({"ninja", "samurai"});
            server->initialize(kGameKey, kGameSecret);
            ASSERT_TRUE(drainGAThread());
            ASSERT_TRUE(state::GAState::isInitialized());
            events::GAEvents::stopEventQueue();
        }

        void TearDown() override
        {
            EXPECT_TRUE(drainGAThread());
            events::GAEvents::stopEventQueue();
            state::GAState::setEnabledEventSubmission(false);
            http::GAHTTPApi::setCustomHttpImpl(nullptr);
            state::GAStateTestAccessor::forceUninitialized();
            clearStore();
        }

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

        static void clearStore()
        {
            store::GAStore::executeQuerySync("DELETE FROM ga_events;");
            store::GAStore::executeQuerySync("DELETE FROM ga_session;");
            store::GAStore::executeQuerySync("DELETE FROM ga_players;");
        }

        static std::vector<json> storedEvents(std::string const& category, std::string const& userId)
        {
            json rows;
            store::GAStore::executeQuerySync("SELECT event FROM ga_events WHERE category='" + category + "';", rows);

            std::vector<json> events;
            if (rows.is_array())
            {
                for (auto& row : rows)
                {
                    json ev = json::parse(row["event"].get<std::string>());
                    if (ev.value("user_id", "") == userId)
                    {
                        events.push_back(std::move(ev));
                    }
                }
            }
            return events;
        }

        // starts a session and waits for it so the player exists for the userId overloads
        void startSession(std::string const& userId)
        {
            server->startPlayerSession(userId);
            ASSERT_TRUE(drainGAThread());
            ASSERT_TRUE(server->isExistingPlayer(userId));
        }

        static CustomFields fields(std::string const& key, std::string const& value)
        {
            CustomFields f;
            f.setString(key, value);
            return f;
        }

        std::shared_ptr<MockHttpState> http;
        std::unique_ptr<GameAnalyticsServer> server;
        std::shared_ptr<RecordingCallbacks> callbacks;
    };
}

TEST_F(GameAnalyticsServerTest, OnNewPlayerFiresOnlyForTheFirstSession)
{
    server->startPlayerSession("p1");
    ASSERT_TRUE(drainGAThread());

    EXPECT_EQ(1, callbacks->newPlayer);
    EXPECT_EQ(1, callbacks->joined);
    EXPECT_TRUE(callbacks->newUserFlagInNewPlayer);
    EXPECT_TRUE(server->getPlayer("p1").isNewUser());

    server->endPlayerSession("p1");
    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(1, callbacks->exited);
    EXPECT_FALSE(server->getPlayer("p1").isNewUser());

    server->startPlayerSession("p1");
    ASSERT_TRUE(drainGAThread());

    EXPECT_EQ(1, callbacks->newPlayer);
    EXPECT_EQ(2, callbacks->joined);
    EXPECT_FALSE(server->getPlayer("p1").isNewUser());
    EXPECT_EQ(2, server->getPlayer("p1").getSessionCount());
}

TEST_F(GameAnalyticsServerTest, EndingASessionThatWasNeverStartedIsIgnored)
{
    ASSERT_TRUE(server->addPlayer(Player("p2", "")));

    server->endPlayerSession("p2");
    ASSERT_TRUE(drainGAThread());

    EXPECT_EQ(0, callbacks->exited);
    EXPECT_TRUE(storedEvents("session_end", "p2").empty());
    EXPECT_EQ(0, server->getPlayer("p2").getLastSessionLength());
}

TEST_F(GameAnalyticsServerTest, EndingASessionTwiceEmitsOneSessionEnd)
{
    server->startPlayerSession("p1");
    server->endPlayerSession("p1");
    server->endPlayerSession("p1");
    ASSERT_TRUE(drainGAThread());

    EXPECT_EQ(1, callbacks->exited);
    EXPECT_EQ(1u, storedEvents("session_end", "p1").size());
    EXPECT_FALSE(server->getPlayer("p1").isActive());
}

TEST_F(GameAnalyticsServerTest, PlayerProgressionEventCarriesTheScore)
{
    server->startPlayerSession("p1");
    ASSERT_TRUE(drainGAThread());
    server->addPlayerProgressionEvent("p1", EGAProgressionStatus::Complete, "w1", "l1", "", 1500, true);
    ASSERT_TRUE(drainGAThread());

    std::vector<json> events = storedEvents("progression", "p1");
    ASSERT_EQ(1u, events.size());
    EXPECT_EQ("Complete:w1:l1", events[0].value("event_id", ""));
    EXPECT_EQ(1500, events[0].value("score", -1));
    EXPECT_EQ(1, events[0].value("attempt_num", -1));
}

TEST_F(GameAnalyticsServerTest, PlayerProgressionEventOmitsScoreWhenNotRequested)
{
    server->startPlayerSession("p1");
    ASSERT_TRUE(drainGAThread());
    server->addPlayerProgressionEvent("p1", EGAProgressionStatus::Fail, "w1", "l1", "", 7, false);
    ASSERT_TRUE(drainGAThread());

    std::vector<json> events = storedEvents("progression", "p1");
    ASSERT_EQ(1u, events.size());
    EXPECT_EQ("Fail:w1:l1", events[0].value("event_id", ""));
    EXPECT_FALSE(events[0].contains("score"));
    EXPECT_EQ(1, server->getPlayer("p1").progressionTries.getTries("w1:l1"));
}

// ---- player event payloads ----

TEST_F(GameAnalyticsServerTest, PlayerEventsCarryThePlayerAnnotations)
{
    startSession("p1");

    Player& p = server->getPlayer("p1");
    p.extUserId   = "steam_1";
    p.countryCode = "dk";
    p.device      = "dedicated";
    p.setCustomDimension1("ninja");
    p.customFields.setInt("level", 12);

    server->addPlayerDesignEvent("p1", "menu:open");
    ASSERT_TRUE(drainGAThread());

    std::vector<json> events = storedEvents("design", "p1");
    ASSERT_EQ(1u, events.size());
    const json& ev = events[0];

    EXPECT_EQ(2, ev.value("v", 0));
    EXPECT_FALSE(ev.value("event_uuid", "").empty());
    EXPECT_FALSE(ev.value("sdk_version", "").empty());
    EXPECT_EQ("steam_1", ev.value("user_id_ext", ""));
    EXPECT_EQ("dk", ev.value("country_code", ""));
    EXPECT_EQ("dedicated", ev.value("device", ""));
    EXPECT_EQ("ninja", ev.value("custom_01", ""));
    EXPECT_EQ(1, ev.value("session_num", 0));
    EXPECT_EQ("1.0.0", ev.value("build", ""));

    ASSERT_TRUE(ev.contains("custom_fields"));
    EXPECT_EQ(12, ev["custom_fields"].value("level", 0));
}

TEST_F(GameAnalyticsServerTest, PlayerDesignEventPayload)
{
    startSession("p1");
    server->getPlayer("p1").customFields.setString("tier", "gold");

    server->addPlayerDesignEvent("p1", "shop:buy:potion", 2.5, fields("source", "quest"));
    ASSERT_TRUE(drainGAThread());

    std::vector<json> events = storedEvents("design", "p1");
    ASSERT_EQ(1u, events.size());
    const json& ev = events[0];

    EXPECT_EQ("design", ev.value("category", ""));
    EXPECT_EQ("shop:buy:potion", ev.value("event_id", ""));
    EXPECT_DOUBLE_EQ(2.5, ev.value("value", 0.0));

    // event fields are merged on top of the player's own fields
    ASSERT_TRUE(ev.contains("custom_fields"));
    EXPECT_EQ("quest", ev["custom_fields"].value("source", ""));
    EXPECT_EQ("gold", ev["custom_fields"].value("tier", ""));
}

TEST_F(GameAnalyticsServerTest, PlayerDesignEventWithInvalidIdIsRejected)
{
    startSession("p1");

    server->addPlayerDesignEvent("p1", "a:b:c:d:e:f");  // more than 5 parts
    ASSERT_TRUE(drainGAThread());

    EXPECT_TRUE(storedEvents("design", "p1").empty());
}

TEST_F(GameAnalyticsServerTest, PlayerBusinessEventPayload)
{
    startSession("p1");

    server->addPlayerBusinessEvent("p1", "USD", 499, "iap", "starter_pack", "shop", fields("promo", "launch"));
    server->addPlayerBusinessEvent("p1", "EUR", 100, "iap", "coins_small", "");
    ASSERT_TRUE(drainGAThread());

    std::vector<json> events = storedEvents("business", "p1");
    ASSERT_EQ(2u, events.size());

    const json& first = events[0];
    EXPECT_EQ("business", first.value("category", ""));
    EXPECT_EQ("iap:starter_pack", first.value("event_id", ""));
    EXPECT_EQ("USD", first.value("currency", ""));
    EXPECT_EQ(499, first.value("amount", 0));
    EXPECT_EQ("shop", first.value("cart_type", ""));
    ASSERT_TRUE(first.contains("custom_fields"));
    EXPECT_EQ("launch", first["custom_fields"].value("promo", ""));

    const json& second = events[1];
    EXPECT_EQ("iap:coins_small", second.value("event_id", ""));
    EXPECT_FALSE(second.contains("cart_type"));

    // transaction numbers are per player and strictly increasing
    ASSERT_TRUE(first.contains("transaction_num"));
    ASSERT_TRUE(second.contains("transaction_num"));
    EXPECT_EQ(first["transaction_num"].get<int64_t>() + 1, second["transaction_num"].get<int64_t>());
}

TEST_F(GameAnalyticsServerTest, PlayerBusinessEventWithInvalidCurrencyIsRejected)
{
    startSession("p1");

    server->addPlayerBusinessEvent("p1", "NOT_A_CURRENCY", 100, "iap", "coins", "shop");
    ASSERT_TRUE(drainGAThread());

    EXPECT_TRUE(storedEvents("business", "p1").empty());
}

TEST_F(GameAnalyticsServerTest, PlayerResourceEventPayload)
{
    startSession("p1");

    server->addPlayerResourceEvent("p1", EGAResourceFlowType::Source, "gems", 50.0f, "boost", "double_xp", fields("from", "daily"));
    server->addPlayerResourceEvent("p1", EGAResourceFlowType::Sink, "gold", 25.0f, "weapon", "sword");
    ASSERT_TRUE(drainGAThread());

    std::vector<json> events = storedEvents("resource", "p1");
    ASSERT_EQ(2u, events.size());

    const json& source = events[0];
    EXPECT_EQ("resource", source.value("category", ""));
    EXPECT_EQ("Source:gems:boost:double_xp", source.value("event_id", ""));
    EXPECT_FLOAT_EQ(50.0f, source.value("amount", 0.0f));
    ASSERT_TRUE(source.contains("custom_fields"));
    EXPECT_EQ("daily", source["custom_fields"].value("from", ""));

    // sink amounts are negated on the wire
    const json& sink = events[1];
    EXPECT_EQ("Sink:gold:weapon:sword", sink.value("event_id", ""));
    EXPECT_FLOAT_EQ(-25.0f, sink.value("amount", 0.0f));
}

TEST_F(GameAnalyticsServerTest, PlayerResourceEventWithUnknownCurrencyOrItemTypeIsRejected)
{
    startSession("p1");

    server->addPlayerResourceEvent("p1", EGAResourceFlowType::Source, "diamonds", 5.0f, "boost", "x");
    server->addPlayerResourceEvent("p1", EGAResourceFlowType::Source, "gems", 5.0f, "armor", "x");
    server->addPlayerResourceEvent("p1", EGAResourceFlowType::Source, "gems", 0.0f, "boost", "x");
    ASSERT_TRUE(drainGAThread());

    EXPECT_TRUE(storedEvents("resource", "p1").empty());
}

TEST_F(GameAnalyticsServerTest, PlayerErrorEventPayload)
{
    startSession("p1");

    server->addPlayerErrorEvent("p1", EGAErrorSeverity::Critical, "null deref in matchmaking", fields("map", "arena_2"));
    server->addPlayerErrorEvent("p1", EGAErrorSeverity::Info, "");
    ASSERT_TRUE(drainGAThread());

    std::vector<json> events = storedEvents("error", "p1");
    ASSERT_EQ(2u, events.size());

    const json& critical = events[0];
    EXPECT_EQ("error", critical.value("category", ""));
    EXPECT_EQ("critical", critical.value("severity", ""));
    EXPECT_EQ("null deref in matchmaking", critical.value("message", ""));
    ASSERT_TRUE(critical.contains("custom_fields"));
    EXPECT_EQ("arena_2", critical["custom_fields"].value("map", ""));

    const json& info = events[1];
    EXPECT_EQ("info", info.value("severity", ""));
    EXPECT_EQ("", info.value("message", "missing"));
    EXPECT_FALSE(info.contains("custom_fields"));
}

TEST_F(GameAnalyticsServerTest, PlayerErrorEventWithTooLongMessageIsRejected)
{
    startSession("p1");

    server->addPlayerErrorEvent("p1", EGAErrorSeverity::Error, std::string(8193, 'x'));
    ASSERT_TRUE(drainGAThread());

    EXPECT_TRUE(storedEvents("error", "p1").empty());
}

TEST_F(GameAnalyticsServerTest, PlayerEventsForUnknownPlayersAreDropped)
{
    server->addPlayerDesignEvent("ghost", "menu:open");
    server->addPlayerBusinessEvent("ghost", "USD", 1, "iap", "x", "");
    server->addPlayerResourceEvent("ghost", EGAResourceFlowType::Source, "gems", 1.0f, "boost", "x");
    server->addPlayerErrorEvent("ghost", EGAErrorSeverity::Error, "boom");
    ASSERT_TRUE(drainGAThread());

    EXPECT_TRUE(storedEvents("design", "ghost").empty());
    EXPECT_TRUE(storedEvents("business", "ghost").empty());
    EXPECT_TRUE(storedEvents("resource", "ghost").empty());
    EXPECT_TRUE(storedEvents("error", "ghost").empty());
    EXPECT_FALSE(server->isExistingPlayer("ghost"));
}

TEST_F(GameAnalyticsServerTest, PlayerEventsAreKeptSeparatePerPlayer)
{
    startSession("p1");
    startSession("p2");

    server->addPlayerDesignEvent("p1", "level:start");
    server->addPlayerDesignEvent("p2", "level:start");
    server->addPlayerDesignEvent("p2", "level:end");
    ASSERT_TRUE(drainGAThread());

    EXPECT_EQ(1u, storedEvents("design", "p1").size());
    EXPECT_EQ(2u, storedEvents("design", "p2").size());
    EXPECT_TRUE(storedEvents("design", "server_1").empty());
}

// ---- remote configs per player ----

TEST_F(GameAnalyticsServerTest, SessionStartDeliversRemoteConfigsToThePlayer)
{
    setHttpResponse(201, json{
        {"server_ts", utilities::GAUtilities::timeIntervalSince1970()},
        {"configs_hash", "hash_p1"},
        {"ab_id", "exp_a"},
        {"ab_variant_id", "variant_b"},
        {"configs", json::array({
            {{"key", "spawn_rate"}, {"value", "0.25"}, {"id", "cfg_1"}, {"vsn", 3}},
            {{"key", "hardcore"},   {"value", "true"}, {"id", "cfg_2"}, {"vsn", 1}}
        })}
    }.dump());

    startSession("p1");
    Player& p = server->getPlayer("p1");

    EXPECT_TRUE(p.isRemoteConfigsReady());
    EXPECT_EQ("exp_a", p.getABId());
    EXPECT_EQ("variant_b", p.getABVariantId());
    EXPECT_EQ("hash_p1", p.getConfigsHash());
    EXPECT_DOUBLE_EQ(0.25, p.getRemoteConfigsValueAsFloat("spawn_rate"));
    EXPECT_TRUE(p.getRemoteConfigsValueAsBool("hardcore"));

    server->addPlayerDesignEvent("p1", "match:start");
    ASSERT_TRUE(drainGAThread());

    std::vector<json> events = storedEvents("design", "p1");
    ASSERT_EQ(1u, events.size());
    EXPECT_EQ("exp_a", events[0].value("ab_id", ""));
    EXPECT_EQ("variant_b", events[0].value("ab_variant_id", ""));
    ASSERT_TRUE(events[0].contains("configurations_v3"));
    ASSERT_EQ(2u, events[0]["configurations_v3"].size());
    EXPECT_EQ("spawn_rate", events[0]["configurations_v3"][0].value("key", ""));
    EXPECT_EQ("cfg_1", events[0]["configurations_v3"][0].value("id", ""));
    EXPECT_EQ(3, events[0]["configurations_v3"][0].value("vsn", 0));

    // the backend answers 200 without configs when the hash matches: keep what we have
    setHttpResponse(200, json{{"server_ts", utilities::GAUtilities::timeIntervalSince1970()}}.dump());
    server->endPlayerSession("p1");
    ASSERT_TRUE(drainGAThread());
    startSession("p1");

    EXPECT_EQ("hash_p1", server->getPlayer("p1").getConfigsHash());
    EXPECT_DOUBLE_EQ(0.25, server->getPlayer("p1").getRemoteConfigsValueAsFloat("spawn_rate"));
    EXPECT_EQ("exp_a", server->getPlayer("p1").getABId());
}

TEST_F(GameAnalyticsServerTest, PlayerRetentionIsAppliedOnInitialize)
{
    constexpr int64_t DAY = 24 * 60 * 60;
    auto storeAged = [](std::string const& uid, int64_t age)
    {
        const int64_t ts = utilities::GAUtilities::timeIntervalSince1970() - age;
        json j = json::parse(Player(uid, "").serialize());
        j["last_seen_ts"] = ts;
        store::GAStore::setPlayer(uid, j.dump(), ts);
    };
    storeAged("stale", 90 * DAY);
    storeAged("recent", 2 * DAY);

    // a second server instance over the same store, with a policy, picks up only the recent player
    GameAnalyticsServer other("server_1", "ext_server_1", "1.0.0");
    other.setPlayerRetention(30, 100);
    other.initialize(kGameKey, kGameSecret);
    ASSERT_TRUE(drainGAThread());

    EXPECT_FALSE(other.isExistingPlayer("stale"));
    EXPECT_TRUE(other.isExistingPlayer("recent"));
    EXPECT_EQ(1, store::GAStore::countPlayers());

    EXPECT_EQ(0u, other.evictPlayersInactiveSince(7));
    EXPECT_EQ(1u, other.evictPlayersInactiveSince(1));
    EXPECT_FALSE(other.isExistingPlayer("recent"));
    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(0, store::GAStore::countPlayers());
}
