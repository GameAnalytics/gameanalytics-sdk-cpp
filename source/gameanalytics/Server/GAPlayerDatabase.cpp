#include "Server/GAPlayerDatabase.h"
#include "GAUtilities.h"
#include "GALogger.h"
#include "GADevice.h"
#include "GASerialize.h"
#include "GAStore.h"
#include "GAState.h"
#include "GAThreading.h"

#include <algorithm>
#include <limits>

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

        PlayerPtr entry = std::make_shared<Player>(std::move(player));
        _players[uid] = entry;
        persistPlayer(*entry);

        if(_maxPlayers > 0 && _players.size() > _maxPlayers)
        {
            trimToMaxPlayers(_maxPlayers);
        }

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

        PlayerPtr p = _players[uid];
        if(p->isActive())
        {
            logging::GALogger::w("Cannot remove player while the player is active, user id: %s", uid.c_str());
            return false;
        }

        p->_isRemoved = true;
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
            logging::GALogger::w("User id is already in use: %s", newUserId.c_str());
            return false;
        }

        PlayerPtr player = _players[uid];
        if(player->isActive())
        {
            logging::GALogger::w("Cannot change user id while the user is active, user id: %s", uid.c_str());
            return false;
        }

        player->_userId = newUserId;
        _players[newUserId] = player;
        _players.erase(uid);

        eraseFromStore(uid);
        persistPlayer(*player);
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
        std::unordered_map<std::string, bool> restoredNow;

        try
        {
            // trim the store first so evicted players are never loaded
            if(_maxIdleSeconds > 0)
            {
                const int64_t evicted = store::GAStore::evictPlayersInactiveSince(utilities::GAUtilities::timeIntervalSince1970() - _maxIdleSeconds);
                if(evicted > 0)
                {
                    logging::GALogger::i("Player database: evicted %lld stored players inactive for more than %lld seconds", (long long)evicted, (long long)_maxIdleSeconds);
                }
            }

            if(_maxPlayers > 0)
            {
                const int64_t trimmed = store::GAStore::trimPlayers(static_cast<int64_t>(_maxPlayers));
                if(trimmed > 0)
                {
                    logging::GALogger::i("Player database: trimmed %lld stored players over the limit of %zu", (long long)trimmed, _maxPlayers);
                }
            }

            json rows;
            store::GAStore::getPlayers(rows);

            if(rows.is_array())
            {
                restoredNow.reserve(rows.size());

                for(json& row : rows)
                {
                    const std::string uid  = utilities::getOptionalValue<std::string>(row, "user_id", "");
                    const std::string data = utilities::getOptionalValue<std::string>(row, "data", "");

                    if(uid.empty() || data.empty())
                    {
                        continue;
                    }

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

                    _players[uid] = std::make_shared<Player>(std::move(player));
                    restoredNow[uid] = true;
                    ++restored;
                }
            }
        }
        catch(std::exception& e)
        {
            logging::GALogger::e("Player database: failed to restore players: %s", e.what());
        }

        _storeLoaded = true;

        // write back everything registered before the load: those writes were dropped
        // while the store was unavailable, and in-memory players win over stored copies
        for(auto& [uid, player] : _players)
        {
            if(!restoredNow.count(uid))
            {
                persistPlayer(*player);
            }
        }

        logging::GALogger::i("Player database: restored %zu players, %zu registered in total", restored, _players.size());

        // players registered before the load count against the policy too
        applyRetention();

        return restored;
    }

    void PlayerDatabase::setRetention(int64_t maxIdleSeconds, size_t maxPlayers)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);
        _maxIdleSeconds = maxIdleSeconds > 0 ? maxIdleSeconds : 0;
        _maxPlayers     = maxPlayers;
    }

    size_t PlayerDatabase::evictPlayersInactiveSince(int64_t cutoffTs)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        size_t evicted = 0;
        for(auto it = _players.begin(); it != _players.end(); )
        {
            PlayerPtr const& p = it->second;
            if(!p->isActive() && p->getLastSeen() < cutoffTs)
            {
                p->_isRemoved = true;
                eraseFromStore(it->first);
                it = _players.erase(it);
                ++evicted;
            }
            else
            {
                ++it;
            }
        }

        if(evicted > 0)
        {
            logging::GALogger::i("Player database: evicted %zu players last seen before %lld", evicted, (long long)cutoffTs);
        }

        return evicted;
    }

    size_t PlayerDatabase::trimToMaxPlayers(size_t maxPlayers)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        if(maxPlayers == 0 || _players.size() <= maxPlayers)
        {
            return 0;
        }

        // candidates: inactive players, oldest first
        std::vector<std::pair<int64_t, std::string>> candidates;
        candidates.reserve(_players.size());
        for(auto const& [uid, p] : _players)
        {
            if(!p->isActive())
            {
                candidates.emplace_back(p->getLastSeen(), uid);
            }
        }
        std::sort(candidates.begin(), candidates.end());

        size_t evicted = 0;
        for(auto const& [lastSeen, uid] : candidates)
        {
            if(_players.size() <= maxPlayers)
            {
                break;
            }

            (void)lastSeen;
            _players[uid]->_isRemoved = true;
            eraseFromStore(uid);
            _players.erase(uid);
            ++evicted;
        }

        if(evicted > 0)
        {
            logging::GALogger::i("Player database: evicted %zu players to stay within %zu", evicted, maxPlayers);
        }

        if(_players.size() > maxPlayers)
        {
            logging::GALogger::w("Player database: %zu active players exceed the limit of %zu, none were evicted", _players.size(), maxPlayers);
        }

        return evicted;
    }

    size_t PlayerDatabase::applyRetention()
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        size_t evicted = 0;
        if(_maxIdleSeconds > 0)
        {
            evicted += evictPlayersInactiveSince(utilities::GAUtilities::timeIntervalSince1970() - _maxIdleSeconds);
        }
        if(_maxPlayers > 0)
        {
            evicted += trimToMaxPlayers(_maxPlayers);
        }
        return evicted;
    }

    void PlayerDatabase::applyInitResponse(Player& player, json& response)
    {
        player._configsHash = utilities::getOptionalValue<std::string>(response, "configs_hash", player._configsHash);
        player._abId        = utilities::getOptionalValue<std::string>(response, "ab_id", player._abId);
        player._abVariantId = utilities::getOptionalValue<std::string>(response, "ab_variant_id", player._abVariantId);

        if(!response.contains("configs") || !response["configs"].is_array())
        {
            // hash matched: the backend sends no configs, keep the cached ones
            player._remoteConfigsReady = true;
            return;
        }

        try
        {
            json content  = json::array();
            json tracking = json::array();

            const int64_t now = state::GAState::getClientTsAdjusted();

            for(json& cfg : response["configs"])
            {
                if(!cfg.is_object() || cfg.empty())
                {
                    continue;
                }

                const std::string key = utilities::getOptionalValue<std::string>(cfg, "key", "");
                const int64_t start_ts = utilities::getOptionalValue<int64_t>(cfg, "start_ts", std::numeric_limits<int64_t>::min());
                const int64_t end_ts   = utilities::getOptionalValue<int64_t>(cfg, "end_ts",   std::numeric_limits<int64_t>::max());

                if(key.empty() || !cfg.contains("value") || now <= start_ts || now >= end_ts)
                {
                    continue;
                }

                content.push_back({{"key", key}, {"value", cfg["value"]}});
                tracking.push_back({
                    {"key", key},
                    {"id",  utilities::getOptionalValue<std::string>(cfg, "id", "")},
                    {"vsn", utilities::getOptionalValue<int>(cfg, "vsn", 0)}
                });
            }

            player._remoteConfigsContent = content.dump();
            player._remoteConfigs        = tracking.dump();
            player._remoteConfigsReady   = true;
            player.rebuildRemoteConfigValues();

            logging::GALogger::d("Remote configs for player %s: %s", player._userId.c_str(), player._remoteConfigsContent.c_str());
        }
        catch(std::exception const& e)
        {
            logging::GALogger::e("Failed to apply remote configs for player %s: %s", player._userId.c_str(), e.what());
        }
    }

    bool PlayerDatabase::isStoreLoaded() const
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);
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

        writeToStore(uid, data, player.getLastSeen());
    }

    bool PlayerDatabase::persistPlayer(std::string const& uid)
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        if(!playerExists(uid))
        {
            logging::GALogger::w("Invalid user id: %s", uid.c_str());
            return false;
        }

        persistPlayer(*_players[uid]);
        return true;
    }

    void PlayerDatabase::persistAll()
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        for(auto& [uid, player] : _players)
        {
            persistPlayer(*player);
        }
    }

    void PlayerDatabase::writeToStore(std::string const& uid, std::string const& data, int64_t lastSeenTs)
    {
        threading::GAThreading::performTaskOnGAThread(
            [uid, data, lastSeenTs]()
            {
                // before initialize the store does not exist yet; loadFromStore
                // writes the registered players back once it does
                if(store::GAStore::getTableReady())
                {
                    store::GAStore::setPlayer(uid, data, lastSeenTs);
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

    PlayerPtr PlayerDatabase::getPlayer(std::string const& userId) const
    {
        PlayerPtr p = findPlayer(userId);
        if(!p)
        {
            logging::GALogger::e("Invalid user id: %s", userId.c_str());
            throw PlayerNotFound(userId);
        }

        return p;
    }

    PlayerPtr PlayerDatabase::findPlayer(std::string const& userId) const
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);

        auto it = _players.find(userId);
        return it != _players.end() ? it->second : nullptr;
    }

    size_t PlayerDatabase::countPlayers() const
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);
        return _players.size();
    }

    bool PlayerDatabase::playerExists(std::string const& userId) const
    {
        std::unique_lock<std::recursive_mutex> lock(_mutex);
        return _players.count(userId) > 0;
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

        utilities::addIfNotEmpty(data, "build", state::GAState::getBuild());
        utilities::addIfNotEmpty(data, "engine_version", device::GADevice::getGameEngineVersion());

        return data;
    }

    json PlayerDatabase::getPlayerAnnotations(Player const& p)
    {
        json data;

        data["v"] = 2;
        data["event_uuid"] = utilities::GAUtilities::generateUUID();
        
        utilities::addIfNotEmpty(data, "build", state::GAState::getBuild());
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