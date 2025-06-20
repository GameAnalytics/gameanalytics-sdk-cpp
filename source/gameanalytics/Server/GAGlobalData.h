#pragma once

#include "GameAnalytics/GATypes.h"
#include "Server/GACustomFields.h"

namespace gameanalytics
{
    struct GlobalData
    {
        std::string engineVersion;
        std::string sdkVersion;
        std::string build;

        std::string severId;

        std::string extUserId;
        std::string countryCode;
        std::string device;
        std::string platform;
        std::string manufacturer;
        std::string osVersion;
        std::string customDimension1;
        std::string customDimension2;
        std::string customDimension3;
        CustomFields customFields;
    }
}