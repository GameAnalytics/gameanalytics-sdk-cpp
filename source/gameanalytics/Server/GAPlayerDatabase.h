#pragma once

#include "GameAnalytics/Server/GAPlayer.h"
#include "GACommon.h"

namespace gameanalytics
{
    struct PlayerNotFound : public std::runtime_error
    {
        std::string userId;
        PlayerNotFound(std::string const& userId);
    };

    // In-memory registry of players, mirrored into the ga_players table of the
    // sqlite store. Every mutation made through this class is written through;
    // changes made directly on a Player reference are picked up by the next
    // persistPlayer call (the server does this on session start/end).
    //
    // All sqlite access happens on the GA thread so it never interleaves with
    // the event store's own transactions.
    class PlayerDatabase
    {
        std::unordered_map<std::string, Player> _players;
        std::recursive_mutex _mutex;
        bool _storeLoaded = false;

        static void writeToStore(std::string const& userId, std::string const& data);
        static void eraseFromStore(std::string const& userId);

        public:

            PlayerDatabase(int sizeHint = -1);

            size_t  countPlayers() const;

            bool    playerExists(std::string const& userId) const;
            Player& getPlayer(std::string const& userId);
            bool    addPlayer(Player&& player);
            bool    removePlayer(std::string const& userId);
            bool    changePlayerId(std::string const& oldUserId, std::string const& newUserId);

            // restores players saved in the sqlite store (must run on the GA thread
            // once the store is ready). Players already registered in memory win over
            // their stored copy and are written back so the store catches up.
            // Returns the number of players restored from the store.
            size_t  loadFromStore();
            bool    isStoreLoaded() const;

            // writes the current state of a player to the store
            void    persistPlayer(Player const& player);
            bool    persistPlayer(std::string const& userId);

            // writes every registered player to the store
            void    persistAll();

            static json getPlayerAnnotations(Player const& player);
            static json getInitAnnotations(Player const& player);

            inline Player& operator[](std::string const& uid) { return getPlayer(uid); }
    };
}
