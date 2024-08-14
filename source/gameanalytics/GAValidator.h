//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include "GameAnalytics.h"
#include "GAHTTPApi.h"

namespace gameanalytics
{
    namespace validators
    {
        struct ValidationResult
        {
            http::EGASdkErrorCategory category;
            http::EGASdkErrorArea area;
            http::EGASdkErrorAction action;
            http::EGASdkErrorParameter parameter;
            std::string reason;
            bool result = false;
        };

        class GAValidator
        {
         public:
            // ---------- EVENTS -------------- //

            // user created events
            static void validateBusinessEvent(
                std::string const& currency,
                std::int64_t amount,
                std::string const& cartType,
                std::string const& itemType,
                std::string const& itemId,
                ValidationResult& out
                );

            static void validateResourceEvent(
                EGAResourceFlowType flowType,
                std::string const& currency,
                double amount,
                std::string const& itemType,
                std::string const& itemId,
                ValidationResult& out
                );


            static void validateProgressionEvent(
                EGAProgressionStatus progressionStatus,
                std::string const& progression01,
                std::string const& progression02,
                std::string const& progression03,
                ValidationResult& out
                );

            static void validateDesignEvent(
                std::string const& eventId,
                ValidationResult& out
                );

            static void validateErrorEvent(
                EGAErrorSeverity severity,
                std::string const& message,
                ValidationResult& out
                );

            static bool validateSdkErrorEvent(
                std::string const& gameKey, 
                std::string const& gameSecret, 
                http::EGASdkErrorCategory category, 
                http::EGASdkErrorArea area, 
                http::EGASdkErrorAction action);


            // -------------------- HELPERS --------------------- //

            // event params
            static bool validateKeys(std::string const& gameKey, std::string const& gameSecret);
            static bool validateCurrency(std::string const& currency);
            static bool validateEventPartLength(std::string const& eventPart, bool allowNull);
            static bool validateEventPartCharacters(std::string const& eventPart);
            static bool validateEventIdLength(std::string const& eventId);
            static bool validateEventIdCharacters(std::string const& eventId);
            static bool validateShortString(std::string const& shortString, bool canBeEmpty);
            static bool validateString(std::string const& string, bool canBeEmpty);
            static bool validateLongString(std::string const& longString, bool canBeEmpty);

            // validate wrapper version, build, engine version, store
            static bool validateSdkWrapperVersion(std::string const& wrapperVersion);
            static bool validateBuild(std::string const& build);
            static bool validateEngineVersion(std::string const& engineVersion);
            static bool validateStore(std::string const& store);
            static bool validateConnectionType(std::string const& connectionType);

            // dimensions
            static bool validateCustomDimensions(const StringVector& customDimensions);

            // resource
            static bool validateResourceCurrencies(const StringVector& resourceCurrencies);
            static bool validateResourceItemTypes(const StringVector& resourceItemTypes);

            static bool validateDimension01(std::string const& dimension01);
            static bool validateDimension02(std::string const& dimension02);
            static bool validateDimension03(std::string const& dimension03);

            static void validateAndCleanInitRequestResponse(const json& initResponse, json& out, bool configsCreated);

            // array of strings
            static bool validateArrayOfStrings(
                const StringVector& arrayOfStrings,
                size_t maxCount,
                size_t maxStringLength,
                bool allowNoValues,
                std::string const& arrayTag
                );
            static bool validateClientTs(int64_t clientTs);

            static bool validateUserId(std::string const& uId);
        };
    }
}
