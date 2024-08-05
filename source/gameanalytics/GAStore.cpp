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
#if USE_UWP
#elif USE_TIZEN
#elif _WIN32
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace gameanalytics
{
    namespace store
    {
        constexpr int MaxDbSizeBytes = 6291456;
        constexpr int MaxDbSizeBytesBeforeTrim = 5242880;

        bool GAStore::_destroyed = false;
        GAStore* GAStore::_instance = 0;
        std::once_flag GAStore::_initInstanceFlag;

        GAStore::GAStore()
        {
        }

        void GAStore::cleanUp()
        {
            delete _instance;
            _instance = 0;
            _destroyed = true;
            threading::GAThreading::endThread();
        }

        GAStore* GAStore::getInstance()
        {
            std::call_once(_initInstanceFlag, &GAStore::initInstance);
            return _instance;
        }

        bool GAStore::isDestroyed()
        {
            return _destroyed;
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


        void GAStore::executeQuerySync(std::string const& sql, const char* parameters[], size_t size)
        {
            json d;
            executeQuerySync(sql, parameters, size, false, d);
        }

        void GAStore::executeQuerySync(std::string const& sql, const char* parameters[], size_t size, json& out)
        {
            executeQuerySync(sql, parameters, size, false, out);
        }

        void GAStore::executeQuerySync(std::string const& sql, const char* parameters[], size_t size, bool useTransaction)
        {
            json d;
            executeQuerySync(sql, parameters, size, useTransaction, d);
        }

        void GAStore::executeQuerySync(std::string const& sql, const char* parameters[], size_t size, bool useTransaction, json& out)
        {
            GAStore* i = getInstance();
            if(!i)
            {
                return;
            }
            // Force transaction if it is an update, insert or delete.
            std::string sqlUpper = utilities::toUpperCase(sql);
            if (utilities::GAUtilities::stringMatch(sqlUpper, "^(UPDATE|INSERT|DELETE)"))
            {
                useTransaction = true;
            }

            // Get database connection from singelton getInstance
            sqlite3 *sqlDatabasePtr = i->getDatabase();

            if (useTransaction)
            {
                if (sqlite3_exec(sqlDatabasePtr, "BEGIN;", 0, 0, 0) != SQLITE_OK)
                {
                    logging::GALogger::e("SQLITE3 BEGIN ERROR: %s", sqlite3_errmsg(sqlDatabasePtr));
                    return;
                }
            }

            // Create statement
            sqlite3_stmt *statement;

            // Prepare statement
            if (sqlite3_prepare_v2(sqlDatabasePtr, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK)
            {
                // Bind parameters
                if (size > 0)
                {
                    for (size_t index = 0; index < size; index++)
                    {
                        sqlite3_bind_text(statement, static_cast<int>(index + 1), parameters[index], -1, 0);
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
                        const char *column = reinterpret_cast<const char *>(sqlite3_column_name(statement, i));
                        const char *value = reinterpret_cast<const char *>(sqlite3_column_text(statement, i));

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
                                    logging::GALogger::e(e.what());
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
                                    logging::GALogger::e(e.what());
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
                }

                return;
            }
        }

        sqlite3* GAStore::getDatabase()
        {
            return sqlDatabase;
        }

        bool GAStore::ensureDatabase(bool dropDatabase, const char* key)
        {
            GAStore* i = getInstance();
            if(!i)
            {
                return false;
            }
            // lazy creation of db path
            if(i->dbPath.empty())
            {
                // device::GADevice::getWritablePath()
                i->dbPath = device::GADevice::getWritablePath() + "/ga.sqlite3";
            }

            // Open database
            if (sqlite3_open(i->dbPath.c_str(), &i->sqlDatabase) != SQLITE_OK)
            {
                i->dbReady = false;
                logging::GALogger::w("Could not open database: %s", i->dbPath.c_str());
                return false;
            }
            else
            {
                i->dbReady = true;
                logging::GALogger::i("Database opened: %s", i->dbPath.c_str());
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

            i->trimEventTable();
            i->tableReady = true;

            logging::GALogger::d("Database tables ensured present");

            return true;
        }

        void GAStore::setState(const char* key, const char* value)
        {
            if (strlen(value) == 0)
            {
                const char* parameterArray[1] = {key};
                executeQuerySync("DELETE FROM ga_state WHERE key = ?;", parameterArray, 1);
            }
            else
            {
                const char* parameterArray[2] = {key, value};
                executeQuerySync("INSERT OR REPLACE INTO ga_state (key, value) VALUES(?, ?);", parameterArray, 2, true);
            }
        }

        int64_t GAStore::getDbSizeBytes()
        {
            std::ifstream in(getInstance()->dbPath, std::ifstream::ate | std::ifstream::binary);
            return in.tellg();
        }

        bool GAStore::getTableReady()
        {
            GAStore* i = GAStore::getInstance();
            if(!i)
            {
                return false;
            }
            return i->tableReady;
        }

        bool GAStore::isDbTooLargeForEvents()
        {
            return getDbSizeBytes() > MaxDbSizeBytes;
        }


        bool GAStore::trimEventTable()
        {
            if(getDbSizeBytes() > MaxDbSizeBytesBeforeTrim)
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

                    const std::string deleteOldSessionsSql = "DELETE FROM ga_events WHERE session_id IN (\"" + sessionDeleteString + "\");";
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

            return true;
        }

    }
}
