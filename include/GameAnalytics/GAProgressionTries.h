#pragma once

#include "GameAnalytics/GATypes.h"

namespace gameanalytics
{
    struct ProgressionTries
    {
        void setTries(std::string const& s, int tries);
        void remove(std::string const& s);

        int getTries(std::string const& s) const;
        int incrementTries(std::string const& s);

        void clear();
        bool empty() const;

        std::unordered_map<std::string, int> const& getAll() const;

        private:

            std::unordered_map<std::string, int> _tries;
    };
}