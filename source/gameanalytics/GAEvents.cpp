//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#include <vector>
#include "GAEvents.h"
#include "GAState.h"
#include "GAUtilities.h"
#include "GALogger.h"
#include "GAStore.h"
#include "GAThreading.h"
#include "GAValidator.h"
#include <string.h>
#include <stdio.h>
#include <cmath>
#include <inttypes.h>

namespace gameanalytics
{
    namespace events
    {
        GAEvents::GAEvents()
        {
            isRunning = false;
            keepRunning = false;
        }

        GAEvents::~GAEvents()
        {
            isRunning = false;
            keepRunning = false;
        }

        GAEvents& GAEvents::getInstance()
        {
            static GAEvents instance;
            return instance;
        }

        void GAEvents::stopEventQueue()
        {
            getInstance().keepRunning = false;
        }

        void GAEvents::ensureEventQueueIsRunning()
        {
            getInstance().keepRunning = true;
            if (!getInstance().isRunning)
            {
                getInstance().isRunning = true;
                threading::GAThreading::scheduleTimer(GAEvents::ProcessEventsIntervalInSeconds, 
                []() {return getInstance().processEventQueue();});
            }
        }

        // USER EVENTS
        void GAEvents::addSessionStartEvent()
        {
            try
            {
                if(!state::GAState::isEventSubmissionEnabled())
                {
                    return;
                }

                // Increment session number  and persist
                state::GAState::incrementSessionNum();
                std::string sessionNum = std::to_string(state::GAState::getSessionNum());

                // Event specific data
                json eventDict;

                eventDict["category"]       = CategorySessionStart;
                eventDict["session_num"]    = sessionNum;

                StringVector parameters = {"session_num", sessionNum};
                store::GAStore::executeQuerySync("INSERT OR REPLACE INTO ga_state (key, value) VALUES(?, ?);", parameters);

                // Add custom dimensions
                getInstance().addDimensionsToEvent(eventDict);

                json cleanedFields = state::GAState::getValidatedCustomFields();

                getInstance().addCustomFieldsToEvent(eventDict, cleanedFields);

                // Add to store
                getInstance().addEventToStore(eventDict);

                // Log
                logging::GALogger::i("Add SESSION START event");

                // Send event right away
                GAEvents::processEvents(CategorySessionStart, false);
            }
            catch(const std::exception& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
            
        }

        void GAEvents::addSessionEndEvent()
        {
            state::GAState& state = state::GAState::getInstance();

            if(!state::GAState::isEventSubmissionEnabled())
            {
                return;
            }
            
            try
            {
                int64_t session_start_ts    = state.getSessionStart();
                int64_t client_ts_adjusted  = state::GAState::getClientTsAdjusted();
                int64_t sessionLength       = client_ts_adjusted - session_start_ts;

                if(sessionLength < 0)
                {
                    // Should never happen.
                    // Could be because of edge cases regarding time altering on device.
                    logging::GALogger::w("Session length was calculated to be less then 0. Should not be possible. Resetting to 0.");
                    sessionLength = 0;
                }

                // Event specific data
                json eventDict;
                eventDict["category"]   = GAEvents::CategorySessionEnd;
                eventDict["length"]     = sessionLength;

                // Add custom dimensions
                getInstance().addDimensionsToEvent(eventDict);

                json cleanedFields = state::GAState::getValidatedCustomFields();

                getInstance().addCustomFieldsToEvent(eventDict, cleanedFields);

                // Add to store
                getInstance().addEventToStore(eventDict);

                // Log
                logging::GALogger::i("Add SESSION END event.");

                // Send all event right away
                GAEvents::processEvents("", false);
            }
            catch(const std::exception& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
        }

        // BUSINESS EVENT
        void GAEvents::addBusinessEvent(std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, const json& fields, bool mergeFields)
        {
            if(!state::GAState::isEventSubmissionEnabled())
            {
                return;
            }

            try 
            {
                // Validate event params
                validators::ValidationResult validationResult;
                validators::GAValidator::validateBusinessEvent(currency, amount, cartType, itemType, itemId, validationResult);
                if (!validationResult.result)
                {
                    http::GAHTTPApi* httpInstance = http::GAHTTPApi::getInstance();
                    if(!httpInstance)
                    {
                        return;
                    }
                    httpInstance->sendSdkErrorEvent(validationResult.category, validationResult.area, validationResult.action, validationResult.parameter, validationResult.reason, state::GAState::getGameKey(), state::GAState::getGameSecret());
                    return;
                }

                // Create empty eventData
                json eventDict;

                // Increment transaction number and persist
                state::GAState::incrementTransactionNum();

                const int transactionNum = state::GAState::getTransactionNum();

                StringVector params = {"transaction_num", std::to_string(transactionNum)};
                store::GAStore::executeQuerySync("INSERT OR REPLACE INTO ga_state (key, value) VALUES(?, ?);", params);

                eventDict["category"] = GAEvents::CategoryBusiness;
                eventDict["event_id"] = itemType + ':' + itemId;
                eventDict["currency"] = currency;
                eventDict["amount"]   = amount;
                eventDict["transaction_num"] = transactionNum;

                utilities::addIfNotEmpty(eventDict, "cart_type", cartType);

                // Add custom dimensions
                getInstance().addDimensionsToEvent(eventDict);

                json cleanedFields = state::GAState::getValidatedCustomFields(fields);
                getInstance().addCustomFieldsToEvent(eventDict, cleanedFields);

                // Log
                logging::GALogger::i("Add BUSINESS event: {currency:%s, amount:%d, itemType:%s, itemId:%s, cartType:%s, fields:%s}",
                    currency.c_str(), amount, itemType.c_str(), itemId.c_str(), cartType.c_str(), cleanedFields.dump(JSON_PRINT_INDENT).c_str());

                // Send to store
                getInstance().addEventToStore(eventDict);
            } 
            catch (std::exception const& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
            
        }

        void GAEvents::addResourceEvent(EGAResourceFlowType flowType, std::string const& currency, double amount, std::string const& itemType, std::string const& itemId, const json& fields, bool mergeFields)
        {
            try
            {
                if(!state::GAState::isEventSubmissionEnabled())
                {
                    return;
                }

                // Validate event params
                validators::ValidationResult validationResult;
                validators::GAValidator::validateResourceEvent(flowType, currency, amount, itemType, itemId, validationResult);
                if (!validationResult.result)
                {
                    http::GAHTTPApi* httpInstance = http::GAHTTPApi::getInstance();
                    if(!httpInstance)
                    {
                        return;
                    }
                    httpInstance->sendSdkErrorEvent(validationResult.category, validationResult.area, validationResult.action, validationResult.parameter, validationResult.reason, state::GAState::getGameKey(), state::GAState::getGameSecret());
                    return;
                }

                // If flow type is sink reverse amount
                if (flowType == Sink)
                {
                    amount *= -1;
                }

                // Create empty eventData
                json eventDict;
                
                const std::string flowStr = resourceFlowTypeString(flowType);

                // insert event specific values
                eventDict["event_id"] = flowStr + ':' + currency + ':' + itemType + ':' + itemId;
                eventDict["category"] = GAEvents::CategoryResource;
                eventDict["amount"]   = amount;

                // Add custom dimensions
                getInstance().addDimensionsToEvent(eventDict);

                json cleanedFields = state::GAState::getValidatedCustomFields(fields);
                getInstance().addCustomFieldsToEvent(eventDict, cleanedFields);

                // Log
                logging::GALogger::i("Add RESOURCE event: {currency:%s, amount: %f, itemType:%s, itemId:%s, fields:%s}", currency.c_str(), amount, itemType.c_str(), itemId.c_str(), cleanedFields.dump(4).c_str());

                // Send to store
                getInstance().addEventToStore(eventDict);
            }
            catch(const json::exception& e)
            {
                logging::GALogger::e("Failed to parse json: %s", e.what());
            }
            catch(const std::exception& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
            
        }

        void GAEvents::addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, bool sendScore, const json& fields, bool mergeFields)
        {
            try
            {
                if(!state::GAState::isEventSubmissionEnabled())
                {
                    return;
                }

                // Validate event params
                validators::ValidationResult validationResult;
                validators::GAValidator::validateProgressionEvent(progressionStatus, progression01, progression02, progression03, validationResult);
                if (!validationResult.result)
                {
                    http::GAHTTPApi* httpInstance = http::GAHTTPApi::getInstance();
                    if(!httpInstance)
                    {
                        return;
                    }
                    httpInstance->sendSdkErrorEvent(validationResult.category, validationResult.area, validationResult.action, validationResult.parameter, validationResult.reason, state::GAState::getGameKey(), state::GAState::getGameSecret());
                    return;
                }

                // Create empty eventData
                json eventDict;

                // Progression identifier
                std::string progressionIdentifier = progression01;

                if(!progression02.empty())
                {
                    progressionIdentifier += ':';
                    progressionIdentifier += progression02;

                    if(!progression03.empty())
                    {
                        progressionIdentifier += ':';
                        progressionIdentifier += progression03;
                    }
                }

                const std::string statusString = progressionStatusString(progressionStatus);

                eventDict["category"] = GAEvents::CategoryProgression;
                eventDict["event_id"] = statusString + ':' + progressionIdentifier;

                // Attempt
                int attempt_num = 0;

                // Add score if specified and status is not start
                if (sendScore && progressionStatus != EGAProgressionStatus::Start)
                {
                    eventDict["score"] = score;
                }

                // Count attempts on each progression fail and persist
                if (progressionStatus == EGAProgressionStatus::Fail)
                {
                    // Increment attempt number
                    state::GAState::incrementProgressionTries(progressionIdentifier);
                }

                // increment and add attempt_num on complete and delete persisted
                if (progressionStatus == EGAProgressionStatus::Complete)
                {
                    // Increment attempt number
                    state::GAState::incrementProgressionTries(progressionIdentifier);

                    // Add to event
                    attempt_num = state::GAState::getProgressionTries(progressionIdentifier);
                    eventDict["attempt_num"] = attempt_num;

                    // Clear
                    state::GAState::clearProgressionTries(progressionIdentifier);
                }

                // Add custom dimensions
                getInstance().addDimensionsToEvent(eventDict);

                json cleanedFields = state::GAState::getValidatedCustomFields(fields);

                getInstance().addCustomFieldsToEvent(eventDict, cleanedFields);

                // Log
                logging::GALogger::i("Add PROGRESSION event: {status:%s, progression01:%s, progression02:%s, progression03:%s, score:%d, attempt:%d, fields:%s}", statusString.c_str(), progression01.c_str(), progression02.c_str(), progression03.c_str(), score, attempt_num, cleanedFields.dump(4).c_str());

                // Send to store
                getInstance().addEventToStore(eventDict);
            }
            catch(std::exception& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
        }

        void GAEvents::addDesignEvent(std::string const& eventId, double value, bool sendValue, const json& fields, bool mergeFields)
        {
            try
            {
                if(!state::GAState::isEventSubmissionEnabled())
                {
                    return;
                }

                // Validate
                validators::ValidationResult validationResult;
                validators::GAValidator::validateDesignEvent(eventId, validationResult);
                if (!validationResult.result)
                {
                    http::GAHTTPApi* httpInstance = http::GAHTTPApi::getInstance();
                    if(!httpInstance)
                    {
                        return;
                    }
                    httpInstance->sendSdkErrorEvent(validationResult.category, validationResult.area, validationResult.action, validationResult.parameter, validationResult.reason, state::GAState::getGameKey(), state::GAState::getGameSecret());
                    return;
                }

                // Create empty eventData
                json eventData;
                eventData["category"] = GAEvents::CategoryDesign;
                eventData["event_id"] = eventId;

                if (sendValue)
                {
                    eventData["value"] = value;
                }

                json cleanedFields = state::GAState::getValidatedCustomFields(fields);

                GAEvents::getInstance().addCustomFieldsToEvent(eventData, cleanedFields);

                // Add custom dimensions
                getInstance().addDimensionsToEvent(eventData);

                // Log
                logging::GALogger::i("Add DESIGN event: {eventId:%s, value:%f, fields:%s}", eventId.c_str(), value, cleanedFields.dump(4).c_str());

                // Send to store
                getInstance().addEventToStore(eventData);
            }
            catch(json::exception const& e)
            {
                logging::GALogger::e("Failed to parse json: %s", e.what());
            }
            catch(std::exception const& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
        }

        void GAEvents::addErrorEvent(EGAErrorSeverity severity, std::string const& message, const json& fields, bool mergeFields)
        {
            addErrorEvent(severity, message, fields, mergeFields, false);
        }

        void GAEvents::addErrorEvent(EGAErrorSeverity severity, std::string const& message, const json& fields, bool mergeFields, bool skipAddingFields)
        {
            try
            {
                if(!state::GAState::isEventSubmissionEnabled())
                {
                    return;
                }

                // Validate
                validators::ValidationResult validationResult;
                validators::GAValidator::validateErrorEvent(severity, message, validationResult);
                if (!validationResult.result)
                {
                    http::GAHTTPApi* httpInstance = http::GAHTTPApi::getInstance();
                    if(!httpInstance)
                    {
                        return;
                    }
                    httpInstance->sendSdkErrorEvent(validationResult.category, validationResult.area, validationResult.action, validationResult.parameter, validationResult.reason, state::GAState::getGameKey(), state::GAState::getGameSecret());
                    return;
                }

                // Create empty eventData
                json eventData;
                eventData["category"] = GAEvents::CategoryError;
                eventData["severity"] = errorSeverityString(severity);
                eventData["message"]  = message;

                json cleanedFields;
                if(!skipAddingFields)
                {
                    cleanedFields = state::GAState::getValidatedCustomFields(fields);
                    getInstance().addCustomFieldsToEvent(eventData, cleanedFields);
                }

                // Add custom dimensions
                getInstance().addDimensionsToEvent(eventData);

                // Log
                logging::GALogger::i("Add ERROR event: {severity:%s, message:%s, fields:%s}", errorSeverityString(severity), message.c_str(), cleanedFields.dump(4).c_str());

                // Send to store
                getInstance().addEventToStore(eventData);
            }
            catch(std::exception& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
        }

        void GAEvents::processEventQueue()
        {
            processEvents("", true);

            if (getInstance().keepRunning)
            {
                threading::GAThreading::scheduleTimer(GAEvents::ProcessEventsIntervalInSeconds, 
                [this]()
                {
                    processEventQueue();
                });
            }
            else
            {
                getInstance().isRunning = false;
            }
        }

        void GAEvents::processEvents(std::string const& category, bool performCleanup)
        {
            if(!state::GAState::isEventSubmissionEnabled())
            {
                return;
            }

            // Request identifier
            std::string requestIdentifier = utilities::GAUtilities::generateUUID();

            std::string andCategory = utilities::printString(" AND category='%s' ", category.c_str());

            std::string selectSql  = utilities::printString("SELECT event FROM ga_events WHERE status = 'new' %s;", andCategory.c_str());
            std::string updateSql  = utilities::printString("UPDATE ga_events SET status = '%s' WHERE status = 'new' %s;", requestIdentifier.c_str(), andCategory.c_str());
            std::string deleteSql  = utilities::printString("DELETE FROM ga_events WHERE status = '%s'", requestIdentifier.c_str());
            std::string putbackSql = utilities::printString("UPDATE ga_events SET status = 'new' WHERE status = '%s';", requestIdentifier.c_str());

            // Cleanup
            if (performCleanup)
            {
                getInstance().cleanupEvents();
                getInstance().fixMissingSessionEndEvents();
            }

            // Get events to process
            json events;
            store::GAStore::executeQuerySync(selectSql, events);

            // Check for errors or empty
            if (events.is_null() || events.size() == 0)
            {
                logging::GALogger::i("Event queue: No events to send");
                getInstance().updateSessionTime();
                return;
            }

            // Check number of events and take some action if there are too many?
            if (events.size() > MaxEventCount)
            {
                // Make a limit request
                selectSql = utilities::printString("SELECT client_ts FROM ga_events WHERE status = 'new' %s ORDER BY client_ts ASC LIMIT 0,%d;", andCategory.c_str(), GAEvents::MaxEventCount);
                store::GAStore::executeQuerySync(selectSql, events);
                if (events.is_null())
                {
                    return;
                }

                // Get last timestamp
                const json& lastItem = events.back();
                const std::string lastTimestamp = lastItem["client_ts"].get<std::string>();

                // Select again
                selectSql = utilities::printString("SELECT event FROM ga_events WHERE status = 'new' %s AND client_ts<='%s';", andCategory.c_str(), lastTimestamp.c_str());
                store::GAStore::executeQuerySync(selectSql, events);
                if (events.is_null())
                {
                    return;
                }

                // Update sql
                updateSql = utilities::printString("UPDATE ga_events SET status='%s' WHERE status='new' %s AND client_ts<='%s';", requestIdentifier.c_str(), andCategory.c_str(), lastTimestamp.c_str());
            }

            // Log
            logging::GALogger::i("Event queue: Sending %d events.", events.size());

            // Set status of events to 'sending' (also check for error)
            json updateResult;
            store::GAStore::executeQuerySync(updateSql, updateResult);
            if (updateResult.is_null())
            {
                return;
            }

            // Create payload data from events
            json payloadArray;
            for (auto& node : events)
            {
                const std::string eventDict = utilities::getOptionalValue<std::string>(node, "event");
                if (!eventDict.empty())
                {
                    try
                    {
                        json d = json::parse(eventDict);
                        if(d.contains("client_ts") && d["client_ts"].is_number_integer())
                        {
                            if (!validators::GAValidator::validateClientTs(d["client_ts"].get<int64_t>()))
                            {
                                d.erase("client_ts");
                            }
                        }

                        payloadArray.push_back(std::move(d));
                    }
                    catch(const json::exception& e)
                    {
                        logging::GALogger::d("processEvents -- JSON error: %s", e.what());
                        logging::GALogger::d(eventDict.c_str());
                    }
                }
            }

            // send events
            json dataDict;
            http::EGAHTTPApiResponse responseEnum;
            http::GAHTTPApi* http = http::GAHTTPApi::getInstance();
            if(!http)
            {
                return;
            }
#if USE_UWP && defined(USE_UWP_HTTP)
            std::pair<http::EGAHTTPApiResponse, std::string> pair;

            try
            {
                pair = http->sendEventsInArray(payloadArray).get();
            }
            catch(Platform::COMException^ e)
            {
                pair = std::pair<http::EGAHTTPApiResponse, std::string>(http::NoResponse, "");
            }
            responseEnum = pair.first;

            if(pair.second.size() > 0)
            {
                try
                {
                    json d = json::parse(pair.second);
                    dataDict.merge_patch(d);
                }
                catch(const json::exception& e)
                {
                    logging::GALogger::d("processEvents -- JSON error: %s", e.what());
                    logging::GALogger::d("%s", pair.second.c_str());
                }
            }
#else
            http->sendEventsInArray(responseEnum, dataDict, payloadArray);
#endif

            if (responseEnum == http::Ok)
            {
                // Delete events
                store::GAStore::executeQuerySync(deleteSql);

                logging::GALogger::i("Event queue: %d events sent.", events.size());
            }
            else
            {
                // Put events back (Only in case of no response)
                if (responseEnum == http::NoResponse)
                {
                    logging::GALogger::w("Event queue: Failed to send events to collector - Retrying next time");
                    store::GAStore::executeQuerySync(putbackSql);
                    // Delete events (When getting some anwser back always assume events are processed)
                }
                else
                {
                    if (responseEnum == http::BadRequest && dataDict.is_array())
                    {
                        logging::GALogger::w("Event queue: %d events sent. %d events failed GA server validation.", events.size(), dataDict.size());
                    }
                    else
                    {
                        logging::GALogger::w("Event queue: Failed to send events.");
                    }

                    store::GAStore::executeQuerySync(deleteSql);
                }
            }
        }

        void GAEvents::updateSessionTime()
        {
            if(state::GAState::sessionIsStarted())
            {
                try
                {
                    json ev;
                    state::GAState::getEventAnnotations(ev);

                    // Add custom dimensions
                    GAEvents::addDimensionsToEvent(ev);

                    json cleanedFields = state::GAState::getValidatedCustomFields();

                    GAEvents::addCustomFieldsToEvent(ev, cleanedFields);

                    std::string jsonDefaults = ev.dump();
                    constexpr const char* sql = "INSERT OR REPLACE INTO ga_session(session_id, timestamp, event) VALUES(?, ?, ?);";
                    state::GAState& state = state::GAState::getInstance();

                    const std::string sessionStart = std::to_string(state.getSessionStart());
                    
                    const StringVector parameters = { ev["session_id"].get<std::string>(), sessionStart, jsonDefaults};
                    store::GAStore::executeQuerySync(sql, parameters);
                }
                catch(json::exception const& e)
                {
                    logging::GALogger::e("Failed to parse json: %s", e.what());
                }
                catch(std::exception const& e)
                {
                    logging::GALogger::e("Exception thrown: %s", e.what());
                }
            }
        }

        void GAEvents::cleanupEvents()
        {
            store::GAStore::executeQuerySync("UPDATE ga_events SET status = 'new';");
        }

        void GAEvents::fixMissingSessionEndEvents()
        {
            if(!state::GAState::isEventSubmissionEnabled())
            {
                return;
            }

            // Get all sessions that are not current
            StringVector const parameters = { state::GAState::getSessionId() };

            constexpr const char* sql = "SELECT timestamp, event FROM ga_session WHERE session_id != ?;";

            json sessions;
            store::GAStore::executeQuerySync(sql, parameters, sessions);

            if (sessions.is_null() || sessions.empty())
            {
                return;
            }

            logging::GALogger::i("%d session(s) located with missing session_end event.", sessions.size());

            // Add missing session_end events
            for (json& session : sessions)
            {
                if(session.contains("event"))
                {
                    try
                    {
                        json sessionEndEvent = json::parse(session["event"].get<std::string>());

                        int64_t event_ts = utilities::getOptionalValue<int64_t>(sessionEndEvent, "client_ts", 0);
                        int64_t start_ts = std::stoll(utilities::getOptionalValue<std::string>(sessionEndEvent, "timestamp", "0"));

                        int64_t length = event_ts - start_ts;
                        length = static_cast<int64_t>(fmax(length, 0));

                        logging::GALogger::d("fixMissingSessionEndEvents length calculated: %lld", length);

                        sessionEndEvent["category"] = GAEvents::CategorySessionEnd;
                        sessionEndEvent["length"]   = length;

                        // Add to store
                        addEventToStore(sessionEndEvent);
                    }
                    catch(json::exception const& e)
                    {
                        logging::GALogger::d("fixMissingSessionEndEvents -- JSON error: %s", e.what());
                        logging::GALogger::d("%s", session["event"].dump(JSON_PRINT_INDENT).c_str());
                    }
                    catch(std::exception const& e)
                    {
                        logging::GALogger::e("Exception thrown: %s", e.what());
                    }
                }
            }
        }

        // GENERAL
        void GAEvents::addEventToStore(json& eventData)
        {
            if(!state::GAState::isEventSubmissionEnabled())
            {
                return;
            }

            if(store::GAStore::isDestroyed())
            {
                return;
            }

            // Check if datastore is available
            if (!store::GAStore::getTableReady())
            {
                logging::GALogger::w("Could not add event: SDK datastore error");
                return;
            }

            // Check if we are initialized
            if (!state::GAState::isInitialized())
            {
                logging::GALogger::w("Could not add event: SDK is not initialized");
                return;
            }

            try
            {
                // Check db size limits (10mb)
                // If database is too large block all except user, session and business
                if (store::GAStore::isDbTooLargeForEvents() && !utilities::GAUtilities::stringMatch(eventData["category"].get<std::string>(), "^(user|session_end|business)$"))
                {
                    logging::GALogger::w("Database too large. Event has been blocked.");
                    http::GAHTTPApi* httpInstance = http::GAHTTPApi::getInstance();
                    if(!httpInstance)
                    {
                        return;
                    }
                    httpInstance->sendSdkErrorEvent(http::EGASdkErrorCategory::Database, http::EGASdkErrorArea::AddEventsToStore, http::EGASdkErrorAction::DatabaseTooLarge, (http::EGASdkErrorParameter)0, "", state::GAState::getGameKey(), state::GAState::getGameSecret());
                    return;
                }

                // Get default annotations
                json ev;
                state::GAState::getEventAnnotations(ev);
                ev.merge_patch(eventData);

                const std::string jsonString = ev.dump();

                // output if VERBOSE LOG enabled
                logging::GALogger::ii("Event added to queue: %s", jsonString.c_str());

                // Add to store
                std::string const clientTS = std::to_string(ev["client_ts"].get<int64_t>());
                StringVector parameters = { "new", ev["category"].get<std::string>(), ev["session_id"].get<std::string>(), clientTS, jsonString };
                constexpr const char* sql = "INSERT INTO ga_events (status, category, session_id, client_ts, event) VALUES(?, ?, ?, ?, ?);";

                store::GAStore::executeQuerySync(sql, parameters);

                // Add to session store if not last
                if (eventData["category"].get<std::string>() == GAEvents::CategorySessionEnd)
                {
                    StringVector params = { ev["session_id"].get<std::string>() };
                    store::GAStore::executeQuerySync("DELETE FROM ga_session WHERE session_id = ?;", params);
                }
                else
                {
                    GAEvents::updateSessionTime();
                }
            }
            catch(json::exception const& e)
            {
                logging::GALogger::e("Failed to parse json: %s", e.what());
            }
            catch(std::exception const& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
        }

        void GAEvents::addDimensionsToEvent(json& eventData)
        {
            if (eventData.is_null())
            {
                return;
            }

            utilities::addIfNotEmpty(eventData, "custom_01", state::GAState::getCurrentCustomDimension01());
            utilities::addIfNotEmpty(eventData, "custom_02", state::GAState::getCurrentCustomDimension02());
            utilities::addIfNotEmpty(eventData, "custom_03", state::GAState::getCurrentCustomDimension03());
        }

        void GAEvents::addCustomFieldsToEvent(json &eventData, json &fields)
        {
            if(eventData.is_null())
            {
                return;
            }

            if (fields.is_object())
            {
                eventData["custom_fields"] = fields;
            }
        }

        std::string GAEvents::progressionStatusString(EGAProgressionStatus progressionStatus)
        {
            switch (progressionStatus) 
            {
                case Start:
                    return "Start";
                case Complete:
                    return "Complete";
                case Fail:
                    return "Fail";
                default:
                    return "";
            }
        }

        std::string GAEvents::errorSeverityString(EGAErrorSeverity errorSeverity)
        {
            switch (errorSeverity) 
            {
                case Info:
                    return "info";
                case Debug:
                    return "debug";
                case Warning:
                    return "warning";
                case Error:
                    return "error";
                case Critical:
                    return "critical";
                default:
                    return "";
            }
        }

        std::string GAEvents::resourceFlowTypeString(EGAResourceFlowType flowType)
        {
            switch (flowType) 
            {
                case Source:
                    return "Source";
                case Sink:
                    return "Sink";
                default:
                    return "";
            }
        }
    }
}
