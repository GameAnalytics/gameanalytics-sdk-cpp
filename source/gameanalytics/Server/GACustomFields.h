#pragma once

#include "GameAnalytics/GATypes.h"

namespace gameanalytics
{
    struct CustomFields
    {
        struct Value
        {
            enum Type
            {
                value_int,
                value_float,
                value_str,
                value_bool
            };

            std::string key;
            std::variant<int64_t, double, std::string, bool> value;
        };

        std::unordered_map<std::string, Value> fields;

        inline bool isEmpty() const { return fields.empty(); }

        std::string toString() const;

        void setValue(std::string const& key, int64_t val);
        void setValue(std::string const& key, double val);
        void setValue(std::string const& key, std::string const& val);
        void setValue(std::string const& key, bool val);

        template<typename T>
        T getValue(std::string const& key, T const& defaultValue)
        {
            return defaultValue;
        }

        template<>
        std::string getValue<std::string>(std::string const& key, std::string const& defaultValue)
        {
            return getValueById<std::string>(key, Value::value_str, defaultValue);
        }

        template<>
        int64_t getValue<int64_t>(std::string const& key, int64_t const& defaultValue)
        {
            return getValueById<int64_t>(key, Value::value_int, defaultValue);
        }

        template<>
        double getValue<double>(std::string const& key, double const& defaultValue)
        {
            return getValueById<double>(key, Value::value_float, defaultValue);
        }

        template<>
        bool getValue<bool>(std::string const& key, bool const& defaultValue)
        {
            return getValueById<bool>(key, Value::value_bool, defaultValue);
        }

        template<>
        int getValue<int>(std::string const& key, int const& defaultValue)
        {
            return static_cast<int>(getValue<int64_t>(key, defaultValue));
        }

        template<>
        float getValue<float>(std::string const& key, float const& defaultValue)
        {
            return static_cast<float>(getValue<double>(key, defaultValue));
        }

        private:

        template<typename T>
        T getValueById(std::string const& key, int id, T const& defaultValue)
        {
            return (fields.count(key) && fields[key].value.index() == id) ? std::get<T>(fields[key].value) : defaultValue;
        }
    };
}
