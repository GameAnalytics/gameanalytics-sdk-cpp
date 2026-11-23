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
#include <GAUtilities.h>
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

        // writes a player row whose last activity was ageSeconds ago
        static void storeAged(std::string const& uid, int64_t ageSeconds)
        {
            const int64_t ts = utilities::GAUtilities::timeIntervalSince1970() - ageSeconds;
            json j = json::parse(makePlayer(uid).serialize());
            j["last_seen_ts"] = ts;
            store::GAStore::setPlayer(uid, j.dump(), ts);
        }

        static json initResponseWithConfigs()
        {
            const int64_t now = utilities::GAUtilities::timeIntervalSince1970();
            return json{
                {"configs_hash", "hash2"},
                {"ab_id", "ab2"},
                {"ab_variant_id", "var2"},
                {"configs", json::array({
                    {{"key", "speed"},   {"value", "1.5"},  {"id", "c1"}, {"vsn", 2}},
                    {{"key", "enabled"}, {"value", "true"}, {"id", "c2"}, {"vsn", 1}},
                    {{"key", "lives"},   {"value", 3},      {"id", "c3"}, {"vsn", 1}},
                    {{"key", "expired"}, {"value", "x"},    {"id", "c4"}, {"vsn", 1}, {"end_ts", now - 10}},
                    {{"key", "future"},  {"value", "y"},    {"id", "c5"}, {"vsn", 1}, {"start_ts", now + 1000}},
                    {{"key", ""},        {"value", "z"}},
                    {{"key", "novalue"}}
                })}
            };
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

    PlayerPtr a = fresh.getPlayer("user_a");
    EXPECT_EQ("ext_user_a", a->extUserId);
    EXPECT_EQ("dk", a->countryCode);
    EXPECT_EQ(3, a->progressionTries.getTries("w1:l1"));
    EXPECT_EQ(7, a->customFields.getValue<int64_t>("level", 0));
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
    EXPECT_EQ("new_id", fresh.getPlayer("new_id")->getUserId());
    EXPECT_EQ("ext_old_id", fresh.getPlayer("new_id")->extUserId);
}

TEST_F(GAPlayerDatabaseTest, PersistPlayerWritesUpdatedState)
{
    PlayerDatabase db;
    ASSERT_TRUE(db.addPlayer(makePlayer("user_up")));

    PlayerPtr p = db.getPlayer("user_up");
    p->countryCode = "se";
    p->progressionTries.incrementTries("w1:l1");
    ASSERT_TRUE(db.persistPlayer("user_up"));
    EXPECT_FALSE(db.persistPlayer("missing"));
    ASSERT_TRUE(drainGAThread());

    PlayerDatabase fresh;
    ASSERT_EQ(1u, fresh.loadFromStore());
    EXPECT_EQ("se", fresh.getPlayer("user_up")->countryCode);
    EXPECT_EQ(4, fresh.getPlayer("user_up")->progressionTries.getTries("w1:l1"));
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
    EXPECT_EQ("live", db.getPlayer("shared")->countryCode);

    ASSERT_TRUE(drainGAThread());
    json rows = storedRows();
    ASSERT_EQ(2u, rows.size());

    PlayerDatabase fresh;
    EXPECT_EQ(2u, fresh.loadFromStore());
    EXPECT_EQ("live", fresh.getPlayer("shared")->countryCode);
    EXPECT_TRUE(fresh.playerExists("only_in_memory"));
}

TEST_F(GAPlayerDatabaseTest, CorruptRowsAreDroppedOnLoad)
{
    store::GAStore::setPlayer("broken", "not json", 0);
    store::GAStore::setPlayer("mismatch", R"({"user_id":"someone_else","session_num":1,"current_session_length":0,"lifetime_session_length":0})", 0);
    ASSERT_EQ(2u, storedRows().size());

    PlayerDatabase db;
    EXPECT_EQ(0u, db.loadFromStore());
    EXPECT_EQ(0u, db.countPlayers());

    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(0u, storedRows().size());
}

// ---- retention ----

TEST_F(GAPlayerDatabaseTest, EvictPlayersInactiveSinceRemovesIdlePlayers)
{
    constexpr int64_t DAY = 24 * 60 * 60;
    storeAged("old", 10 * DAY);
    storeAged("recent", 1 * DAY);

    PlayerDatabase db;
    ASSERT_EQ(2u, db.loadFromStore());

    EXPECT_EQ(1u, db.evictPlayersInactiveSince(utilities::GAUtilities::timeIntervalSince1970() - 7 * DAY));
    EXPECT_FALSE(db.playerExists("old"));
    EXPECT_TRUE(db.playerExists("recent"));

    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(1, store::GAStore::countPlayers());
    EXPECT_EQ(1u, storedRows().size());
    EXPECT_EQ("recent", storedRows()[0].value("user_id", ""));
}

TEST_F(GAPlayerDatabaseTest, TrimToMaxPlayersKeepsActiveAndMostRecent)
{
    constexpr int64_t DAY = 24 * 60 * 60;
    storeAged("oldest", 30 * DAY);
    storeAged("older", 20 * DAY);
    storeAged("active_but_old", 40 * DAY);
    storeAged("newest", 1 * DAY);

    PlayerDatabase db;
    ASSERT_EQ(4u, db.loadFromStore());
    db.getPlayer("active_but_old")->onJoin();

    EXPECT_EQ(2u, db.trimToMaxPlayers(2));
    EXPECT_EQ(2u, db.countPlayers());
    EXPECT_TRUE(db.playerExists("newest"));
    EXPECT_TRUE(db.playerExists("active_but_old"));

    // active players are never evicted, even when that leaves the registry over the cap
    EXPECT_EQ(1u, db.trimToMaxPlayers(1));
    EXPECT_TRUE(db.playerExists("active_but_old"));
    EXPECT_EQ(0u, db.trimToMaxPlayers(0));  // 0 = unlimited

    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(1, store::GAStore::countPlayers());
}

TEST_F(GAPlayerDatabaseTest, LoadFromStoreAppliesRetentionBeforeLoading)
{
    constexpr int64_t DAY = 24 * 60 * 60;
    storeAged("p10d", 10 * DAY);
    storeAged("p5d", 5 * DAY);
    storeAged("p1d", 1 * DAY);
    ASSERT_EQ(3, store::GAStore::countPlayers());

    {
        PlayerDatabase db;
        db.setRetention(7 * DAY, 0);
        EXPECT_EQ(2u, db.loadFromStore());
        EXPECT_FALSE(db.playerExists("p10d"));
        EXPECT_EQ(2, store::GAStore::countPlayers());
    }

    {
        PlayerDatabase db;
        db.setRetention(0, 1);
        EXPECT_EQ(1u, db.loadFromStore());
        EXPECT_TRUE(db.playerExists("p1d"));
        EXPECT_EQ(1, store::GAStore::countPlayers());
    }
}

TEST_F(GAPlayerDatabaseTest, AddPlayerBeyondCapEvictsLeastRecentlySeen)
{
    constexpr int64_t DAY = 24 * 60 * 60;
    storeAged("older", 20 * DAY);
    storeAged("old", 10 * DAY);

    PlayerDatabase db;
    db.setRetention(0, 2);
    ASSERT_EQ(2u, db.loadFromStore());

    ASSERT_TRUE(db.addPlayer(makePlayer("fresh")));
    EXPECT_EQ(2u, db.countPlayers());
    EXPECT_FALSE(db.playerExists("older"));
    EXPECT_TRUE(db.playerExists("old"));
    EXPECT_TRUE(db.playerExists("fresh"));

    ASSERT_TRUE(drainGAThread());
    EXPECT_EQ(2, store::GAStore::countPlayers());
}

TEST_F(GAPlayerDatabaseTest, LastSeenIsUpdatedBySessionsAndPersisted)
{
    constexpr int64_t DAY = 24 * 60 * 60;
    storeAged("p", 10 * DAY);

    PlayerDatabase db;
    ASSERT_EQ(1u, db.loadFromStore());
    const int64_t before = db.getPlayer("p")->getLastSeen();
    EXPECT_LT(before, utilities::GAUtilities::timeIntervalSince1970() - 9 * DAY);

    db.getPlayer("p")->onJoin();
    db.getPlayer("p")->onExit();
    EXPECT_GE(db.getPlayer("p")->getLastSeen(), utilities::GAUtilities::timeIntervalSince1970() - 5);
    db.persistPlayer("p");
    ASSERT_TRUE(drainGAThread());

    PlayerDatabase fresh;
    fresh.setRetention(7 * DAY, 0);
    EXPECT_EQ(1u, fresh.loadFromStore());
    EXPECT_EQ(1, fresh.getPlayer("p")->getSessionCount());
}

// ---- remote configs ----

TEST_F(GAPlayerDatabaseTest, ApplyInitResponseSetsRemoteConfigsAndAbIds)
{
    Player p = makePlayer("rc");
    EXPECT_FALSE(p.isRemoteConfigsReady());
    EXPECT_EQ("[]", p.getRemoteConfigs());
    EXPECT_EQ(9, p.getRemoteConfigsValueAsInt("lives", 9));

    json response = initResponseWithConfigs();
    PlayerDatabase::applyInitResponse(p, response);

    EXPECT_TRUE(p.isRemoteConfigsReady());
    EXPECT_EQ("hash2", p.getConfigsHash());
    EXPECT_EQ("ab2", p.getABId());
    EXPECT_EQ("var2", p.getABVariantId());

    EXPECT_DOUBLE_EQ(1.5, p.getRemoteConfigsValueAsFloat("speed"));
    EXPECT_EQ("1.5", p.getRemoteConfigsValueAsString("speed"));
    EXPECT_TRUE(p.getRemoteConfigsValueAsBool("enabled"));
    EXPECT_EQ(3, p.getRemoteConfigsValueAsInt("lives"));
    EXPECT_EQ(3u, p.getRemoteConfigsValueAsUInt("lives"));

    // inactive, malformed and unknown keys fall back to the default
    EXPECT_EQ("def", p.getRemoteConfigsValueAsString("expired", "def"));
    EXPECT_EQ("def", p.getRemoteConfigsValueAsString("future", "def"));
    EXPECT_EQ("def", p.getRemoteConfigsValueAsString("novalue", "def"));
    EXPECT_EQ(42, p.getRemoteConfigsValueAsInt("speed_missing", 42));
    EXPECT_EQ(7, p.getRemoteConfigsValueAsInt("enabled", 7));  // "true" is not an int
    EXPECT_TRUE(p.getRemoteConfigsValueAsBool("lives", true)); // "3" is not a bool

    json content = json::parse(p.getRemoteConfigs());
    ASSERT_EQ(3u, content.size());
    EXPECT_EQ("speed", content[0].value("key", ""));

    // tracking info goes out with every player event
    json annotations = PlayerDatabase::getPlayerAnnotations(p);
    ASSERT_TRUE(annotations.contains("configurations_v3"));
    ASSERT_EQ(3u, annotations["configurations_v3"].size());
    EXPECT_EQ("c1", annotations["configurations_v3"][0].value("id", ""));
    EXPECT_EQ(2, annotations["configurations_v3"][0].value("vsn", 0));
}

TEST_F(GAPlayerDatabaseTest, ApplyInitResponseWithoutConfigsKeepsCachedOnes)
{
    Player p = makePlayer("rc");
    json first = initResponseWithConfigs();
    PlayerDatabase::applyInitResponse(p, first);

    json unchanged = json{{"server_ts", utilities::GAUtilities::timeIntervalSince1970()}};
    PlayerDatabase::applyInitResponse(p, unchanged);

    EXPECT_TRUE(p.isRemoteConfigsReady());
    EXPECT_EQ("hash2", p.getConfigsHash());
    EXPECT_EQ("ab2", p.getABId());
    EXPECT_DOUBLE_EQ(1.5, p.getRemoteConfigsValueAsFloat("speed"));

    json replaced = json{{"configs_hash", "hash3"}, {"configs", json::array({{{"key", "speed"}, {"value", "9"}}})}};
    PlayerDatabase::applyInitResponse(p, replaced);
    EXPECT_EQ("hash3", p.getConfigsHash());
    EXPECT_EQ(9, p.getRemoteConfigsValueAsInt("speed"));
    EXPECT_EQ("gone", p.getRemoteConfigsValueAsString("enabled", "gone"));
}

TEST_F(GAPlayerDatabaseTest, RemoteConfigsSurviveARestart)
{
    {
        PlayerDatabase db;
        Player p = makePlayer("rc");
        json response = initResponseWithConfigs();
        PlayerDatabase::applyInitResponse(p, response);
        ASSERT_TRUE(db.addPlayer(std::move(p)));
    }
    ASSERT_TRUE(drainGAThread());

    PlayerDatabase fresh;
    ASSERT_EQ(1u, fresh.loadFromStore());
    PlayerPtr p = fresh.getPlayer("rc");

    EXPECT_TRUE(p->isRemoteConfigsReady());
    EXPECT_EQ("hash2", p->getConfigsHash());
    EXPECT_EQ("ab2", p->getABId());
    EXPECT_EQ("var2", p->getABVariantId());
    EXPECT_DOUBLE_EQ(1.5, p->getRemoteConfigsValueAsFloat("speed"));
    EXPECT_TRUE(p->getRemoteConfigsValueAsBool("enabled"));
    EXPECT_EQ(3u, PlayerDatabase::getPlayerAnnotations(*p)["configurations_v3"].size());
}

// ---- shared ownership ----

TEST_F(GAPlayerDatabaseTest, RemovedAndEvictedPlayersStayValidForHolders)
{
    PlayerDatabase db;
    ASSERT_TRUE(db.addPlayer(makePlayer("held")));
    ASSERT_TRUE(db.addPlayer(makePlayer("evicted")));

    PlayerPtr held    = db.getPlayer("held");
    PlayerPtr evicted = db.getPlayer("evicted");
    EXPECT_FALSE(held->isRemoved());
    EXPECT_EQ(nullptr, db.findPlayer("nobody"));
    EXPECT_THROW(db.getPlayer("nobody"), PlayerNotFound);

    ASSERT_TRUE(db.removePlayer("held"));
    EXPECT_FALSE(db.playerExists("held"));
    EXPECT_TRUE(held->isRemoved());
    EXPECT_EQ("held", held->getUserId());
    EXPECT_EQ("dk", held->countryCode);

    // eviction marks the player the same way
    db.getPlayer("evicted")->onJoin();
    db.getPlayer("evicted")->onExit();
    EXPECT_EQ(1u, db.evictPlayersInactiveSince(utilities::GAUtilities::timeIntervalSince1970() + 10));
    EXPECT_TRUE(evicted->isRemoved());
    EXPECT_EQ(1, evicted->getSessionCount());

    // a renamed player keeps its identity for holders
    ASSERT_TRUE(db.addPlayer(makePlayer("before")));
    PlayerPtr renamed = db.getPlayer("before");
    ASSERT_TRUE(db.changePlayerId("before", "after"));
    EXPECT_FALSE(renamed->isRemoved());
    EXPECT_EQ("after", renamed->getUserId());
    EXPECT_EQ(renamed, db.getPlayer("after"));
}

TEST_F(GAPlayerDatabaseTest, LoadFromStoreWritesBackPlayersRegisteredBeforeLoad)
{
    PlayerDatabase db;
    Player live = makePlayer("shared");
    live.countryCode = "live";
    ASSERT_TRUE(db.addPlayer(std::move(live)));
    ASSERT_TRUE(drainGAThread());

    // simulate the write having been dropped: the store holds an older copy
    Player stale = makePlayer("shared");
    stale.countryCode = "stale";
    store::GAStore::setPlayer("shared", stale.serialize(), stale.getLastSeen());

    EXPECT_EQ(0u, db.loadFromStore());
    EXPECT_EQ("live", db.getPlayer("shared")->countryCode);
    ASSERT_TRUE(drainGAThread());

    PlayerDatabase fresh;
    ASSERT_EQ(1u, fresh.loadFromStore());
    EXPECT_EQ("live", fresh.getPlayer("shared")->countryCode);
}
