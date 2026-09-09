//
// GA-SDK-CPP
// Tests for the event store and send queue, using a mock HTTP client
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <GAEvents.h>
#include <GAState.h>
#include <GAStore.h>
#include <GAHTTPApi.h>
#include "GameAnalytics/GAHttpClient.h"

using namespace gameanalytics;

namespace
{
    constexpr const char* kGameKey    = "bd624ee6f8e6efb32a054f8d7ba11618";
    constexpr const char* kGameSecret = "7f5c3f682cbd217841efba92e92ffb1b3b6612bc";

    class MockHttpClient : public GAHttpClient
    {
    public:
        void initialize() override {}
        void cleanup() override {}

        Response sendRequest(
            std::string const& url,
            std::string const& auth,
            std::vector<uint8_t> const& payloadData,
            bool useGzip,
            void* userData) override
        {
            lastUrl = url;
            lastAuth = auth;
            lastPayload = payloadData;
            lastUseGzip = useGzip;
            requestCount++;

            return configuredResponse;
        }

        int requestCount = 0;
        std::string lastUrl;
        std::string lastAuth;
        std::vector<uint8_t> lastPayload;
        bool lastUseGzip = false;

        Response configuredResponse = {};
    };

    class GAEventsTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            state::GAState::setKeys(kGameKey, kGameSecret);
            ASSERT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));

            auto mockPtr = std::make_unique<MockHttpClient>();
            mock = mockPtr.get();
            setResponse(200, R"({"status":"ok"})");
            http::GAHTTPApi::setCustomHttpImpl(std::move(mockPtr));

            state::GAState::setEnabledEventSubmission(true);
            state::GAState::internalInitialize();
            events::GAEvents::stopEventQueue();

            clearEvents();
            mock->requestCount = 0;
        }

        void TearDown() override
        {
            clearEvents();
            state::GAState::setEnabledEventSubmission(false);
            http::GAHTTPApi::setCustomHttpImpl(nullptr);
        }

        void setResponse(long code, std::string const& body)
        {
            mock->configuredResponse.code = code;
            mock->configuredResponse.packet.assign(body.begin(), body.end());
        }

        static void clearEvents()
        {
            store::GAStore::executeQuerySync("DELETE FROM ga_events;");
        }

        // parsed event payloads currently in the store, optionally filtered by category
        static std::vector<json> storedEvents(std::string const& category = "", std::string const& status = "")
        {
            std::string sql = "SELECT event FROM ga_events";
            if (!category.empty())
            {
                sql += " WHERE category='" + category + "'";
            }
            if (!status.empty())
            {
                sql += category.empty() ? " WHERE" : " AND";
                sql += " status='" + status + "'";
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

        MockHttpClient* mock = nullptr;
    };
}

// ---- storing events ----

TEST_F(GAEventsTest, testDesignEventIsStoredWithAnnotations)
{
    events::GAEvents::addDesignEvent("level:complete", 42.5, true, json(), false);

    auto stored = storedEvents("design");
    ASSERT_EQ(1u, stored.size());

    const json& ev = stored[0];
    ASSERT_EQ("design", ev["category"].get<std::string>());
    ASSERT_EQ("level:complete", ev["event_id"].get<std::string>());
    ASSERT_DOUBLE_EQ(42.5, ev["value"].get<double>());

    // shared annotations merged in by addEventToStore
    ASSERT_EQ(2, ev["v"].get<int>());
    ASSERT_FALSE(ev["user_id"].get<std::string>().empty());
    ASSERT_FALSE(ev["session_id"].get<std::string>().empty());
    ASSERT_TRUE(ev.contains("client_ts"));
}

TEST_F(GAEventsTest, testDesignEventWithoutValueOmitsValue)
{
    events::GAEvents::addDesignEvent("level:skip", 0.0, false, json(), false);

    auto stored = storedEvents("design");
    ASSERT_EQ(1u, stored.size());
    ASSERT_FALSE(stored[0].contains("value"));
}

TEST_F(GAEventsTest, testInvalidDesignEventIsNotStored)
{
    // event id may have at most 5 segments
    events::GAEvents::addDesignEvent("a:b:c:d:e:f", 0.0, false, json(), false);

    ASSERT_TRUE(storedEvents("design").empty());
}

TEST_F(GAEventsTest, testErrorEventStoresSeverityAndMessage)
{
    events::GAEvents::addErrorEvent(EGAErrorSeverity::Warning, "something happened", "update", 42, json(), false);

    auto stored = storedEvents("error");
    ASSERT_EQ(1u, stored.size());

    const json& ev = stored[0];
    ASSERT_EQ("warning", ev["severity"].get<std::string>());
    ASSERT_EQ("something happened", ev["message"].get<std::string>());
    ASSERT_EQ("update", ev["function_name"].get<std::string>());
    ASSERT_EQ(42, ev["line_number"].get<int>());
}

TEST_F(GAEventsTest, testBusinessEventIncrementsTransactionNum)
{
    const int64_t before = state::GAState::getTransactionNum();

    events::GAEvents::addBusinessEvent("USD", 499, "weapon", "sword", "shop", json(), false);
    events::GAEvents::addBusinessEvent("USD", 199, "weapon", "shield", "shop", json(), false);

    auto stored = storedEvents("business");
    ASSERT_EQ(2u, stored.size());

    ASSERT_EQ("weapon:sword", stored[0]["event_id"].get<std::string>());
    ASSERT_EQ(499, stored[0]["amount"].get<int>());
    ASSERT_EQ("USD", stored[0]["currency"].get<std::string>());
    ASSERT_EQ(before + 1, stored[0]["transaction_num"].get<int64_t>());
    ASSERT_EQ(before + 2, stored[1]["transaction_num"].get<int64_t>());
}

TEST_F(GAEventsTest, testProgressionCompleteCarriesAttemptNum)
{
    const json noFields;

    events::GAEvents::addProgressionEvent(EGAProgressionStatus::Fail, "world1", "level2", "", 0, false, noFields, false);
    events::GAEvents::addProgressionEvent(EGAProgressionStatus::Fail, "world1", "level2", "", 0, false, noFields, false);
    events::GAEvents::addProgressionEvent(EGAProgressionStatus::Complete, "world1", "level2", "", 100, true, noFields, false);

    auto stored = storedEvents("progression");
    ASSERT_EQ(3u, stored.size());

    const json& complete = stored[2];
    ASSERT_EQ("Complete:world1:level2", complete["event_id"].get<std::string>());
    ASSERT_EQ(3, complete["attempt_num"].get<int>());
    ASSERT_EQ(100, complete["score"].get<int>());

    // completing clears the attempt counter
    ASSERT_EQ(0, state::GAState::getProgressionTries("world1:level2"));
}

// ---- sending events ----

TEST_F(GAEventsTest, testProcessEventsSendsBatchAndClearsQueue)
{
    events::GAEvents::addDesignEvent("send:one", 0.0, false, json(), false);
    events::GAEvents::addDesignEvent("send:two", 0.0, false, json(), false);

    setResponse(200, R"({"status":"ok"})");
    events::GAEvents::processEvents("design", false);

    ASSERT_EQ(1, mock->requestCount);
    ASSERT_NE(std::string::npos, mock->lastUrl.find(kGameKey));
    ASSERT_NE(std::string::npos, mock->lastUrl.find("/events"));
    ASSERT_EQ(0u, mock->lastAuth.find("Authorization: "));
    ASSERT_FALSE(mock->lastPayload.empty());

    // sent events are removed from the store
    ASSERT_TRUE(storedEvents("design").empty());
}

TEST_F(GAEventsTest, testProcessEventsKeepsEventsWhenNoResponse)
{
    events::GAEvents::addDesignEvent("retry:later", 0.0, false, json(), false);

    setResponse(-1, "");
    events::GAEvents::processEvents("design", false);

    ASSERT_EQ(1, mock->requestCount);

    // events go back to 'new' so the next flush retries them
    ASSERT_EQ(1u, storedEvents("design", "new").size());
}

TEST_F(GAEventsTest, testProcessEventsDropsEventsOnServerError)
{
    events::GAEvents::addDesignEvent("dropped:event", 0.0, false, json(), false);

    setResponse(500, "Internal Server Error");
    events::GAEvents::processEvents("design", false);

    ASSERT_EQ(1, mock->requestCount);

    // any answer other than no-response counts as processed
    ASSERT_TRUE(storedEvents("design").empty());
}

TEST_F(GAEventsTest, testProcessEventsWithNoEventsSendsNothing)
{
    events::GAEvents::processEvents("design", false);

    ASSERT_EQ(0, mock->requestCount);
}

TEST_F(GAEventsTest, testEventsAreNotStoredWhenSubmissionDisabled)
{
    state::GAState::setEnabledEventSubmission(false);

    events::GAEvents::addDesignEvent("blocked:event", 0.0, false, json(), false);
    events::GAEvents::addErrorEvent(EGAErrorSeverity::Error, "blocked", "", -1, json(), false);

    state::GAState::setEnabledEventSubmission(true);
    ASSERT_TRUE(storedEvents().empty());
}
