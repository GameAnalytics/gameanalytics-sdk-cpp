#pragma once

#include "GACommon.h"
#include "GameAnalytics/Server/GACustomFields.h"

namespace gameanalytics
{
    json serializeCustomFields(CustomFields const& customFields);
    CustomFields deserializeCustomFields(json const& jsonFields);
}