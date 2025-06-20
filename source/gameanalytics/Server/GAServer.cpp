#include "GAServer.h"
#include "Server/GAPlayerDatabase.h"
#include "GALogger.h"
#include "GameAnalytics/GameAnalytics.h"
#include "GADevice.h"
#include "GAHTTPApi.h"
#include "GAEvents.h"
#include "GAThreading.h"
#include "GAState.h"
#include "GAValidator.h"

namespace gameanalytics
{
    GameAnalyticsServer::GameAnalyticsServer(std::string const& serverId, std::string const& extServerId, std::string const& build):
        _playerDatabase(std::make_unique<PlayerDatabase>())
    {
        GameAnalytics::configureUserId(serverId);
        GameAnalytics::configureBuild(build);
        GameAnalytics::configureExternalUserId(extServerId);
    }

    bool GameAnalyticsServer::isExistingPlayer(std::string const& uid) const
    {
        return _playerDatabase->playerExists(uid);
    }

    bool GameAnalyticsServer::addPlayer(Player&& player)
    {
        return _playerDatabase->addPlayer(std::forward<Player>(player));
    }

    bool GameAnalyticsServer::removePlayer(std::string const& uid) const
    {
        return _playerDatabase->removePlayer(uid);
    }

    Player& GameAnalyticsServer::getPlayer(std::string const& uid) const
    {
        return _playerDatabase->getPlayer(uid);
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

    void GameAnalyticsServer::addServerDesignEvent(std::string const& eventId, double value, std::string const& customFields)
    {
        GameAnalytics::addDesignEvent(eventId, value, customFields);
    }
    
    void GameAnalyticsServer::addServerBusinessEvent(std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, std::string const& customFields)
    {
        GameAnalytics::addBusinessEvent(currency, amount, itemType, itemId, cartType, customFields);
    }
    
    void GameAnalyticsServer::addServerResourceEvent(EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, std::string const& customFields)
    {
        GameAnalytics::addResourceEvent(flowType, currency, amount, itemType, itemId, customFields);
    }
    
    void GameAnalyticsServer::addServerErrorEvent(EGAErrorSeverity severity, std::string const& message, std::string const& customFields)
    {
        GameAnalytics::addErrorEvent(severity, message, customFields);
    }

    bool GameAnalyticsServer::startPlayerSessionInternal(std::string const& userId)
    {
        // check if it's a new player
        if(!isExistingPlayer(userId))
        {
            Player newPlayer(userId);
            if(!addPlayer(std::move(newPlayer)))
            {
                logging::GALogger::e("Failed to register new player: %s", userId.c_str());
                return false;
            }
        }

        Player& player = getPlayer(userId);
        if(player.isActive())
        {
            logging::GALogger::w("A session for player %s already exists", userId.c_str());
            return false;
        }

        const bool isNewPlayer = player.isNewUser();
        player.onJoin();

        json initAnnotations = PlayerDatabse::getInitAnnotations(player);

        // call the init call
        http::GAHTTPApi& httpApi = http::GAHTTPApi::getInstance();

        json initResponseDict;
        http::EGAHTTPApiResponse initResponse = httpApi.requestInitReturningDict(initResponseDict, initAnnotations, player.GetConfigsHash());

        // init is ok
        if ((initResponse == http::Ok || initResponse == http::Created) && !initResponseDict.empty())
        {
            player._configsHash = utilities::getOptionalValue<std::string>(initResponseDict, "configs_hash");
            player._abId        = utilities::getOptionalValue<std::string>(initResponseDict, "ab_id");
            player._abVariantId = utilities::getOptionalValue<std::string>(initResponseDict, "ab_variant_id");

            if(_playerCallbacks)
            {
                if(isNewPlayer)
                {
                    _playerCallbacks->onNewPlayer(player);
                }

                _playerCallbacks->onPlayerJoin(player);
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

        Player& player = getPlayer(userId);
        
        // Event specific data
        json eventDict = PlayerDatabase::getPlayerAnnotations(player);

        eventDict["category"]   = events::GAEvents::CategorySessionEnd;
        eventDict["length"]     = player.currentSessionPlaytime();

        events::GAEvents::getInstance().addEventToStore(eventDict);

        if(_playerCallbacks)
            _playerCallbacks->onPlayerExit(player);
        
        player.onExit();

        return true;
    }

    void GameAnalyticsServer::addPlayerDesignEventInternal(Player& player, std::string const& eventId, double value, std::string const& customFields)
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
            json eventData = PlayerDatabase::getPlayerAnnotations(player);
            
            eventData["category"] = events::GAEvents::CategoryDesign;
            eventData["event_id"] = eventId;
            eventData["value"]    = value;

            // Log
            logging::GALogger::i("Add DESIGN event: {eventId:%s, value:%f, fields:%s}", eventId.c_str(), value, cleanedFields.dump(JSON_PRINT_INDENT).c_str());

            // Send to store
            events::GAEvents::getInstance().addEventToStore(eventData);
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

    void GameAnalyticsServer::addPlayerErrorEventInternal(Player& player, EGAErrorSeverity severity, std::string const& message, std::string const& function, int32_t line, const json& fields)
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
            json eventData = PlayerDatabase::getPlayerAnnotations(player);

            eventData["category"] = events::GAEvents::CategoryError;
            eventData["severity"] = events::GAEvents::errorSeverityString(severity);
            eventData["message"]  = message;

            constexpr int MAX_FUNCTION_LEN = 256;
            if(!function.empty())
            {
                eventData["function_name"] = utilities::trimString(function, MAX_FUNCTION_LEN);

                if(line >= 0)
                {
                    eventData["line_number"] = line;
                }
            }

                // Log
                logging::GALogger::i("Add ERROR event: {severity:%s, message:%s, fields:%s}", 
                    events::GAEvents::errorSeverityString(severity).c_str(), message.c_str(), cleanedFields.dump(JSON_PRINT_INDENT).c_str());

                // Send to store
                events::GAEvents::getInstance().addEventToStore(eventData);
        }
        catch(std::exception& e)
        {
            logging::GALogger::e("addErrorEvent - Exception thrown: %s", e.what());
        }
    }

    void GameAnalyticsServer::addPlayerBusinessEventInternal(Player& player, std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, std::string const& customFields = "")
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

                const int64_t transactionNum = player.getTransactionNum();

                eventDict["category"] = events::GAEvents::CategoryBusiness;
                eventDict["event_id"] = itemType + ':' + itemId;
                eventDict["currency"] = currency;
                eventDict["amount"]   = amount;
                eventDict["transaction_num"] = transactionNum;

                utilities::addIfNotEmpty(eventDict, "cart_type", cartType);
                
                // Log
                logging::GALogger::i("Add BUSINESS event: {currency:%s, amount:%d, itemType:%s, itemId:%s, cartType:%s, fields:%s}",
                    currency.c_str(), amount, itemType.c_str(), itemId.c_str(), cartType.c_str(), cleanedFields.dump(JSON_PRINT_INDENT).c_str());

                // Send to store
                events::GAEvents::getInstance().getInstance().addEventToStore(eventDict);
            } 
            catch (std::exception const& e)
            {
                logging::GALogger::e("addBusinessEvent - Exception thrown: %s", e.what());
            }
    }
}