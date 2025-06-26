#pragma once

#include "GAPlayer.h"
#include "GACommon.h"

namespace gameanalytics
{
    struct PlayerNotFound : public std::runtime_error
    {
        std::string userId;
        PlayerNotFound(std::string const& userId);
    };

    class PlayerDatabase
    {
        std::unordered_map<std::string, Player> _players;
        std::recursive_mutex _mutex;

        public:

            PlayerDatabase(int sizeHint = -1);

            size_t  countPlayers() const;

            bool    playerExists(std::string const& userId) const;
            Player& getPlayer(std::string const& userId);
            bool    addPlayer(Player&& player);
            bool    removePlayer(std::string const& userId);
            bool    changePlayerId(std::string const& oldUserId, std::string const& newUserId);

            static json getPlayerAnnotations(Player const& player);
            static json getInitAnnotations(Player const& player);

            inline Player& operator[](std::string const& uid) { return getPlayer(uid); }
    };
}
