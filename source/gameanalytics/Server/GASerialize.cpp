#include "Server/GASerialize.h"

namespace gameanalytics
{
  json serializeCustomFields(CustomFields const& customFields)
    {
        json j;

        for(auto& [key, field]: customFields.fields)
        {
            switch(field.value.index())
            {
                case CustomFields::Value::value_int:
                    j[key] = std::get<int64_t>(field.value);
                    break;

                case CustomFields::Value::value_float:
                    j[key] = std::get<double>(field.value);
                    break;

                case CustomFields::Value::value_str:
                    j[key] = std::get<std::string>(field.value);
                    break;

                case CustomFields::Value::value_bool:
                    j[key] = std::get<bool>(field.value);
                    break;
            }
        }

        return j;
    }

    CustomFields deserializeCustomFields(json const& jsonFields)
    {
        CustomFields customFields;

        for(auto& item: jsonFields.items())
        {
            CustomFields::Value f;
            f.key = item.key();

            if(item.value().is_boolean())
            {
                f.value = item.value().get<bool>();    
            }
            else if(item.value().is_number_float())
            {
                f.value = item.value().get<double>();
            }
            else if(item.value().is_number_integer())
            {
                f.value = item.value().get<int64_t>();
            }
            else if(item.value().is_string())
            {
                f.value = item.value().get<std::string>();
            }
            else // unsupported value
            {
                continue;
            }

            customFields.fields[f.key] = std::move(f);
        }

        return customFields;
    }
}