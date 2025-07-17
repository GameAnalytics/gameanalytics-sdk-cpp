#pragma once

#include "GameAnalytics/GAProgressionTries.h"

namespace gameanalytics
{
    void ProgressionTries::setTries(std::string const& s, int tries)
    {
        _tries[s] = tries;
    }

    void ProgressionTries::remove(std::string const& s)
    {
        if (_tries.count(s))
        {
            _tries.erase(s);
        }
    }

    int ProgressionTries::getTries(std::string const& s) const
    {
        return _tries.count(s) ? _tries.at(s) : 0;
    }

    int ProgressionTries::incrementTries(std::string const& s)
    {
        if (_tries.count(s))
        {
            _tries[s]++;
        }
        else
        {
            _tries[s] = 1;
        }

        return _tries[s];
    }
}