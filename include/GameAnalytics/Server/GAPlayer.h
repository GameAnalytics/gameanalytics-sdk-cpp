#pragma once

#include "GameAnalytics/GATypes.h"
#include "GameAnalytics/Server/GACustomFields.h"
#include "GameAnalytics/GAProgressionTries.h"
#include <chrono>

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
        std::string _remoteConfigs;

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
            std::string getRemoteConfigs() const;
            std::string getConfigsHash() const;

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

            // total session playtime
            int  totalPlaytime() const;

            // total session count
            int  getSessionCount() const;
    };

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
