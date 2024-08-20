//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#include "GAStore.h"
#include "GADevice.h"
#include "GAThreading.h"
#include "GALogger.h"
#include "GAUtilities.h"
#include <fstream>
#include <string.h>

namespace gameanalytics
{
    namespace store
    {
        constexpr int MaxDbSizeBytes = 6291456;
        constexpr int MaxDbSizeBytesBeforeTrim = 5242880;

        GAStore::GAStore()
        {
        }

        GAStore::~GAStore()
        {
            threading::GAThreading::endThread();
        }

        GAStore& GAStore::getInstance()
        {
            static GAStore instance;
            return instance;
        }

        bool GAStore::executeQuerySync(std::string const& sql)
        {
            json d;
            executeQuerySync(sql, d);
            return !d.is_null();
        }

        void GAStore::executeQuerySync(std::string const& sql, json& out)
        {
            executeQuerySync(sql, {}, 0, out);
        }


        void GAStore::executeQuerySync(std::string const& sql, StringVector const& parameters)
        {
            json d;
            executeQuerySync(sql, parameters, false, d);
        }

        void GAStore::executeQuerySync(std::string const& sql, StringVector const& parameters, json& out)
        {
            executeQuerySync(sql, parameters, false, out);
        }

        void GAStore::executeQuerySync(std::string const& sql, StringVector const& parameters, bool useTransaction)
        {
            json d;
            executeQuerySync(sql, parameters, useTransaction, d);
        }

        void GAStore::executeQuerySync(std::string const& sql, StringVector const& parameters, bool useTransaction, json& out)
        {
            try
            {
                // Force transaction if it is an update, insert or delete.
                std::string sqlUpper = utilities::toUpperCase(sql);
                if (utilities::GAUtilities::stringMatch(sqlUpper, "^(UPDATE|INSERT|DELETE)"))
                {
                    useTransaction = true;
                }

                // Get database connection from singelton getInstance
                sqlite3 *sqlDatabasePtr = getInstance().getDatabase();

                if (useTransaction)
                {
                    if (sqlite3_exec(sqlDatabasePtr, "BEGIN;", 0, 0, 0) != SQLITE_OK)
                    {
                        logging::GALogger::e("SQLITE3 BEGIN ERROR: %s", sqlite3_errmsg(sqlDatabasePtr));
                        return;
                    }
                }

                out = json::array();

                // Create statement
                sqlite3_stmt *statement = nullptr;

                // Prepare statement
                if (sqlite3_prepare_v2(sqlDatabasePtr, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK)
                {
                    // Bind parameters
                    if (!parameters.empty())
                    {
                        for (size_t index = 0; index < parameters.size(); index++)
                        {
                            sqlite3_bind_text(statement, static_cast<int>(index + 1), parameters[index].c_str(), -1, 0);
                        }
                    }

                    // get columns count
                    int columnCount = sqlite3_column_count(statement);

                    // Loop through results
                    while (sqlite3_step(statement) == SQLITE_ROW)
                    {
                        json row;
                        for (int i = 0; i < columnCount; i++)
                        {
                            const char *column = sqlite3_column_name(statement, i);
                            const char *value  = reinterpret_cast<const char*>(sqlite3_column_text(statement, i));

                            if (!column || !value)
                            {
                                continue;
                            }

                            switch (sqlite3_column_type(statement, i))
                            {
                                case SQLITE_INTEGER:
                                {
                                    try
                                    {
                                        int64_t valInt = std::stoll(value);
                                        row[column] = valInt;
                                    }
                                    catch(std::exception& e)
                                    {
                                        logging::GALogger::w("Failed to parse int: %s", e.what());
                                    }

                                    break;
                                }
                                case SQLITE_FLOAT:
                                {
                                    try
                                    {
                                        double valFloat = std::stod(value);
                                        row[column] = valFloat;
                                    }
                                    catch(std::exception& e)
                                    {
                                        logging::GALogger::w("Failed to parse float: %s", e.what());
                                    }
                                    
                                    break;
                                }
                                default:
                                {
                                    row[column] = value;
                                }
                            }
                        }
                        out.push_back(std::move(row));
                    }
                }
                else
                {
                    // TODO(nikolaj): Should we do a db validation to see if the db is corrupt here?
                    logging::GALogger::e("SQLITE3 PREPARE ERROR: %s", sqlite3_errmsg(sqlDatabasePtr));
                    out = {};
                    return;
                }

                // Destroy statement
                if (sqlite3_finalize(statement) == SQLITE_OK)
                {
                    if (useTransaction)
                    {
                        if (sqlite3_exec(sqlDatabasePtr, "COMMIT", 0, 0, 0) != SQLITE_OK)
                        {
                            logging::GALogger::e("SQLITE3 COMMIT ERROR: %s", sqlite3_errmsg(sqlDatabasePtr));
                            out = {};
                            return;
                        }
                    }
                }
                else
                {
                    logging::GALogger::d("SQLITE3 FINALIZE ERROR: %s", sqlite3_errmsg(sqlDatabasePtr));

                    if (useTransaction)
                    {
                        if (sqlite3_exec(sqlDatabasePtr, "ROLLBACK", 0, 0, 0) != SQLITE_OK)
                        {
                            logging::GALogger::e("SQLITE3 ROLLBACK ERROR: %s", sqlite3_errmsg(sqlDatabasePtr));
                        }

                        out = {};
                    }

                    return;
                }
            }
            catch(std::exception& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
                out = {};
            }
        }

        sqlite3* GAStore::getDatabase()
        {
            return sqlDatabase;
        }

        bool GAStore::ensureDatabase(bool dropDatabase, std::string const& key)
        {
            // lazy creation of db path
            if(getInstance().dbPath.empty())
            {
                // device::GADevice::getWritablePath()
                getInstance().dbPath = device::GADevice::getWritablePath() + "/ga.sqlite3";
            }

            // Open database
            if (sqlite3_open(getInstance().dbPath.c_str(), &getInstance().sqlDatabase) != SQLITE_OK)
            {
                getInstance().dbReady = false;
                logging::GALogger::w("Could not open database: %s", getInstance().dbPath.c_str());
                return false;
            }
            else
            {
                getInstance().dbReady = true;
                logging::GALogger::i("Database opened: %s", getInstance().dbPath.c_str());
            }

            if (dropDatabase)
            {
                logging::GALogger::d("Drop tables");
                GAStore::executeQuerySync("DROP TABLE ga_events");
                GAStore::executeQuerySync("DROP TABLE ga_state");
                GAStore::executeQuerySync("DROP TABLE ga_session");
                GAStore::executeQuerySync("DROP TABLE ga_progression");
                GAStore::executeQuerySync("VACUUM");
            }

            // Create statements
            constexpr const char* sql_ga_events = "CREATE TABLE IF NOT EXISTS ga_events(status CHAR(50) NOT NULL, category CHAR(50) NOT NULL, session_id CHAR(50) NOT NULL, client_ts CHAR(50) NOT NULL, event TEXT NOT NULL);";
            constexpr const char* sql_ga_session = "CREATE TABLE IF NOT EXISTS ga_session(session_id CHAR(50) PRIMARY KEY NOT NULL, timestamp CHAR(50) NOT NULL, event TEXT NOT NULL);";
            constexpr const char* sql_ga_state = "CREATE TABLE IF NOT EXISTS ga_state(key CHAR(255) PRIMARY KEY NOT NULL, value TEXT);";
            constexpr const char* sql_ga_progression = "CREATE TABLE IF NOT EXISTS ga_progression(progression CHAR(255) PRIMARY KEY NOT NULL, tries CHAR(255));";

            if (!GAStore::executeQuerySync(sql_ga_events))
            {
                logging::GALogger::d("ensureDatabase failed: %s", sql_ga_events);
                return false;
            }

            if (!GAStore::executeQuerySync("SELECT status FROM ga_events LIMIT 0,1"))
            {
                logging::GALogger::d("ga_events corrupt, recreating.");
                GAStore::executeQuerySync("DROP TABLE ga_events");
                if (!GAStore::executeQuerySync(sql_ga_events))
                {
                    logging::GALogger::w("ga_events corrupt, could not recreate it.");
                    return false;
                }
            }

            if (!GAStore::executeQuerySync(sql_ga_session))
            {
                return false;
            }

            if (!GAStore::executeQuerySync("SELECT session_id FROM ga_session LIMIT 0,1"))
            {
                logging::GALogger::d("ga_session corrupt, recreating.");
                GAStore::executeQuerySync("DROP TABLE ga_session");
                if (!GAStore::executeQuerySync(sql_ga_session))
                {
                    logging::GALogger::w("ga_session corrupt, could not recreate it.");
                    return false;
                }
            }

            if (!GAStore::executeQuerySync(sql_ga_state))
            {
                return false;
            }

            if (!GAStore::executeQuerySync("SELECT key FROM ga_state LIMIT 0,1"))
            {
                logging::GALogger::d("ga_state corrupt, recreating.");
                GAStore::executeQuerySync("DROP TABLE ga_state");
                if (!GAStore::executeQuerySync(sql_ga_state))
                {
                    logging::GALogger::w("ga_state corrupt, could not recreate it.");
                    return false;
                }
            }

            if (!GAStore::executeQuerySync(sql_ga_progression))
            {
                return false;
            }

            if (!GAStore::executeQuerySync("SELECT progression FROM ga_progression LIMIT 0,1"))
            {
                logging::GALogger::d("ga_progression corrupt, recreating.");
                GAStore::executeQuerySync("DROP TABLE ga_progression");
                if (!GAStore::executeQuerySync(sql_ga_progression))
                {
                    logging::GALogger::w("ga_progression corrupt, could not recreate it.");
                    return false;
                }
            }

            getInstance().trimEventTable();
            getInstance().tableReady = true;

            logging::GALogger::d("Database tables ensured present");

            return true;
        }

        void GAStore::setState(std::string const& key, std::string const& value)
        {
            if (value.empty())
            {
                StringVector parameterArray = {key};
                executeQuerySync("DELETE FROM ga_state WHERE key = ?;", parameterArray);
            }
            else
            {
                StringVector parameterArray = {key, value};
                executeQuerySync("INSERT OR REPLACE INTO ga_state (key, value) VALUES(?, ?);", parameterArray, true);
            }
        }

        int64_t GAStore::getDbSizeBytes()
        {
            std::ifstream in(getInstance().dbPath, std::ifstream::ate | std::ifstream::binary);
            return in.tellg();
        }

        bool GAStore::getTableReady()
        {
            return getInstance().tableReady;
        }

        bool GAStore::isDbTooLargeForEvents()
        {
            return getDbSizeBytes() > MaxDbSizeBytes;
        }

        bool GAStore::trimEventTable()
        {
            if(getDbSizeBytes() > MaxDbSizeBytesBeforeTrim)
            {
                try
                {
                    json resultSessionArray;
                    executeQuerySync("SELECT session_id, Max(client_ts) FROM ga_events GROUP BY session_id ORDER BY client_ts LIMIT 3", resultSessionArray);

                    if(!resultSessionArray.is_null() && resultSessionArray.size() > 0)
                    {
                        std::string sessionDeleteString;

                        unsigned int i = 0;
                        for (auto itr = resultSessionArray.begin(); itr != resultSessionArray.end(); ++itr)
                        {
                            std::string const session_id = itr->get<std::string>();

                            if(i < resultSessionArray.size() - 1)
                            {
                                sessionDeleteString += session_id + ",";
                            }
                            else
                            {
                                sessionDeleteString += session_id;
                            }
                            ++i;
                        }

                        const std::string deleteOldSessionsSql = utilities::printString("DELETE FROM ga_events WHERE session_id IN (\"%s\");", sessionDeleteString.c_str());
                        logging::GALogger::w("Database too large when initializing. Deleting the oldest 3 sessions.");
                        executeQuerySync(deleteOldSessionsSql);
                        executeQuerySync("VACUUM");

                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                catch(std::exception& e)
                {
                    logging::GALogger::e("Exception thrown: %s", e.what());
                    return false;
                }
            }

            return true;
        }

    }
}
