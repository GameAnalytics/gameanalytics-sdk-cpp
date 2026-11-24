//
// GA-SDK-CPP
// Tests for the ga_players table in the sqlite store (the raw persistence layer
// under PlayerDatabase): upsert, delete, listing, counting and retention queries
//

#include <gtest/gtest.h>

#include <GAState.h>
#include <GAStore.h>
#include <GAUtilities.h>

#include <map>
#include <string>

using namespace gameanalytics;

namespace
{
    constexpr const char* kGameKey    = "bd624ee6f8e6efb32a054f8d7ba11618";
    constexpr const char* kGameSecret = "7f5c3f682cbd217841efba92e92ffb1b3b6612bc";

    class GAStorePlayersTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            state::GAState::setKeys(kGameKey, kGameSecret);
            ASSERT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));
            ASSERT_TRUE(store::GAStore::getTableReady());
            clear();
        }

        void TearDown() override
        {
            clear();
        }

        static void clear()
        {
            store::GAStore::executeQuerySync("DELETE FROM ga_players;");
            store::GAStore::executeQuerySync("DELETE FROM ga_events;");
        }

        // user_id -> data for every stored row
        static std::map<std::string, std::string> rows()
        {
            json out;
            store::GAStore::getPlayers(out);

            std::map<std::string, std::string> result;
            if (out.is_array())
            {
                for (auto& row : out)
                {
                    result[row.value("user_id", "")] = row.value("data", "");
                }
            }
            return result;
        }

        static int64_t lastSeen(std::string const& uid)
        {
            json out;
            store::GAStore::executeQuerySync("SELECT last_seen FROM ga_players WHERE user_id = ?;", {uid}, out);
            if (out.is_array() && !out.empty())
            {
                return utilities::getOptionalValue<int64_t>(out[0], "last_seen", -1);
            }
            return -1;
        }
    };
}

TEST_F(GAStorePlayersTest, TableStartsEmpty)
{
    EXPECT_EQ(0, store::GAStore::countPlayers());
    EXPECT_TRUE(rows().empty());
}

TEST_F(GAStorePlayersTest, SetPlayerInsertsARow)
{
    store::GAStore::setPlayer("p1", "{\"user_id\":\"p1\"}", 100);

    EXPECT_EQ(1, store::GAStore::countPlayers());
    EXPECT_EQ("{\"user_id\":\"p1\"}", rows()["p1"]);
    EXPECT_EQ(100, lastSeen("p1"));
}

TEST_F(GAStorePlayersTest, SetPlayerReplacesAnExistingRow)
{
    store::GAStore::setPlayer("p1", "v1", 100);
    store::GAStore::setPlayer("p1", "v2", 200);

    EXPECT_EQ(1, store::GAStore::countPlayers());
    EXPECT_EQ("v2", rows()["p1"]);
    EXPECT_EQ(200, lastSeen("p1"));
}

TEST_F(GAStorePlayersTest, SetPlayerIgnoresAnEmptyUserId)
{
    store::GAStore::setPlayer("", "data", 100);
    EXPECT_EQ(0, store::GAStore::countPlayers());
}

TEST_F(GAStorePlayersTest, SetPlayerWithEmptyDataDeletesTheRow)
{
    store::GAStore::setPlayer("p1", "v1", 100);
    store::GAStore::setPlayer("p1", "", 200);

    EXPECT_EQ(0, store::GAStore::countPlayers());
}

TEST_F(GAStorePlayersTest, DataWithQuotesAndUnicodeRoundTrips)
{
    const std::string data = "{\"name\":\"O'Brien \\\"the\\\" ☃\",\"semi\":\"a;b--c\"}";
    store::GAStore::setPlayer("p'1;--", data, 1);

    std::map<std::string, std::string> stored = rows();
    ASSERT_EQ(1u, stored.size());
    EXPECT_EQ(data, stored["p'1;--"]);
}

TEST_F(GAStorePlayersTest, RemovePlayerDeletesOnlyThatRow)
{
    store::GAStore::setPlayer("p1", "d1", 1);
    store::GAStore::setPlayer("p2", "d2", 2);

    store::GAStore::removePlayer("p1");
    store::GAStore::removePlayer("missing");
    store::GAStore::removePlayer("");

    std::map<std::string, std::string> stored = rows();
    EXPECT_EQ(1u, stored.size());
    EXPECT_EQ("d2", stored["p2"]);
}

TEST_F(GAStorePlayersTest, GetPlayersListsUserIdAndData)
{
    store::GAStore::setPlayer("p1", "d1", 1);
    store::GAStore::setPlayer("p2", "d2", 2);
    store::GAStore::setPlayer("p3", "d3", 3);

    json out;
    store::GAStore::getPlayers(out);

    ASSERT_TRUE(out.is_array());
    ASSERT_EQ(3u, out.size());
    for (auto& row : out)
    {
        EXPECT_TRUE(row.contains("user_id"));
        EXPECT_TRUE(row.contains("data"));
        EXPECT_EQ(2u, row.size());
    }

    std::map<std::string, std::string> stored = rows();
    EXPECT_EQ("d1", stored["p1"]);
    EXPECT_EQ("d2", stored["p2"]);
    EXPECT_EQ("d3", stored["p3"]);
}

TEST_F(GAStorePlayersTest, CountPlayersTracksInsertsAndDeletes)
{
    EXPECT_EQ(0, store::GAStore::countPlayers());
    store::GAStore::setPlayer("p1", "d", 1);
    store::GAStore::setPlayer("p2", "d", 1);
    EXPECT_EQ(2, store::GAStore::countPlayers());
    store::GAStore::removePlayer("p1");
    EXPECT_EQ(1, store::GAStore::countPlayers());
}

// ---- retention queries ----

TEST_F(GAStorePlayersTest, EvictPlayersInactiveSinceDropsRowsBeforeTheCutoff)
{
    store::GAStore::setPlayer("old",    "d", 100);
    store::GAStore::setPlayer("border", "d", 200);
    store::GAStore::setPlayer("recent", "d", 300);

    // strict: a row last seen exactly at the cutoff is kept
    EXPECT_EQ(1, store::GAStore::evictPlayersInactiveSince(200));

    std::map<std::string, std::string> stored = rows();
    EXPECT_EQ(0u, stored.count("old"));
    EXPECT_EQ(1u, stored.count("border"));
    EXPECT_EQ(1u, stored.count("recent"));

    EXPECT_EQ(0, store::GAStore::evictPlayersInactiveSince(0));
    EXPECT_EQ(2, store::GAStore::evictPlayersInactiveSince(1000));
    EXPECT_EQ(0, store::GAStore::countPlayers());
}

TEST_F(GAStorePlayersTest, TrimPlayersKeepsTheMostRecentlySeenRows)
{
    store::GAStore::setPlayer("a", "d", 10);
    store::GAStore::setPlayer("b", "d", 40);
    store::GAStore::setPlayer("c", "d", 20);
    store::GAStore::setPlayer("d", "d", 30);

    EXPECT_EQ(2, store::GAStore::trimPlayers(2));

    std::map<std::string, std::string> stored = rows();
    EXPECT_EQ(2u, stored.size());
    EXPECT_EQ(1u, stored.count("b"));
    EXPECT_EQ(1u, stored.count("d"));
}

TEST_F(GAStorePlayersTest, TrimPlayersBreaksTiesByUserId)
{
    store::GAStore::setPlayer("z", "d", 5);
    store::GAStore::setPlayer("a", "d", 5);
    store::GAStore::setPlayer("m", "d", 5);

    EXPECT_EQ(1, store::GAStore::trimPlayers(2));

    std::map<std::string, std::string> stored = rows();
    EXPECT_EQ(1u, stored.count("a"));
    EXPECT_EQ(1u, stored.count("m"));
    EXPECT_EQ(0u, stored.count("z"));
}

TEST_F(GAStorePlayersTest, TrimPlayersIsANoOpWhenUnderTheCap)
{
    store::GAStore::setPlayer("a", "d", 1);
    store::GAStore::setPlayer("b", "d", 2);

    EXPECT_EQ(0, store::GAStore::trimPlayers(2));
    EXPECT_EQ(0, store::GAStore::trimPlayers(5));
    EXPECT_EQ(0, store::GAStore::trimPlayers(-1));
    EXPECT_EQ(2, store::GAStore::countPlayers());

    EXPECT_EQ(2, store::GAStore::trimPlayers(0));
    EXPECT_EQ(0, store::GAStore::countPlayers());
}

// ---- interaction with the rest of the store ----

TEST_F(GAStorePlayersTest, PlayerRowsSurviveReopeningTheDatabase)
{
    store::GAStore::setPlayer("p1", "d1", 7);

    ASSERT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));

    EXPECT_EQ(1, store::GAStore::countPlayers());
    EXPECT_EQ("d1", rows()["p1"]);
    EXPECT_EQ(7, lastSeen("p1"));
}

TEST_F(GAStorePlayersTest, DroppingTheDatabaseClearsPlayers)
{
    store::GAStore::setPlayer("p1", "d1", 7);

    ASSERT_TRUE(store::GAStore::ensureDatabase(true, kGameKey));

    EXPECT_TRUE(store::GAStore::getTableReady());
    EXPECT_EQ(0, store::GAStore::countPlayers());

    // the table is usable again after the drop
    store::GAStore::setPlayer("p2", "d2", 8);
    EXPECT_EQ(1, store::GAStore::countPlayers());
}

TEST_F(GAStorePlayersTest, PlayerRowsAreIndependentOfEventRows)
{
    store::GAStore::setPlayer("p1", "d1", 1);
    store::GAStore::executeQuerySync("INSERT INTO ga_events (status, category, session_id, client_ts, event) VALUES(?, ?, ?, ?, ?);",
        {"new", "design", "s1", "1", "{\"user_id\":\"p1\"}"});

    json events;
    store::GAStore::executeQuerySync("SELECT COUNT(*) AS n FROM ga_events;", events);
    ASSERT_TRUE(events.is_array());
    EXPECT_EQ(1, utilities::getOptionalValue<int64_t>(events[0], "n", 0));

    store::GAStore::executeQuerySync("DELETE FROM ga_players;");
    store::GAStore::executeQuerySync("SELECT COUNT(*) AS n FROM ga_events;", events);
    EXPECT_EQ(1, utilities::getOptionalValue<int64_t>(events[0], "n", 0));

    store::GAStore::setPlayer("p1", "d1", 1);
    store::GAStore::executeQuerySync("DELETE FROM ga_events;");
    EXPECT_EQ(1, store::GAStore::countPlayers());
}
