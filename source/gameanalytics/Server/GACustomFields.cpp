#include "Server/GACustomFields.h"
#include "GACommon.h"
#include "GASerialize.h"
#include "GALogger.h"

namespace gameanalytics
{
    bool CustomFields::checkCustomFieldLimit() const
    {
        int num = numFields() + 1;
        if(num > MAX_CUSTOM_FIELDS_COUNT)
        {
            logging::GALogger::w("Too many custom fields. Maximum number of allowed custom fields is: %i. New size is: %i", MAX_CUSTOM_FIELDS_COUNT, num);
            return false;
        }

        return true;
    }

    bool CustomFields::setValue(std::string const& key, int64_t val)
    {
        return setField(key, val);
    }

    bool CustomFields::setValue(std::string const& key, double val)
    {
        return setField(key, val);
    }

    bool CustomFields::setValue(std::string const& key, std::string const& val)
    {
        return setField(key, val);
    }

    bool CustomFields::setValue(std::string const& key, bool val)
    {
        return setField(key, val);
    }

    std::string CustomFields::toString() const
    {
        json j = serializeCustomFields(*this);
        return j.dump();
    }

    bool CustomFields::merge(CustomFields const& other)
    {
        if(other.isEmpty())
            return true;

        const int num = other.numFields() + numFields();
        if(num > MAX_CUSTOM_FIELDS_COUNT)
        {
            logging::GALogger::w("Too many custom fields. Maximum number of allowed custom fields is: %i. New size is: %i", MAX_CUSTOM_FIELDS_COUNT, num);
            return false;
        }

        for(auto& [key, val]: other.fields)
        {
            fields[key] = val;
        }

        return true;
    }
}