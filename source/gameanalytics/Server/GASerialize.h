#pragma once

#include "GACommon.h"
#include "Server/GACustomFields.h"

namespace gameanalytics
{
    json serializeCustomFields(CustomFields const& customFields);
    CustomFields deserializeCustomFields(json const& jsonFields);
}