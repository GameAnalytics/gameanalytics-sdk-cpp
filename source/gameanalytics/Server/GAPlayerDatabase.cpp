#include "Server/GAPlayerDatabase.h"
#include "GAUtilities.h"
#include "GALogger.h"

namespace gameanalytics
{
    extern json         serializeCustomFields(CustomFields const& customFields);
    extern CustomFields deserializeCustomFields(json const& jsonFields);

    PlayerNotFound::PlayerNotFound(std::string const& userId):
        std::runtime_error(utilities::printString("User does not exist: %s", userId.c_str())),
        userId(userId)
    {
    }

    PlayerDatabase::PlayerDatabase(int sizeHint)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        if(sizeHint > 0)
        {
            _players.reserve(sizeHint);
        }
    }

    bool PlayerDatabase::addPlayer(Player&& player)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        const std::string uid = player.getUserId();

        if(PlayerExists(uid))
        {
            logging::GALogger::w("User id is already in use: %s", userId.c_str());
            return false;
        }

        _players[uid] = std::forward<Player>(player);
        return true;
    }

    bool PlayerDatabase::removePlayer(std::string const& uid)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        if(!PlayerExists(uid))
        {
            logging::GALogger::w("Invalid user id: %s", userId.c_str());
            return false;
        }

        _players.erase(uid);
        return true;
    }

    bool PlayerDatabase::changePlayerId(std::string const& oldUserId, std::string const& newUserId)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        if(!playerExists(oldUserId))
        {
            logging::GALogger::w("Invalid user id: %s", userId.c_str());
            return false;
        }

        if(playerExists(newUserId))
        {
            logging::GALogger::w("User id is already in use: %s", userId.c_str());
            return false;
        }

        Player player = getPlayer(oldUserId);
        player.userId = newUserId;

        _players[newUserId] = std::move(player);
        
        return true;
    }

    Player& PlayerDatabase::getPlayer(std::string const& userId)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);
        
        if(!playerExits(userId))
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

    bool PlayerDatabase::playerExists(std::string const& userId)
    {
        return _players.count(userId);
    }

    json PlayerDatabase::getInitAnnotations(Player const& p)
    {
        json data;

        data["user_id"]      = id;
        data["sdk_version"]  = device::GADevice::getRelevantSdkVersion();
        data["os_version"]   = p.osVersion;
        data["device"]       = p.device;
        data["manufacturer"] = p.manufacturer;
        data["platform"]     = p.platform;
        data["session_id"]   = p._sessionId;
        data["session_num"]  = out["random_salt"] = p._sessionNum;

        utilities::addIfNotEmpty(data, "build", device::GADevice::getBuildPlatform());
        utilities::addIfNotEmpty(data, "engine_version", device::GADevice::getGameEngineVersion());

        return out;
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

        utilities::addIfNotEmpty(data, "ab_id", p._abId);
        utilities::addIfNotEmpty(data, "ab_variant_id", p._abVariantId); 
        utilities::addIfNotEmpty(data, "user_id_ext", p.extUserId);

        utilities::addIfNotEmpty(data, "custom_01", p._customDimension1);
        utilities::addIfNotEmpty(data, "custom_02", p._customDimension2);
        utilities::addIfNotEmpty(data, "custom_03", p._customDimension3);

        if(!p.customFields.IsEmpty())
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