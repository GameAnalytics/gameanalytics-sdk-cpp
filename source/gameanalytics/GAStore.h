//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include <sqlite3.h>
#include <vector>
#include <mutex>
#include <cstdlib>
#include "GACommon.h"

namespace gameanalytics
{
    namespace store
    {
        class GAStore
        {
            friend class state::GAState;

         public:

            sqlite3* getDatabase();

            static bool ensureDatabase(bool dropDatabase, std::string const& key = "");

            static void setState(std::string const& key, std::string const& value);

            // server-side player persistence (ga_players table)
            static void    setPlayer(std::string const& userId, std::string const& data, int64_t lastSeenTs);
            static void    removePlayer(std::string const& userId);
            static void    getPlayers(json& out);
            static int64_t countPlayers();

            // retention: drop players last seen before the given unix timestamp, and keep
            // at most maxPlayers rows (the most recently seen). Both return the rows removed.
            static int64_t evictPlayersInactiveSince(int64_t cutoffTs);
            static int64_t trimPlayers(int64_t maxPlayers);

            static bool executeQuerySync(std::string const& sql);
            static void executeQuerySync(std::string const& sql, json& out);

            static void executeQuerySync(std::string const& sql, StringVector const& parameters);
            static void executeQuerySync(std::string const& sql, StringVector const& parameters, json& out);

            static void executeQuerySync(std::string const& sql, StringVector const& parameters, bool useTransaction);
            static void executeQuerySync(std::string const& sql, StringVector const& parameters, bool useTransaction, json& out);

            static int64_t getDbSizeBytes();

            static bool getTableReady();
            static bool isDbTooLargeForEvents();

        private:

            GAStore();
            GAStore(const GAStore&) = delete;
            GAStore& operator=(const GAStore&) = delete;
            ~GAStore();

            static GAStore& getInstance();

            bool fixOldDatabase();
            bool trimEventTable();
            
            bool initDatabaseLocation();

            // set when calling "ensureDatabase"
            // using a "writablePath" that needs to be set into the C++ component before
            std::string dbPath;

            // local pointer to database
            sqlite3* sqlDatabase = nullptr;

            // ??
            bool dbReady = false;
            
            // bool to determine if tables are ensured ready
            bool tableReady = false;
        };
    }
}
