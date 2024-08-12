//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include <sqlite3.h>
#include <vector>
#include "GameAnalytics.h"
#include <mutex>
#include <cstdlib>
#include "GACommon.h"

namespace gameanalytics
{
    namespace store
    {
        class GAStore
        {
         public:

            sqlite3* getDatabase();
            static bool isDestroyed();

            static bool ensureDatabase(bool dropDatabase, std::string const& key = "");

            static void setState(std::string const& key, std::string const& value);

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

            static bool _destroyed;
            static GAStore* _instance;
            static std::once_flag _initInstanceFlag;
            static void cleanUp();
            static GAStore* getInstance();

            static void initInstance()
            {
                if(!_destroyed && !_instance)
                {
                    _instance = new GAStore();
                    std::atexit(&cleanUp);
                }
            }

            bool trimEventTable();

            // set when calling "ensureDatabase"
            // using a "writablePath" that needs to be set into the C++ component before
            std::string dbPath;

            // local pointer to database
            sqlite3* sqlDatabase = nullptr;

            // 10 MB limit for database. Will initiate trim logic when exceeded.
            // long maxDbSizeBytes = 10485760;

            // ??
            bool dbReady = false;
            // bool to determine if tables are ensured ready
            bool tableReady = false;
        };
    }
}
