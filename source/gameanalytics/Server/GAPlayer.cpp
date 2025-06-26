#include "Server/GAPlayer.h"
#include "GAUtilities.h"
#include "GALogger.h"
#include "GAValidator.h"
#include "GASerialize.h"

namespace gameanalytics
{
    Player::Player()
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

        _isActive = true;
    }

    void Player::onExit()
    {
        _sessionLength = currentSessionPlaytime();
        _totalSessionLength += _sessionLength;

        _isActive = false;
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
        return _isNewPlayer;
    }

    bool Player::isActive() const
    {
        return _isActive;
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

            utilities::addIfNotEmpty(data, "ab_id", _abId);
            utilities::addIfNotEmpty(data, "ab_variant_id", _abVariantId); 
            utilities::addIfNotEmpty(data, "user_id_ext", extUserId);

            utilities::addIfNotEmpty(data, "custom_01", _customDimension1);
            utilities::addIfNotEmpty(data, "custom_02", _customDimension2);
            utilities::addIfNotEmpty(data, "custom_03", _customDimension3);

            if(!customFields.isEmpty())
            {
                json fields = serializeCustomFields(customFields);
                data["custom_fields"] = fields;
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

            extUserId    = utilities::getOptionalValue<std::string>(data, "user_id_ext", "");
            osVersion    = utilities::getOptionalValue<std::string>(data, "os_version", "");
            device       = utilities::getOptionalValue<std::string>(data, "device", "");
            manufacturer = utilities::getOptionalValue<std::string>(data, "manufacturer", "");
            platform     = utilities::getOptionalValue<std::string>(data, "platform", "");
            countryCode  = utilities::getOptionalValue<std::string>(data, "country_code", "");

            _customDimension1 = utilities::getOptionalValue<std::string>(data, "custom_01", "");
            _customDimension2 = utilities::getOptionalValue<std::string>(data, "custom_02", "");
            _customDimension3 = utilities::getOptionalValue<std::string>(data, "custom_03", "");

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
