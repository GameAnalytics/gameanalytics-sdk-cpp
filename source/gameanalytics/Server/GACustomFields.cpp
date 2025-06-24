#include "Server/GACustomFields.h"
#include "GACommon.h"
#include "GASerialize.h"

namespace gameanalytics
{
    void CustomFields::setValue(std::string const& key, int64_t val)
    {
        Value v;
        v.key = key;
        v.value = val;

        fields[key] = v;
    }

    void CustomFields::setValue(std::string const& key, double val)
    {
        Value v;
        v.key = key;
        v.value = val;

        fields[key] = v;
    }

    void CustomFields::setValue(std::string const& key, std::string const& val)
    {
        Value v;
        v.key = key;
        v.value = val;

        fields[key] = v;
    }

    void CustomFields::setValue(std::string const& key, bool val)
    {
        Value v;
        v.key = key;
        v.value = val;

        fields[key] = v;
    }

    std::string CustomFields::toString() const
    {
        json j = serializeCustomFields(*this);
        return j.dump();
    }

    void CustomFields::merge(CustomFields const& other)
    {
        for(auto& [key, val]: other.fields)
        {
            fields[key] = val;
        }
    }

    bool CustomFields::checkSize() const
    {
        return fields.size() <= NUM_MAX_CUSTOM_FIELDS;
    }
}