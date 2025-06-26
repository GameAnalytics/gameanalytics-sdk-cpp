#pragma once

#include "GameAnalytics/GATypes.h"
#include <variant>

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

        static constexpr int NUM_MAX_CUSTOM_FIELDS = 50;

        std::unordered_map<std::string, Value> fields;

        inline bool isEmpty() const { return fields.empty(); }

        inline size_t numFields() const { return fields.size(); }

        std::string toString() const;

        bool setInt(std::string const& key, int64_t val);
        bool setFloat(std::string const& key, double val);
        bool setString(std::string const& key, std::string const& val);
        bool setBool(std::string const& key, bool val);

        bool merge(CustomFields const& other);

        template<typename T>
        bool setValue(std::string const& key, T const& val);

        template<typename T>
        T getValue(std::string const& key, T const& defaultValue)
        {
            return getValueById<int64_t>(key, Value::value_int, defaultValue);
        }
        
        private:

        bool checkCustomFieldLimit() const;

        template<typename T>
        T getValueById(std::string const& key, int id, T const& defaultValue)
        {
            return (fields.count(key) && fields[key].value.index() == id) ? std::get<T>(fields[key].value) : defaultValue;
        }
    };

    #pragma region get_value_specialization

        template<typename T>
        bool CustomFields::setValue(std::string const& key, T const& val)
        {
            if(checkCustomFieldLimit())
            {
                Value v;
                v.key = key;
                v.value = val;

                fields[key] = std::move(v);

                return true;
            }

            return false;
        }

        template<>
        inline std::string CustomFields::getValue<std::string>(std::string const& key, std::string const& defaultValue)
        {
            return getValueById<std::string>(key, Value::value_str, defaultValue);
        }

        template<>
        inline double CustomFields::getValue<double>(std::string const& key, double const& defaultValue)
        {
            return getValueById<double>(key, Value::value_float, defaultValue);
        }

        template<>
        inline bool CustomFields::getValue<bool>(std::string const& key, bool const& defaultValue)
        {
            return getValueById<bool>(key, Value::value_bool, defaultValue);
        }

        template<>
        inline float CustomFields::getValue<float>(std::string const& key, float const& defaultValue)
        {
            return static_cast<float>(getValue<double>(key, defaultValue));
        }

    #pragma endregion
}
