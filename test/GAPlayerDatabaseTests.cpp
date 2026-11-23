//
// GA-SDK-CPP
// Tests for server-side player persistence in the sqlite store
//

#include <gtest/gtest.h>

#include <future>
#include <chrono>

#include <GAState.h>
#include <GAStore.h>
#include <GAThreading.h>
#include <Server/GAPlayerDatabase.h>
#include "GameAnalytics/Server/GAPlayer.h"

using namespace gameanalytics;

namespace
{
    constexpr const char* kGameKey    = "bd624ee6f8e6efb32a054f8d7ba11618";
    constexpr const char* kGameSecret = "7f5c3f682cbd217841efba92e92ffb1b3b6612bc";

    class GAPlayerDatabaseTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            state::GAState::setKeys(kGameKey, kGameSecret);
            ASSERT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));
            clearPlayers();
        }

        void TearDown() override
        {
            ASSERT_TRUE(drainGAThread());
            clearPlayers();
        }

        static void clearPlayers()
        {
            store::GAStore::executeQuerySync("DELETE FROM ga_players;");
        }

        // the GA thread runs queued blocks in FIFO order, so once this marker
        // task has run every previously queued store write has run too
        [[nodiscard]] static bool drainGAThread()
        {
            std::promise<void> done;
            std::future<void> drained = done.get_future();
            threading::GAThreading::performTaskOnGAThread([&done]() { done.set_value(); });
            return drained.wait_for(std::chrono::seconds(10)) == std::future_status::ready;
        }

        static json storedRows()
        {
            json rows;
            store::GAStore::getPlayers(rows);
            return rows;
        }

        static Player makePlayer(std::string const& uid)
        {
            Player p(uid, "ext_" + uid);
            p.countryCode = "dk";
            p.device      = "server";
            p.customFields.setInt("level", 7);
            p.customFields.setString("guild", "red");
            p.progressionTries.setTries("w1:l1", 3);
            return p;
        }
    };
}

TEST_F(GAPlayerDatabaseTest, PlayerSerializationRoundTrip)
{
    Player p = makePlayer("user_rt");
    p.onJoin();
    p.onExit();
    int tx = p.getNextTransactionNum();
    (void)tx;

    const std::string data = p.serialize();
    ASSERT_FALSE(data.empty());

    Player restored;
    ASSERT_TRUE(restored.deserialize(data));

    EXPECT_EQ("user_rt", restored.getUserId());
    EXPECT_EQ("ext_user_rt", restored.extUserId);
    EXPECT_EQ("dk", restored.countryCode);
    EXPECT_EQ("server", restored.device);
    EXPECT_EQ(1, restored.getSessionCount());
    EXPECT_EQ(3, restored.progressionTries.getTries("w1:l1"));
    EXPECT_EQ(7, restored.customFields.getValue<int64_t>("level", 0));
    EXPECT_EQ("red", restored.customFields.getValue<std::string>("guild", ""));
    EXPECT_FALSE(restored.isActive());

    // transaction counter continues where it left off
    EXPECT_EQ(1, restored.getNextTransactionNum());
}

TEST_F(GAPlayerDatabaseTest, AddPlayerIsWrittenToStoreAndRestored)
{
    {
        PlayerDatabase db;
        ASSERT_TRUE(db.addPlayer(makePlayer("user_a")));
        ASSERT_TRUE(db.addPlayer(makePlayer("user_b")));
    }

    ASSERT_TRUE(drainGAThread());
    ASSERT_EQ(2u, storedRows().size());

    PlayerDatabase fresh;
    EXPECT_FALSE(fresh.playerExists("user_a"));

    EXPECT_EQ(2u, fresh.loadFromStore());
    EXPECT_TRUE(fresh.isStoreLoaded());
    ASSERT_TRUE(fresh.playerExists("user_a"));
    ASSERT_TRUE(fresh.playerExists("user_b"));

    Player& a = fresh.getPlayer("user_a");
    EXPECT_EQ("ext_user_a", a.extUserId);
    EXPECT_EQ("dk", a.countryCode);
    EXPECT_EQ(3, a.progressionTries.getTries("w1:l1"));
    EXPECT_EQ(7, a.customFields.getValue<int64_t>("level", 0));
}

TEST_F(GAPlayerDatabaseTest, RemovePlayerDeletesStoredRow)
{
    PlayerDatabase db;
    ASSERT_TRUE(db.addPlayer(makePlayer("user_rm")));
    ASSERT_TRUE(drainGAThread());
    ASSERT_EQ(1u, storedRows().size());

    ASSERT_TRUE(db.removePlayer("user_rm"));
    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(0u, storedRows().size());

    PlayerDatabase fresh;
    EXPECT_EQ(0u, fresh.loadFromStore());
    EXPECT_FALSE(fresh.playerExists("user_rm"));
}

TEST_F(GAPlayerDatabaseTest, ChangePlayerIdMovesStoredRow)
{
    PlayerDatabase db;
    ASSERT_TRUE(db.addPlayer(makePlayer("old_id")));
    ASSERT_TRUE(db.changePlayerId("old_id", "new_id"));
    ASSERT_TRUE(drainGAThread());

    PlayerDatabase fresh;
    EXPECT_EQ(1u, fresh.loadFromStore());
    EXPECT_FALSE(fresh.playerExists("old_id"));
    ASSERT_TRUE(fresh.playerExists("new_id"));
    EXPECT_EQ("new_id", fresh.getPlayer("new_id").getUserId());
    EXPECT_EQ("ext_old_id", fresh.getPlayer("new_id").extUserId);
}

TEST_F(GAPlayerDatabaseTest, PersistPlayerWritesUpdatedState)
{
    PlayerDatabase db;
    ASSERT_TRUE(db.addPlayer(makePlayer("user_up")));

    Player& p = db.getPlayer("user_up");
    p.countryCode = "se";
    p.progressionTries.incrementTries("w1:l1");
    ASSERT_TRUE(db.persistPlayer("user_up"));
    EXPECT_FALSE(db.persistPlayer("missing"));
    ASSERT_TRUE(drainGAThread());

    PlayerDatabase fresh;
    ASSERT_EQ(1u, fresh.loadFromStore());
    EXPECT_EQ("se", fresh.getPlayer("user_up").countryCode);
    EXPECT_EQ(4, fresh.getPlayer("user_up").progressionTries.getTries("w1:l1"));
}

TEST_F(GAPlayerDatabaseTest, InMemoryPlayersWinOverStoreAndAreWrittenBack)
{
    {
        PlayerDatabase db;
        Player stored = makePlayer("shared");
        stored.countryCode = "stored";
        ASSERT_TRUE(db.addPlayer(std::move(stored)));
    }
    ASSERT_TRUE(drainGAThread());

    PlayerDatabase db;
    Player live = makePlayer("shared");
    live.countryCode = "live";
    ASSERT_TRUE(db.addPlayer(std::move(live)));
    ASSERT_TRUE(db.addPlayer(makePlayer("only_in_memory")));
    ASSERT_TRUE(drainGAThread());

    // "shared" is in both, "only_in_memory" only in memory: nothing new restored
    EXPECT_EQ(0u, db.loadFromStore());
    EXPECT_EQ(2u, db.countPlayers());
    EXPECT_EQ("live", db.getPlayer("shared").countryCode);

    ASSERT_TRUE(drainGAThread());
    json rows = storedRows();
    ASSERT_EQ(2u, rows.size());

    PlayerDatabase fresh;
    EXPECT_EQ(2u, fresh.loadFromStore());
    EXPECT_EQ("live", fresh.getPlayer("shared").countryCode);
    EXPECT_TRUE(fresh.playerExists("only_in_memory"));
}

TEST_F(GAPlayerDatabaseTest, CorruptRowsAreDroppedOnLoad)
{
    store::GAStore::setPlayer("broken", "not json");
    store::GAStore::setPlayer("mismatch", R"({"user_id":"someone_else","session_num":1,"current_session_length":0,"lifetime_session_length":0})");
    ASSERT_EQ(2u, storedRows().size());

    PlayerDatabase db;
    EXPECT_EQ(0u, db.loadFromStore());
    EXPECT_EQ(0u, db.countPlayers());

    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(0u, storedRows().size());
}
