#pragma once

#include "GameAnalytics/GATypes.h"

namespace gameanalytics
{

struct ProgressionTries
{
    inline void setTries(std::string const& s, int tries)
    {
        _tries[s] = tries;
    }

    inline void remove(std::string const& s)
    {
        if (_tries.count(s))
        {
            _tries.erase(s);
        }
    }

    inline int getTries(std::string const& s) const
    {
        return _tries.count(s) ? _tries.at(s) : 0;
    }

    inline int incrementTries(std::string const& s)
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

    private:
            
        std::unordered_map<std::string, int> _tries;
    };
}