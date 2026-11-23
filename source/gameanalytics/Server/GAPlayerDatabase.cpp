#include "Server/GAPlayerDatabase.h"
#include "GAUtilities.h"
#include "GALogger.h"
#include "GADevice.h"
#include "GASerialize.h"
#include "GAStore.h"
#include "GAThreading.h"

namespace gameanalytics
{
    PlayerNotFound::PlayerNotFound(std::string const& userId):
        std::runtime_error(utilities::printString("User does not exist: %s", userId.c_str())),
        userId(userId)
    {
    }

    PlayerDatabase::PlayerDatabase(int sizeHint)
    {
        if(sizeHint > 0)
        {
            _players.reserve(sizeHint);
        }
    }

    bool PlayerDatabase::addPlayer(Player&& player)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        const std::string uid = player.getUserId();

        if(playerExists(uid))
        {
            logging::GALogger::w("User id is already in use: %s", uid.c_str());
            return false;
        }

        _players[uid] = std::forward<Player>(player);
        persistPlayer(_players[uid]);
        return true;
    }

    bool PlayerDatabase::removePlayer(std::string const& uid)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        if(!playerExists(uid))
        {
            logging::GALogger::w("Invalid user id: %s", uid.c_str());
            return false;
        }

        Player& p = getPlayer(uid);
        if(p.isActive())
        {
            logging::GALogger::w("Cannot remove player while the player is active, user id: %s", uid.c_str());
            return false;
        }

        _players.erase(uid);
        eraseFromStore(uid);
        return true;
    }

    bool PlayerDatabase::changePlayerId(std::string const& uid, std::string const& newUserId)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        if(!playerExists(uid))
        {
            logging::GALogger::w("Invalid user id: %s", uid.c_str());
            return false;
        }

        if(playerExists(newUserId))
        {
            logging::GALogger::w("User id is already in use: %s", uid.c_str());
            return false;
        }

        Player& player = getPlayer(uid);
        if(player.isActive())
        {
            logging::GALogger::w("Cannot change user id while the user is active, user id: %s", uid.c_str());
            return false;
        }

        player._userId = newUserId;
        _players[newUserId] = std::move(player);
        _players.erase(uid);

        eraseFromStore(uid);
        persistPlayer(_players[newUserId]);
        return true;
    }

    size_t PlayerDatabase::loadFromStore()
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        if(!store::GAStore::getTableReady())
        {
            logging::GALogger::w("Player database: store is not ready, players were not restored");
            return 0;
        }

        size_t restored = 0;
        std::unordered_map<std::string, bool> inStore;

        try
        {
            json rows;
            store::GAStore::getPlayers(rows);

            if(rows.is_array())
            {
                inStore.reserve(rows.size());

                for(json& row : rows)
                {
                    const std::string uid  = utilities::getOptionalValue<std::string>(row, "user_id", "");
                    const std::string data = utilities::getOptionalValue<std::string>(row, "data", "");

                    if(uid.empty() || data.empty())
                    {
                        continue;
                    }

                    inStore[uid] = true;

                    // players registered before the store was loaded take precedence
                    if(playerExists(uid))
                    {
                        continue;
                    }

                    Player player;
                    if(!player.deserialize(data) || player.getUserId() != uid)
                    {
                        logging::GALogger::w("Player database: dropping corrupt entry for user id: %s", uid.c_str());
                        eraseFromStore(uid);
                        continue;
                    }

                    _players[uid] = std::move(player);
                    ++restored;
                }
            }
        }
        catch(std::exception& e)
        {
            logging::GALogger::e("Player database: failed to restore players: %s", e.what());
        }

        _storeLoaded = true;

        // write back everything that was registered while the store was unavailable
        for(auto& [uid, player] : _players)
        {
            if(!inStore.count(uid))
            {
                persistPlayer(player);
            }
        }

        logging::GALogger::i("Player database: restored %zu players, %zu registered in total", restored, _players.size());
        return restored;
    }

    bool PlayerDatabase::isStoreLoaded() const
    {
        return _storeLoaded;
    }

    void PlayerDatabase::persistPlayer(Player const& player)
    {
        const std::string uid = player.getUserId();
        if(uid.empty())
        {
            return;
        }

        const std::string data = player.serialize();
        if(data.empty())
        {
            logging::GALogger::w("Player database: could not serialize player, user id: %s", uid.c_str());
            return;
        }

        writeToStore(uid, data);
    }

    bool PlayerDatabase::persistPlayer(std::string const& uid)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        if(!playerExists(uid))
        {
            logging::GALogger::w("Invalid user id: %s", uid.c_str());
            return false;
        }

        persistPlayer(_players[uid]);
        return true;
    }

    void PlayerDatabase::persistAll()
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        for(auto& [uid, player] : _players)
        {
            persistPlayer(player);
        }
    }

    void PlayerDatabase::writeToStore(std::string const& uid, std::string const& data)
    {
        threading::GAThreading::performTaskOnGAThread(
            [uid, data]()
            {
                // before initialize the store does not exist yet; loadFromStore
                // writes the registered players back once it does
                if(store::GAStore::getTableReady())
                {
                    store::GAStore::setPlayer(uid, data);
                }
            }
        );
    }

    void PlayerDatabase::eraseFromStore(std::string const& uid)
    {
        threading::GAThreading::performTaskOnGAThread(
            [uid]()
            {
                if(store::GAStore::getTableReady())
                {
                    store::GAStore::removePlayer(uid);
                }
            }
        );
    }

    Player& PlayerDatabase::getPlayer(std::string const& userId)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);
        
        if(!playerExists(userId))
        {
            logging::GALogger::e("Invalid user id: %s", userId.c_str());
            throw PlayerNotFound(userId);
        }

        return _players[userId];
    }

    size_t PlayerDatabase::countPlayers() const
    {
        return _players.size();
    }

    bool PlayerDatabase::playerExists(std::string const& userId) const
    {
        return _players.count(userId);
    }

    json PlayerDatabase::getInitAnnotations(Player const& p)
    {
        json data;

        data["user_id"]      = p.getUserId();
        data["sdk_version"]  = device::GADevice::getRelevantSdkVersion();
        data["os_version"]   = p.osVersion;
        data["device"]       = p.device;
        data["manufacturer"] = p.manufacturer;
        data["platform"]     = p.platform;
        data["session_id"]   = p._sessionId;
        data["session_num"]  = data["random_salt"] = p._sessionNum;

        utilities::addIfNotEmpty(data, "build", device::GADevice::getBuildPlatform());
        utilities::addIfNotEmpty(data, "engine_version", device::GADevice::getGameEngineVersion());

        return data;
    }

    json PlayerDatabase::getPlayerAnnotations(Player const& p)
    {
        json data;

        data["v"] = 2;
        data["event_uuid"] = utilities::GAUtilities::generateUUID();
        
        utilities::addIfNotEmpty(data, "build", device::GADevice::getBuildPlatform());
        utilities::addIfNotEmpty(data, "engine_version", device::GADevice::getGameEngineVersion());

        data["sdk_version"]  = device::GADevice::getRelevantSdkVersion();
            
        data["user_id"]                 = p._userId;
        data["session_num"]             = p._sessionNum;
        data["current_session_length"]  = p._sessionLength;
        data["lifetime_session_length"] = p._totalSessionLength;
        data["os_version"]              = p.osVersion;
        data["device"]                  = p.device;
        data["manufacturer"]            = p.manufacturer;
        data["platform"]                = p.platform;
        data["connection_type"]         = p.connectionType;

        utilities::addIfNotEmpty(data, "country_code", p.countryCode);

        utilities::addIfNotEmpty(data, "ab_id", p._abId);
        utilities::addIfNotEmpty(data, "ab_variant_id", p._abVariantId); 
        utilities::addIfNotEmpty(data, "user_id_ext", p.extUserId);

        utilities::addIfNotEmpty(data, "custom_01", p._customDimension1);
        utilities::addIfNotEmpty(data, "custom_02", p._customDimension2);
        utilities::addIfNotEmpty(data, "custom_03", p._customDimension3);

        if(!p.customFields.isEmpty())
        {
            json fields = serializeCustomFields(p.customFields);
            data["custom_fields"] = fields;
        }

        if(!p._remoteConfigs.empty())
        {
            try
            {
                json configs = json::parse(p._remoteConfigs);
                for(json& obj : configs)
                {
                    json cfg;
                    cfg["vsn"] = utilities::getOptionalValue<int>(obj, "vsn", 0);
                    cfg["key"] = utilities::getOptionalValue<std::string>(obj, "key", "");
                    cfg["id"]  = utilities::getOptionalValue<std::string>(obj, "id", "");

                    data["configurations_v3"].push_back(cfg);
                }

            }
            catch(std::exception& e)
            {
                logging::GALogger::e("Failed to parse remote configs: %s", e.what());
            }
        }
        
        return data;
    }
}