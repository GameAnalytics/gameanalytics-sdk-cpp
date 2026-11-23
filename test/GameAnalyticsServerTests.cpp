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
                {"server_ts", utilities::getTimestamp()},
                {"configs_hash", "hash1"},
                {"ab_id", "ab1"},
                {"ab_variant_id", "var1"}
            }.dump());
            GameAnalytics::configureHttpClient<MockHttpClient>(http);

            server    = std::make_unique<GameAnalyticsServer>("server_1", "ext_server_1", "1.0.0");
            callbacks = std::make_shared<RecordingCallbacks>();
            server->setPlayerCallbacks(callbacks);
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
