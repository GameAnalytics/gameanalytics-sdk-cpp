#pragma once

#include "GameAnalytics/GATypes.h"
#include "Server/GACustomFields.h"

namespace gameanalytics
{
    struct PlayerCallbacks
    {
        virtual void onNewPlayer(Player& player)  = 0;
        virtual void onPlayerJoin(Player& player) = 0;
        virtual void onPlayerExit(Player& player) = 0;
    };

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

        int      _sessionNum = 0;
        uint64_t _sessionLength = 0;
        uint64_t _totalSessionLength = 0;

        bool _isActive    = false;
        bool _isNewPlayer = false;

        std::chrono::time_point _lastSessionTimestamp = 0;

        void generateRandomId();

        public:

            std::string extUserId;
            std::string countryCode;
            std::string device;
            std::string platform;
            std::string manufacturer;
            std::string osVersion;
            std::string connectionType;
            CustomFields customFields;
            
            Player();
            Player(std::string const& jsonData);
            Player(std::string const& userId, std::string const& abId, std::string const& abVariantId);
            
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

            bool isNewUser() const;

            bool isActive() const;

            bool deserialize(std::string const& data);
            std::string serialize() const; 

            // called when the player joins the server
            void onJoin();

            // called when the player logs off
            void onExit();
            
            // playtime during the current session
            int64_t currentSessionPlaytime() const;

            // total session playtime
            int  totalPlaytime() const;

            // total session count
            int  getSessionCount() const;
    };

} // namespace gameanalytics
