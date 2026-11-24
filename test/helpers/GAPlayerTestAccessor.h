#pragma once

#include "GameAnalytics/Server/GAPlayer.h"

namespace gameanalytics
{
    // friend of Player (see GAPlayer.h): lets tests move the clock the remote
    // config windows are evaluated against
    struct PlayerTestAccessor
    {
        static void setClock(int64_t (*clock)())
        {
            Player::_now = clock;
        }

        static void resetClock()
        {
            Player::_now = &Player::defaultClock;
        }
    };
}
