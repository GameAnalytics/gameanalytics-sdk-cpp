#include "GameAnalytics/Server/GAPlayer.h"
#include "GAUtilities.h"
#include "GALogger.h"
#include "GAValidator.h"
#include "GASerialize.h"
#include "GAState.h"

#include <limits>

namespace gameanalytics
{
    Player::Player():
        _lastSeenTs(utilities::GAUtilities::timeIntervalSince1970())
    {
        generateRandomId();

        osVersion      = "windows 10";
        device         = "unknown";
        manufacturer   = "unknown";
        platform       = "windows";
        connectionType = "lan";
    }

    Player::Player(std::string const& userId, std::string const& externalId):
        _userId(userId),
        _lastSeenTs(utilities::GAUtilities::timeIntervalSince1970()),
        extUserId(externalId)
    {
        osVersion      = "windows 10";
        device         = "unknown";
        manufacturer   = "unknown";
        platform       = "windows";
        connectionType = "lan";
    }

    Player::Player(std::string const& jsonData)
    {
        deserialize(jsonData);
    }

    int64_t Player::currentSessionPlaytime() const
    {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - _lastSessionTimestamp).count();
    }

    int64_t Player::getLastSessionLength() const
    {
        return _sessionLength;
    }

    void Player::onJoin()
    {
        _sessionLength = 0;

        _isNewPlayer = _sessionNum == 0;
        ++_sessionNum;

        _sessionId = utilities::GAUtilities::generateUUID();

        _lastSessionTimestamp = std::chrono::steady_clock::now();
        _lastSeenTs = utilities::GAUtilities::timeIntervalSince1970();

        _isActive = true;
    }

    void Player::onExit()
    {
        _sessionLength = currentSessionPlaytime();
        _totalSessionLength += _sessionLength;
        _lastSeenTs = utilities::GAUtilities::timeIntervalSince1970();

        _isActive = false;
    }

    int64_t Player::totalPlaytime() const
    {
        return _totalSessionLength;
    }

    int64_t Player::getLastSeen() const
    {
        return _lastSeenTs;
    }

    std::string Player::getABId() const
    {
        return _abId;
    }

    std::string Player::getABVariantId() const
    {
        return _abVariantId;
    }

    bool Player::isRemoteConfigsReady() const
    {
        return _remoteConfigsReady;
    }

    int64_t Player::defaultClock()
    {
        return state::GAState::getClientTsAdjusted();
    }

    int64_t (*Player::_now)() = &Player::defaultClock;

    Player::RemoteConfig const* Player::findActiveRemoteConfig(std::string const& key) const
    {
        const int64_t now = _now();
        for(RemoteConfig const& cfg : _remoteConfigs)
        {
            if(cfg.key == key && cfg.isActiveAt(now))
            {
                return &cfg;
            }
        }
        return nullptr;
    }

    std::vector<Player::RemoteConfig const*> Player::activeRemoteConfigs() const
    {
        std::vector<RemoteConfig const*> active;
        const int64_t now = _now();
        for(RemoteConfig const& cfg : _remoteConfigs)
        {
            if(cfg.isActiveAt(now))
            {
                active.push_back(&cfg);
            }
        }
        return active;
    }

    std::string Player::getRemoteConfigs() const
    {
        try
        {
            json content = json::array();
            for(RemoteConfig const* cfg : activeRemoteConfigs())
            {
                content.push_back({{"key", cfg->key}, {"value", json::parse(cfg->valueJson)}});
            }
            return content.dump();
        }
        catch(std::exception const& e)
        {
            logging::GALogger::e("Failed to serialize remote configs for player %s: %s", _userId.c_str(), e.what());
            return "[]";
        }
    }

    std::string Player::getRemoteConfigsValueAsString(std::string const& key, std::string const& defaultValue) const
    {
        RemoteConfig const* cfg = findActiveRemoteConfig(key);
        return cfg ? cfg->value : defaultValue;
    }

    std::string Player::getRemoteConfigsValueAsJson(std::string const& key) const
    {
        const std::string value = getRemoteConfigsValueAsString(key);
        return json::accept(value) ? value : "";
    }

    int64_t Player::getRemoteConfigsValueAsInt(std::string const& key, int64_t defaultValue) const
    {
        try
        {
            const std::string s = getRemoteConfigsValueAsString(key);
            if(!s.empty())
            {
                return std::stoll(s);
            }
        }
        catch(std::exception const&)
        {
        }

        return defaultValue;
    }

    uint64_t Player::getRemoteConfigsValueAsUInt(std::string const& key, uint64_t defaultValue) const
    {
        try
        {
            const std::string s = getRemoteConfigsValueAsString(key);
            if(!s.empty())
            {
                return std::stoull(s);
            }
        }
        catch(std::exception const&)
        {
        }

        return defaultValue;
    }

    bool Player::getRemoteConfigsValueAsBool(std::string const& key, bool defaultValue) const
    {
        const std::string s = utilities::toLowerCase(getRemoteConfigsValueAsString(key));
        if(s == "true"  || s == "1") return true;
        if(s == "false" || s == "0") return false;
        return defaultValue;
    }

    double Player::getRemoteConfigsValueAsFloat(std::string const& key, double defaultValue) const
    {
        try
        {
            const std::string s = getRemoteConfigsValueAsString(key);
            if(!s.empty())
            {
                return std::stod(s);
            }
        }
        catch(std::exception const&)
        {
        }

        return defaultValue;
    }

    int Player::getSessionCount() const
    {
        return _sessionNum;
    }

    void Player::generateRandomId()
    {
        _userId = utilities::GAUtilities::generateUUID();
    }

    bool Player::isNewUser() const
    {
        // true until the first session has ended
        return _sessionNum == 0 || (_isActive && _isNewPlayer);
    }

    bool Player::isActive() const
    {
        return _isActive;
    }

    bool Player::isRemoved() const
    {
        return _isRemoved;
    }

    std::string Player::getUserId() const
    {
        return _userId;
    }

    std::string Player::getConfigsHash() const
    {
        return _configsHash;
    }

    std::string Player::getCustomDimension1() const
    {
        return _customDimension1;
    }

    std::string Player::getCustomDimension2() const
    {
        return _customDimension2;
    }

    std::string Player::getCustomDimension3() const
    {
        return _customDimension3;
    }

    int Player::getNextTransactionNum()
    {
        int num = _transactionNum;
        ++_transactionNum;

        return num;
    }

    bool Player::setCustomDimension1(std::string const& dimension)
    {
        if(validators::GAValidator::validateDimension01(dimension))
        {
            _customDimension1 = dimension;
            return true;
        }

        return false;
    }

    bool Player::setCustomDimension2(std::string const& dimension)
    {
        if(validators::GAValidator::validateDimension02(dimension))
        {
            _customDimension2 = dimension;
            return true;
        }

        return false;
    }

    bool Player::setCustomDimension3(std::string const& dimension)
    {
        if(validators::GAValidator::validateDimension03(dimension))
        {
            _customDimension3 = dimension;
            return true;
        }

        return false;
    }

    std::string Player::serialize() const
    {
        try
        {
            json data;
            
            data["user_id"]                 = _userId;
            data["session_num"]             = _sessionNum;
            data["current_session_length"]  = _sessionLength;
            data["lifetime_session_length"] = _totalSessionLength;
            data["os_version"]              = osVersion;
            data["device"]                  = device;
            data["manufacturer"]            = manufacturer;
            data["platform"]                = platform;
            data["connection_type"]         = connectionType;
            data["country_code"]            = countryCode;
            data["transaction_num"]         = _transactionNum;
            data["last_seen_ts"]            = _lastSeenTs;
            data["remote_configs_ready"]    = _remoteConfigsReady;

            utilities::addIfNotEmpty(data, "ab_id", _abId);
            utilities::addIfNotEmpty(data, "ab_variant_id", _abVariantId); 
            utilities::addIfNotEmpty(data, "configs_hash", _configsHash);
            utilities::addIfNotEmpty(data, "user_id_ext", extUserId);
            utilities::addIfNotEmpty(data, "gpu_model", gpuModel);
            utilities::addIfNotEmpty(data, "cpu_model", cpuModel);
            if(!_remoteConfigs.empty())
            {
                json configs = json::array();
                for(RemoteConfig const& cfg : _remoteConfigs)
                {
                    json entry;
                    entry["key"]   = cfg.key;
                    entry["value"] = json::parse(cfg.valueJson);
                    entry["id"]    = cfg.id;
                    entry["vsn"]   = cfg.vsn;
                    if(cfg.startTs != std::numeric_limits<int64_t>::min()) entry["start_ts"] = cfg.startTs;
                    if(cfg.endTs   != std::numeric_limits<int64_t>::max()) entry["end_ts"]   = cfg.endTs;
                    configs.push_back(std::move(entry));
                }
                data["remote_configs"] = configs;
            }

            utilities::addIfNotEmpty(data, "custom_01", _customDimension1);
            utilities::addIfNotEmpty(data, "custom_02", _customDimension2);
            utilities::addIfNotEmpty(data, "custom_03", _customDimension3);

            if(!customFields.isEmpty())
            {
                json fields = serializeCustomFields(customFields);
                data["custom_fields"] = fields;
            }

            if(!progressionTries.empty())
            {
                json tries = json::object();
                for(auto const& [progression, count] : progressionTries.getAll())
                {
                    tries[progression] = count;
                }
                data["progression"] = tries;
            }

            return data.dump();

        }
        catch(const std::exception& e)
        {
            logging::GALogger::e("Failed to serialize player: %s", e.what());
        }
        
        return "";
    }

    bool Player::deserialize(std::string const& jsonData)
    {
        try
        {
            json data = json::parse(jsonData);

            // mandatory values
            _userId             = data["user_id"].get<std::string>();
            _sessionNum         = data["session_num"].get<int>();
            _sessionLength      = data["current_session_length"].get<int64_t>();
            _totalSessionLength = data["lifetime_session_length"].get<int64_t>();
            
            // optional values
            _abId           = utilities::getOptionalValue<std::string>(data, "ab_id", "");
            _abVariantId    = utilities::getOptionalValue<std::string>(data, "ab_variant_id", "");
            _configsHash    = utilities::getOptionalValue<std::string>(data, "configs_hash", "");
            _transactionNum = utilities::getOptionalValue<int>(data, "transaction_num", 0);
            _lastSeenTs     = utilities::getOptionalValue<int64_t>(data, "last_seen_ts", 0);

            _remoteConfigsReady = utilities::getOptionalValue<bool>(data, "remote_configs_ready", false);
            _remoteConfigs.clear();
            if(data.contains("remote_configs") && data["remote_configs"].is_array())
            {
                for(json& entry : data["remote_configs"])
                {
                    RemoteConfig cfg;
                    cfg.key = utilities::getOptionalValue<std::string>(entry, "key", "");
                    if(cfg.key.empty() || !entry.contains("value"))
                    {
                        continue;
                    }

                    const json& value = entry["value"];
                    cfg.value     = value.is_string() ? value.get<std::string>() : value.dump();
                    cfg.valueJson = value.dump();
                    cfg.id        = utilities::getOptionalValue<std::string>(entry, "id", "");
                    cfg.vsn       = utilities::getOptionalValue<int>(entry, "vsn", 0);
                    cfg.startTs   = utilities::getOptionalValue<int64_t>(entry, "start_ts", std::numeric_limits<int64_t>::min());
                    cfg.endTs     = utilities::getOptionalValue<int64_t>(entry, "end_ts",   std::numeric_limits<int64_t>::max());
                    _remoteConfigs.push_back(std::move(cfg));
                }
            }

            extUserId      = utilities::getOptionalValue<std::string>(data, "user_id_ext", "");
            osVersion      = utilities::getOptionalValue<std::string>(data, "os_version", "");
            device         = utilities::getOptionalValue<std::string>(data, "device", "");
            manufacturer   = utilities::getOptionalValue<std::string>(data, "manufacturer", "");
            platform       = utilities::getOptionalValue<std::string>(data, "platform", "");
            connectionType = utilities::getOptionalValue<std::string>(data, "connection_type", "");
            countryCode    = utilities::getOptionalValue<std::string>(data, "country_code", "");
            gpuModel       = utilities::getOptionalValue<std::string>(data, "gpu_model", "");
            cpuModel       = utilities::getOptionalValue<std::string>(data, "cpu_model", "");

            progressionTries.clear();
            if(data.contains("progression") && data["progression"].is_object())
            {
                for(auto& item : data["progression"].items())
                {
                    if(item.value().is_number_integer())
                    {
                        progressionTries.setTries(item.key(), item.value().get<int>());
                    }
                }
            }

            // a restored player never has a running session
            _isActive    = false;
            _isNewPlayer = false;

            _customDimension1 = utilities::getOptionalValue<std::string>(data, "custom_01", "");
            _customDimension2 = utilities::getOptionalValue<std::string>(data, "custom_02", "");
            _customDimension3 = utilities::getOptionalValue<std::string>(data, "custom_03", "");

            customFields = CustomFields{};
            if(data.contains("custom_fields"))
            {
                customFields = deserializeCustomFields(data["custom_fields"]);
            }
        }
        catch(const std::exception& e)
        {
            logging::GALogger::e("Failed to deserialize player: %s", e.what());
            return false;
        }
        
        return true;
    }
}
