#pragma once

#include "GameAnalytics/GATypes.h"
#include "GameAnalytics/Server/GACustomFields.h"
#include "GameAnalytics/GAProgressionTries.h"
#include <chrono>
#include <cstdint>
#include <limits>
#include <vector>

namespace gameanalytics
{
    class Player
    {
        friend class GameAnalyticsServer;
        friend class PlayerDatabase;

        static constexpr int NUM_CUSTOM_DIM = 3;

        std::string _userId;
        std::string _abId;
        std::string _abVariantId;
        std::string _configsHash;

        // remote configs delivered by the init call for this player. Every config is
        // kept with its time window and filtered when read, so a config that expires
        // or starts between two init calls switches over at the right moment.
        struct RemoteConfig
        {
            std::string key;
            std::string value;      // string form, used by the typed getters
            std::string valueJson;  // original json value, used by getRemoteConfigs
            std::string id;
            int         vsn     = 0;
            int64_t     startTs = std::numeric_limits<int64_t>::min();
            int64_t     endTs   = std::numeric_limits<int64_t>::max();

            bool isActiveAt(int64_t ts) const { return ts > startTs && ts < endTs; }
        };

        std::vector<RemoteConfig> _remoteConfigs;
        bool _remoteConfigsReady = false;

        // the first config with this key that is active right now, or nullptr
        RemoteConfig const* findActiveRemoteConfig(std::string const& key) const;
        std::vector<RemoteConfig const*> activeRemoteConfigs() const;

        // clock the config windows are evaluated against (adjusted client time, unix seconds)
        static int64_t defaultClock();
        static int64_t (*_now)();
        friend struct PlayerTestAccessor;

        std::string _customDimension1;
        std::string _customDimension2;
        std::string _customDimension3;

        std::string _sessionId;

        int     _sessionNum = 0;
        int64_t _sessionLength = 0;
        int64_t _totalSessionLength = 0;

        int _transactionNum = 0;

        bool _isActive    = false;
        bool _isNewPlayer = false;
        bool _isRemoved   = false;

        // unix time (seconds) of the last session start or end, used for retention
        int64_t _lastSeenTs = 0;

        std::chrono::steady_clock::time_point _lastSessionTimestamp;

        void generateRandomId();

        public:

            std::string extUserId;
            std::string countryCode;
            std::string device;
            std::string platform;
            std::string manufacturer;
            std::string osVersion;
            std::string connectionType;
            std::string gpuModel;
            std::string cpuModel;
            CustomFields customFields;

            ProgressionTries progressionTries;
            
            Player();
            Player(std::string const& jsonData);
            Player(std::string const& userId, std::string const& extUserId);
            
            std::string getUserId() const;
            std::string getABId() const;
            std::string getABVariantId() const;
            std::string getConfigsHash() const;

            // remote configs (populated by the init call made on session start, cached across restarts)
            bool        isRemoteConfigsReady() const;

            // json array of {"key", "value"} objects for the configs active right now,
            // same format as GameAnalytics::getRemoteConfigsContentAsString
            std::string getRemoteConfigs() const;

            std::string getRemoteConfigsValueAsString(std::string const& key, std::string const& defaultValue = "") const;

            // the value when it is valid json (object, array, number, ...), "" otherwise
            std::string getRemoteConfigsValueAsJson(std::string const& key) const;
            int64_t     getRemoteConfigsValueAsInt(std::string const& key, int64_t defaultValue = 0) const;
            uint64_t    getRemoteConfigsValueAsUInt(std::string const& key, uint64_t defaultValue = 0) const;
            bool        getRemoteConfigsValueAsBool(std::string const& key, bool defaultValue = false) const;
            double      getRemoteConfigsValueAsFloat(std::string const& key, double defaultValue = 0.0) const;

            // unix time (seconds) the player last started or ended a session
            int64_t     getLastSeen() const;

            bool setCustomDimension1(std::string const& dimension);
            bool setCustomDimension2(std::string const& dimension);
            bool setCustomDimension3(std::string const& dimension);

            std::string getCustomDimension1() const;
            std::string getCustomDimension2() const;
            std::string getCustomDimension3() const;

            // returns the last value and increments the transaction count for this player
            int getNextTransactionNum();

            // returns true if this is the 1st player session
            bool isNewUser() const;

            // returns true if the user has an active session 
            bool isActive() const;

            // true once the player was removed or evicted from the server; the object
            // stays valid for anyone still holding it, but events for it are dropped
            bool isRemoved() const;

            // deserializes a player from the given json
            bool deserialize(std::string const& jsonData);
            
            // serializes a player to a json string
            std::string serialize() const; 

            // called when the player joins the server
            void onJoin();

            // called when the player logs off
            void onExit();
            
            // get the playtime during the current session
            int64_t currentSessionPlaytime() const;

            // return playtime of the last session, valid only after onExit has been called
            int64_t getLastSessionLength() const;

            // total session playtime in seconds, excluding the running session
            int64_t totalPlaytime() const;

            // total session count
            int  getSessionCount() const;
    };

    using PlayerPtr = std::shared_ptr<Player>;

    struct PlayerCallbacks
    {
        // called when a new player joins the first session
        virtual void onNewPlayer(Player& player)  = 0;
        
        // called when a player starts a session (e.g: logs in)
        virtual void onPlayerJoin(Player& player) = 0;
        
        // called when a player ends a session (e.g: logs off)
        virtual void onPlayerExit(Player& player) = 0;
    };

} // namespace gameanalytics
