#pragma once

#include "Server/GAPlayer.h"

namespace gameanalytics
{
    class PlayerDatabase;

    class GameAnalyticsServer
    {
        std::shared_ptr<PlayerDatabase>  _playerDatabase;
        std::shared_ptr<PlayerCallbacks> _playerCallbacks;

        bool startPlayerSessionInternal(std::string const& userId);
        bool endPlayerSessionInternal(std::string const& userId);

        void addPlayerDesignEventInternal(Player& player, std::string const& eventId, double value = 0.0, std::string const& customFields = "");
        void addPlayerBusinessEventInternal(Player& player, std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, std::string const& customFields = "");
        void addPlayerResourceEventInternal(Player& player, EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, std::string const& customFields = "");
        void addPlayerErrorEventInternal(Player& player, EGAErrorSeverity severity, std::string const& message, std::string const& customFields = "");
        void addPlayerProgressionEventInternal(Player& player, EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, bool sendScore, std::string const& customFields = "");

        public:

            GameAnalyticsServer(std::string const& serverId, std::string const& serverName, std::string const& build, int numPlayersHint = -1);

            void setPlayerCallbacks(std::shared_ptr<PlayerCallbacks> callbacks);

            bool isExistingPlayer(std::string const& userId) const;
            bool addPlayer(Player&& playerData);
            bool removePlayer(std::string const& userId);
            Player& getPlayer(std::string const& userId);

            void startPlayerSession(std::string const& userId);
            void endPlayerSession(std::string const& userId);

            void addServerDesignEvent(std::string const& eventId, double value = 0.0, std::string const& customFields = "");
            void addServerBusinessEvent(std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, std::string const& customFields = "");
            void addServerResourceEvent(EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, std::string const& customFields = "");
            void addServerErrorEvent(EGAErrorSeverity severity, std::string const& message, std::string const& customFields = "");

            void addPlayerDesignEvent(Player& player, std::string const& eventId, double value = 0.0, std::string const& customFields = "");
            void addPlayerBusinessEvent(Player& player, std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, std::string const& customFields = "");
            void addPlayerResourceEvent(Player& player, EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, std::string const& customFields = "");
            void addPlayerErrorEvent(Player& player, EGAErrorSeverity severity, std::string const& message, std::string const& customFields = "");
            void addPlayerProgressionEvent(Player& player, EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, bool sendScore, std::string const& customFields = "");

            void addPlayerDesignEvent(std::string const& userId, std::string const& eventId, double value = 0.0, std::string const& customFields = "");
            void addPlayerBusinessEvent(std::string const& userId, std::string const& currency, int amount, std::string const& itemType, std::string const& itemId, std::string const& cartType, std::string const& customFields = "");
            void addPlayerResourceEvent(std::string const& userId, EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, std::string const& customFields = "");
            void addPlayerErrorEvent(std::string const& userId, EGAErrorSeverity severity, std::string const& message, std::string const& customFields = "");
            void addPlayerProgressionEvent(std::string const& userId, EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score, bool sendScore, std::string const& customFields = "");
    };

} // namespace gameanalytics
