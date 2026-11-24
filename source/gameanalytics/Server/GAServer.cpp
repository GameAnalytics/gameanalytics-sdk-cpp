#include "GameAnalytics/Server/GAServer.h"
#include "Server/GAPlayerDatabase.h"
#include "GALogger.h"
#include "GameAnalytics/GameAnalytics.h"
#include "GADevice.h"
#include "GAHTTPApi.h"
#include "GAEvents.h"
#include "GAThreading.h"
#include "GAState.h"
#include "GAValidator.h"
#include "GASerialize.h"
#include "GAUtilities.h"

namespace gameanalytics
{
    GameAnalyticsServer::GameAnalyticsServer(std::string const& serverId, std::string const& extServerId, std::string const& build, int numPlayersHint):
        _playerDatabase(std::make_shared<PlayerDatabase>(numPlayersHint))
    {
        GameAnalytics::configureUserId(serverId);
        GameAnalytics::configureExternalUserId(extServerId);
        GameAnalytics::configureBuild(build);
    }

    void GameAnalyticsServer::initialize(std::string const& gameKey, std::string const& secretKey)
    {
        GameAnalytics::initialize(gameKey, secretKey);

        // runs after the initialize task above, once the sqlite store is open
        std::shared_ptr<PlayerDatabase> db = _playerDatabase;
        threading::GAThreading::performTaskOnGAThread(
            [db]()
            {
                db->loadFromStore();
            }
        );
    }

    void GameAnalyticsServer::configureAvailableCurrencyTypes(StringVector const& validTypes)
    {
        GameAnalytics::configureAvailableResourceCurrencies(validTypes);
    }

    void GameAnalyticsServer::configureAvailableResourceItemTypes(StringVector const& validTypes)
    {
        GameAnalytics::configureAvailableResourceItemTypes(validTypes);
    }

    void GameAnalyticsServer::setPlayerCallbacks(std::shared_ptr<PlayerCallbacks> callbacks)
    {
        _playerCallbacks = callbacks;
    }

    bool GameAnalyticsServer::isExistingPlayer(std::string const& uid) const
    {
        return _playerDatabase->playerExists(uid);
    }

    bool GameAnalyticsServer::addPlayer(Player&& player)
    {
        return _playerDatabase->addPlayer(std::forward<Player>(player));
    }

    bool GameAnalyticsServer::removePlayer(std::string const& uid)
    {
        return _playerDatabase->removePlayer(uid);
    }

    PlayerPtr GameAnalyticsServer::getPlayer(std::string const& uid) const
    {
        return _playerDatabase->getPlayer(uid);
    }

    PlayerPtr GameAnalyticsServer::findPlayer(std::string const& uid) const
    {
        return _playerDatabase->findPlayer(uid);
    }

    void GameAnalyticsServer::withPlayer(std::string const& userId, std::function<void(Player&)> fn)
    {
        std::shared_ptr<PlayerDatabase> db = _playerDatabase;
        threading::GAThreading::performTaskOnGAThread(
            [db, userId, fn = std::move(fn)]()
            {
                PlayerPtr player = db->findPlayer(userId);
                if(!player)
                {
                    logging::GALogger::w("Invalid user id: %s", userId.c_str());
                    return;
                }

                fn(*player);
            }
        );
    }

    void GameAnalyticsServer::withPlayer(PlayerPtr const& player, std::function<void(Player&)> fn)
    {
        if(!player)
        {
            logging::GALogger::w("Null player");
            return;
        }

        threading::GAThreading::performTaskOnGAThread(
            [player, fn = std::move(fn)]()
            {
                if(player->isRemoved())
                {
                    logging::GALogger::w("Player %s was removed, event dropped", player->getUserId().c_str());
                    return;
                }

                fn(*player);
            }
        );
    }

    bool GameAnalyticsServer::savePlayer(std::string const& uid)
    {
        return _playerDatabase->persistPlayer(uid);
    }

    void GameAnalyticsServer::saveAllPlayers()
    {
        _playerDatabase->persistAll();
    }

    size_t GameAnalyticsServer::getPlayerCount() const
    {
        return _playerDatabase->countPlayers();
    }

    void GameAnalyticsServer::setPlayerRetention(int maxInactiveDays, size_t maxStoredPlayers)
    {
        constexpr int64_t SECONDS_PER_DAY = 24 * 60 * 60;
        _playerDatabase->setRetention(maxInactiveDays > 0 ? maxInactiveDays * SECONDS_PER_DAY : 0, maxStoredPlayers);
    }

    size_t GameAnalyticsServer::evictPlayersInactiveSince(int days)
    {
        constexpr int64_t SECONDS_PER_DAY = 24 * 60 * 60;
        if(days < 0)
        {
            return 0;
        }

        return _playerDatabase->evictPlayersInactiveSince(utilities::GAUtilities::timeIntervalSince1970() - days * SECONDS_PER_DAY);
    }

    void GameAnalyticsServer::startPlayerSession(std::string const& userId)
    {
        threading::GAThreading::performTaskOnGAThread(
            [this, userId]()
            {
                startPlayerSessionInternal(userId);
            }
        );
    }

    void GameAnalyticsServer::endPlayerSession(std::string const& userId)
    {
        threading::GAThreading::performTaskOnGAThread(
            [this, userId]()
            {
                endPlayerSessionInternal(userId);
            }
        );
    }

    void GameAnalyticsServer::addServerDesignEvent(std::string const& eventId, double value, CustomFields const& customFields)
    {
        GameAnalytics::addDesignEvent(eventId, value, customFields.toString());
    }
    
    void GameAnalyticsServer::addServerBusinessEvent(std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, CustomFields const& customFields)
    {
        GameAnalytics::addBusinessEvent(currency, amount, itemType, itemId, cartType, customFields.toString());
    }
    
    void GameAnalyticsServer::addServerResourceEvent(EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, CustomFields const& customFields)
    {
        GameAnalytics::addResourceEvent(flowType, currency, amount, itemType, itemId, customFields.toString());
    }
    
    void GameAnalyticsServer::addServerErrorEvent(EGAErrorSeverity severity, std::string const& message, CustomFields const& customFields)
    {
        GameAnalytics::addErrorEvent(severity, message, customFields.toString());
    }

    void GameAnalyticsServer::addServerProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, bool sendScore, CustomFields const& customFields)
    {
        // the public SDK wrappers always send the score, so go to the event layer
        // directly to honor sendScore
        const std::string fields = customFields.toString();
        threading::GAThreading::performTaskOnGAThread([=]()
        {
            if(!GameAnalytics::isSdkReady(true, true, "Could not add progression event"))
            {
                return;
            }

            try
            {
                json fieldsJson = utilities::parseFields(fields);
                events::GAEvents::addProgressionEvent(progressionStatus, progression01, progression02, progression03, score, sendScore, fieldsJson, false);
            }
            catch(std::exception const& e)
            {
                logging::GALogger::e("addServerProgressionEvent - Exception thrown: %s", e.what());
            }
        });
    }

    bool GameAnalyticsServer::startPlayerSessionInternal(std::string const& userId)
    {
        // check if it's a new player
        if(!isExistingPlayer(userId))
        {
            Player newPlayer(userId, "");
            if(!addPlayer(std::move(newPlayer)))
            {
                logging::GALogger::e("Failed to register new player: %s", userId.c_str());
                return false;
            }
        }

        PlayerPtr playerPtr = _playerDatabase->findPlayer(userId);
        if(!playerPtr)
        {
            logging::GALogger::e("Player %s disappeared before the session could start", userId.c_str());
            return false;
        }

        Player& player = *playerPtr;
        if(player.isActive())
        {
            logging::GALogger::w("A session for player %s already exists", userId.c_str());
            return false;
        }

        // onJoin has not run yet, so derive newness from the persisted session count
        const bool isNewPlayer = player.getSessionCount() == 0;

        json initAnnotations = PlayerDatabase::getInitAnnotations(player);

        // call the init call
        http::GAHTTPApi& httpApi = http::GAHTTPApi::getInstance();

        json initResponseDict;
        http::EGAHTTPApiResponse initResponse = httpApi.requestInitReturningDict(initResponseDict, initAnnotations, player.getConfigsHash());

        // init is ok
        if ((initResponse == http::Ok || initResponse == http::Created) && !initResponseDict.empty())
        {
            PlayerDatabase::applyInitResponse(player, initResponseDict);

            if(_playerCallbacks)
            {
                if(isNewPlayer)
                {
                    _playerCallbacks->onNewPlayer(player);
                }

                _playerCallbacks->onPlayerJoin(player);
            }

            try
            {
                player.onJoin();
                _playerDatabase->persistPlayer(player);

                if(!state::GAState::isEventSubmissionEnabled())
                {
                    return false;
                }

                // Event specific data
                json eventDict = PlayerDatabase::getPlayerAnnotations(player);
                eventDict["category"] = events::GAEvents::CategorySessionStart;

                events::GAEvents::getInstance().addEventToStore(eventDict);

                // Log
                logging::GALogger::i("Add SESSION START event: userId = %s", userId.c_str());
            }
            catch(const std::exception& e)
            {
                logging::GALogger::e("addSessionStartEvent - Exception thrown: %s", e.what());
            }

            return true;
        }
        else
        {
            player._isActive = false;
            return false;
        }
    }

    bool GameAnalyticsServer::endPlayerSessionInternal(std::string const& userId)
    {
        if(!isExistingPlayer(userId))
        {
            return false;
        }

        PlayerPtr playerPtr = _playerDatabase->findPlayer(userId);
        if(!playerPtr)
        {
            return false;
        }

        Player& player = *playerPtr;
        if(!player.isActive())
        {
            logging::GALogger::w("No active session for player %s, ignoring session end", userId.c_str());
            return false;
        }

        player.onExit();

        if(_playerCallbacks)
            _playerCallbacks->onPlayerExit(player);

        _playerDatabase->persistPlayer(player);

        if(!state::GAState::isEventSubmissionEnabled())
        {
            return false;
        }
        
        // Event specific data
        json eventDict = PlayerDatabase::getPlayerAnnotations(player);

        eventDict["category"]   = events::GAEvents::CategorySessionEnd;
        eventDict["length"]     = player.getLastSessionLength();

        events::GAEvents::getInstance().addEventToStore(eventDict);

        // Log
        logging::GALogger::i("Add SESSION END event: userId = %s", userId.c_str());

        return true;
    }

    void GameAnalyticsServer::addPlayerDesignEventInternal(Player& player, std::string const& eventId, double value, CustomFields const& customFields)
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
                http::GAHTTPApi& httpInstance = http::GAHTTPApi::getInstance();
                httpInstance.sendSdkErrorEvent(validationResult.category, validationResult.area, validationResult.action, validationResult.parameter, validationResult.reason, state::GAState::getGameKey(), state::GAState::getGameSecret());
                return;
            }

            // Create empty eventData
            json eventDict = PlayerDatabase::getPlayerAnnotations(player);
            
            eventDict["category"] = events::GAEvents::CategoryDesign;
            eventDict["event_id"] = eventId;
            eventDict["value"]    = value;

            if(!customFields.isEmpty())
            {
                json& fields = eventDict["custom_fields"];
                fields.merge_patch(serializeCustomFields(customFields));
            }

            // Log
            logging::GALogger::i("Add DESIGN event: {eventId:%s, value:%f, fields:%s}", eventId.c_str(), value, customFields.toString().c_str());

            // Send to store
            events::GAEvents::getInstance().addEventToStore(eventDict);
        }
        catch(json::exception const& e)
        {
            logging::GALogger::e("addDesignEvent - Failed to parse json: %s", e.what());
        }
        catch(std::exception const& e)
        {
            logging::GALogger::e("addDesignEvent - Exception thrown: %s", e.what());
        }
    }

    void GameAnalyticsServer::addPlayerErrorEventInternal(Player& player, EGAErrorSeverity severity, std::string const& message, CustomFields const& customFields)
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
                http::GAHTTPApi& httpInstance = http::GAHTTPApi::getInstance();
                httpInstance.sendSdkErrorEvent(validationResult.category, validationResult.area, validationResult.action, validationResult.parameter, validationResult.reason, state::GAState::getGameKey(), state::GAState::getGameSecret());
                return;
            }

            // Create empty eventData
            json eventDict = PlayerDatabase::getPlayerAnnotations(player);

            eventDict["category"] = events::GAEvents::CategoryError;
            eventDict["severity"] = events::GAEvents::errorSeverityString(severity);
            eventDict["message"]  = message;

            utilities::FunctionInfo f = utilities::getRelevantFunctionFromCallStack();

            constexpr int MAX_FUNCTION_LEN = 256;
            if(!f.functionName.empty())
            {
                eventDict["function_name"] = utilities::trimString(f.functionName, MAX_FUNCTION_LEN);

                if(f.lineNumber >= 0)
                {
                    eventDict["line_number"] = f.lineNumber;
                }
            }

            if(!customFields.isEmpty())
            {
                json& fields = eventDict["custom_fields"];
                fields.merge_patch(serializeCustomFields(customFields));
            }

            // Log
            logging::GALogger::i("Add ERROR event: {severity:%s, message:%s, fields:%s}", 
                events::GAEvents::errorSeverityString(severity).c_str(), message.c_str(), customFields.toString().c_str());

            // Send to store
            events::GAEvents::getInstance().addEventToStore(eventDict);
        }
        catch(std::exception& e)
        {
            logging::GALogger::e("addErrorEvent - Exception thrown: %s", e.what());
        }
    }

    void GameAnalyticsServer::addPlayerBusinessEventInternal(Player& player, std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, CustomFields const& customFields)
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
                http::GAHTTPApi& httpInstance = http::GAHTTPApi::getInstance();
                httpInstance.sendSdkErrorEvent(validationResult.category, validationResult.area, validationResult.action, validationResult.parameter, validationResult.reason, state::GAState::getGameKey(), state::GAState::getGameSecret());
                return;
            }

            // Create empty eventData
            json eventDict = PlayerDatabase::getPlayerAnnotations(player);

            const int64_t transactionNum = player.getNextTransactionNum();

            eventDict["category"] = events::GAEvents::CategoryBusiness;
            eventDict["event_id"] = itemType + ':' + itemId;
            eventDict["currency"] = currency;
            eventDict["amount"]   = amount;
            eventDict["transaction_num"] = transactionNum;

            utilities::addIfNotEmpty(eventDict, "cart_type", cartType);

            if(!customFields.isEmpty())
            {
                json& fields = eventDict["custom_fields"];
                fields.merge_patch(serializeCustomFields(customFields));
            }
                
            // Log
            logging::GALogger::i("Add BUSINESS event: {currency:%s, amount:%d, itemType:%s, itemId:%s, cartType:%s, fields:%s}",
                    currency.c_str(), amount, itemType.c_str(), itemId.c_str(), cartType.c_str(), customFields.toString().c_str());

            // Send to store
            events::GAEvents::getInstance().getInstance().addEventToStore(eventDict);
        } 
        catch (std::exception const& e)
        {
            logging::GALogger::e("addBusinessEvent - Exception thrown: %s", e.what());
        }
    }

    void GameAnalyticsServer::addPlayerResourceEventInternal(Player& player, EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, CustomFields const& customFields)
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
                http::GAHTTPApi& httpInstance = http::GAHTTPApi::getInstance();
                httpInstance.sendSdkErrorEvent(validationResult.category, validationResult.area, validationResult.action, validationResult.parameter, validationResult.reason, state::GAState::getGameKey(), state::GAState::getGameSecret());
                return;
            }

            // If flow type is sink reverse amount
            if (flowType == Sink)
            {
                amount *= -1;
            }

            // Create empty eventData
            json eventDict = PlayerDatabase::getPlayerAnnotations(player);
                
            const std::string flowStr = events::GAEvents::resourceFlowTypeString(flowType);

            // insert event specific values
            eventDict["event_id"] = flowStr + ':' + currency + ':' + itemType + ':' + itemId;
            eventDict["category"] = events::GAEvents::CategoryResource;
            eventDict["amount"]   = amount;

            if(!customFields.isEmpty())
            {
                json& fields = eventDict["custom_fields"];
                fields.merge_patch(serializeCustomFields(customFields));
            }

            // Log
            logging::GALogger::i("Add RESOURCE event: {currency:%s, amount: %f, itemType:%s, itemId:%s, fields:%s}", 
                    currency.c_str(), amount, itemType.c_str(), itemId.c_str(), customFields.toString().c_str());

            // Send to store
            events::GAEvents::getInstance().addEventToStore(eventDict);
        }
        catch(const json::exception& e)
        {
            logging::GALogger::e("addResourceEvent - Failed to parse json: %s", e.what());
        }
        catch(const std::exception& e)
        {
            logging::GALogger::e("addResourceEvent - Exception thrown: %s", e.what());
        }
    }
    
    void GameAnalyticsServer::addPlayerProgressionEventInternal(Player& player, EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, bool sendScore, CustomFields const& customFields)
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
                http::GAHTTPApi& httpInstance = http::GAHTTPApi::getInstance();
                httpInstance.sendSdkErrorEvent(validationResult.category, validationResult.area, validationResult.action, validationResult.parameter, validationResult.reason, state::GAState::getGameKey(), state::GAState::getGameSecret());
                return;
            }

            // Create empty eventData
            json eventDict = PlayerDatabase::getPlayerAnnotations(player);

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

            const std::string statusString = events::GAEvents::progressionStatusString(progressionStatus);

            eventDict["category"] = events::GAEvents::CategoryProgression;
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
                player.progressionTries.incrementTries(progressionIdentifier);
                _playerDatabase->persistPlayer(player);
            }

            // increment and add attempt_num on complete and delete persisted
            if (progressionStatus == EGAProgressionStatus::Complete)
            {
                // Increment attempt number
                player.progressionTries.incrementTries(progressionIdentifier);

                // Add to event
                attempt_num = player.progressionTries.getTries(progressionIdentifier);
                eventDict["attempt_num"] = attempt_num;

                if(!customFields.isEmpty())
                {
                    json& fields = eventDict["custom_fields"];
                    fields.merge_patch(serializeCustomFields(customFields));
                }

                // Clear
                player.progressionTries.remove(progressionIdentifier);
                _playerDatabase->persistPlayer(player);
            }

            // Log
            logging::GALogger::i("Add PROGRESSION event: {status:%s, progression01:%s, progression02:%s, progression03:%s, score:%d, attempt:%d, fields:%s}", 
                    statusString.c_str(), progression01.c_str(), progression02.c_str(), progression03.c_str(), score, attempt_num, customFields.toString().c_str());

            // Send to store
            events::GAEvents::getInstance().addEventToStore(eventDict);
        }
        catch(std::exception& e)
        {
            logging::GALogger::e("addProgressionEvent - Exception thrown: %s", e.what());
        }
    }

    void GameAnalyticsServer::addPlayerDesignEvent(PlayerPtr const& player, std::string const& eventId, double value, CustomFields const& customFields)
    {
        withPlayer(player, [this, eventId, value, customFields](Player& p)
        {
            addPlayerDesignEventInternal(p, eventId, value, customFields);
        });
    }

    void GameAnalyticsServer::addPlayerBusinessEvent(PlayerPtr const& player, std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, CustomFields const& customFields)
    {
        withPlayer(player, [this, currency, amount, itemType, itemId, cartType, customFields](Player& p)
        {
            addPlayerBusinessEventInternal(p, currency, amount, itemType, itemId, cartType, customFields);
        });
    }

    void GameAnalyticsServer::addPlayerResourceEvent(PlayerPtr const& player, EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, CustomFields const& customFields)
    {
        withPlayer(player, [this, flowType, currency, amount, itemType, itemId, customFields](Player& p)
        {
            addPlayerResourceEventInternal(p, flowType, currency, amount, itemType, itemId, customFields);
        });
    }

    void GameAnalyticsServer::addPlayerErrorEvent(PlayerPtr const& player, EGAErrorSeverity severity, std::string const& message, CustomFields const& customFields)
    {
        withPlayer(player, [this, severity, message, customFields](Player& p)
        {
            addPlayerErrorEventInternal(p, severity, message, customFields);
        });
    }

    void GameAnalyticsServer::addPlayerProgressionEvent(PlayerPtr const& player, EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, bool sendScore, CustomFields const& customFields)
    {
        withPlayer(player, [this, progressionStatus, progression01, progression02, progression03, score, sendScore, customFields](Player& p)
        {
            addPlayerProgressionEventInternal(p, progressionStatus, progression01, progression02, progression03, score, sendScore, customFields);
        });
    }

    void GameAnalyticsServer::addPlayerDesignEvent(std::string const& userId, std::string const& eventId, double value, CustomFields const& customFields)
    {
        withPlayer(userId, [this, eventId, value, customFields](Player& p)
        {
            addPlayerDesignEventInternal(p, eventId, value, customFields);
        });
    }

    void GameAnalyticsServer::addPlayerBusinessEvent(std::string const& userId, std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, CustomFields const& customFields)
    {
        withPlayer(userId, [this, currency, amount, itemType, itemId, cartType, customFields](Player& p)
        {
            addPlayerBusinessEventInternal(p, currency, amount, itemType, itemId, cartType, customFields);
        });
    }

    void GameAnalyticsServer::addPlayerResourceEvent(std::string const& userId, EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, CustomFields const& customFields)
    {
        withPlayer(userId, [this, flowType, currency, amount, itemType, itemId, customFields](Player& p)
        {
            addPlayerResourceEventInternal(p, flowType, currency, amount, itemType, itemId, customFields);
        });
    }

    void GameAnalyticsServer::addPlayerErrorEvent(std::string const& userId, EGAErrorSeverity severity, std::string const& message, CustomFields const& customFields)
    {
        withPlayer(userId, [this, severity, message, customFields](Player& p)
        {
            addPlayerErrorEventInternal(p, severity, message, customFields);
        });
    }

    void GameAnalyticsServer::addPlayerProgressionEvent(std::string const& userId, EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, bool sendScore, CustomFields const& customFields)
    {
        withPlayer(userId, [this, progressionStatus, progression01, progression02, progression03, score, sendScore, customFields](Player& p)
        {
            addPlayerProgressionEventInternal(p, progressionStatus, progression01, progression02, progression03, score, sendScore, customFields);
        });
    }
}
