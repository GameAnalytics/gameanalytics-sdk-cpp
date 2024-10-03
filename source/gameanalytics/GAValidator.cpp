//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#include "GAValidator.h"
#include "GAUtilities.h"
#include "GAEvents.h"
#include "GAState.h"
#include "GALogger.h"
#include "GAHTTPApi.h"
#include <string.h>
#include <stdio.h>

#include <map>

namespace gameanalytics
{
    namespace validators
    {
        void GAValidator::validateBusinessEvent(
            std::string const& currency,
            std::int64_t amount,
            std::string const& cartType,
            std::string const& itemType,
            std::string const& itemId,
            ValidationResult& out)
        {
            // validate currency
            if (!GAValidator::validateCurrency(currency))
            {
                logging::GALogger::w("Validation fail - business event - currency: Cannot be (null) and need to be A-Z, 3 characters and in the standard at openexchangerates.org. Failed currency: %s", currency.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::BusinessEvent;
                out.action = http::EGASdkErrorAction::InvalidCurrency;
                out.parameter = http::EGASdkErrorParameter::Currency;

                out.reason = currency;

                return;
            }

            if (amount < 0)
            {
                logging::GALogger::w("Validation fail - business event - amount. Cannot be less than 0. String: %ld", amount);

                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::BusinessEvent;
                out.action = http::EGASdkErrorAction::InvalidAmount;
                out.parameter = http::EGASdkErrorParameter::Amount;
                
                out.reason = std::to_string(amount);

                return;
            }

            // validate cartType
            if (!GAValidator::validateShortString(cartType, true))
            {
                logging::GALogger::w("Validation fail - business event - cartType. Cannot be above 32 length. String: %s", cartType.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::BusinessEvent;
                out.action = http::EGASdkErrorAction::InvalidShortString;
                out.parameter = http::EGASdkErrorParameter::CartType;
                
                out.reason = cartType;

                return;
            }

            // validate itemType length
            if (!GAValidator::validateEventPartLength(itemType, false))
            {
                logging::GALogger::w("Validation fail - business event - itemType: Cannot be (null), empty or above 64 characters. String: %s", itemType.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::BusinessEvent;
                out.action = http::EGASdkErrorAction::InvalidEventPartLength;
                out.parameter = http::EGASdkErrorParameter::ItemType;
                
                out.reason = itemType;

                return;
            }

            // validate itemType chars
            if (!GAValidator::validateEventPartCharacters(itemType))
            {
                logging::GALogger::w("Validation fail - business event - itemType: Cannot contain other characters than A-z, 0-9, -_., ()!?. String: %s", itemType.c_str());
                out.category    = http::EGASdkErrorCategory::EventValidation;
                out.area        = http::EGASdkErrorArea::BusinessEvent;
                out.action      = http::EGASdkErrorAction::InvalidEventPartCharacters;
                out.parameter   = http::EGASdkErrorParameter::ItemType;
                
                out.reason = itemType;

                return;
            }

            // validate itemId
            if (!GAValidator::validateEventPartLength(itemId, false))
            {
                logging::GALogger::w("Validation fail - business event - itemId. Cannot be (null), empty or above 64 characters. String: %s", itemId.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::BusinessEvent;
                out.action = http::EGASdkErrorAction::InvalidEventPartLength;
                out.parameter = http::EGASdkErrorParameter::ItemId;
                
                out.reason = itemType;

                return;
            }

            if (!GAValidator::validateEventPartCharacters(itemId))
            {
                logging::GALogger::w("Validation fail - business event - itemId: Cannot contain other characters than A-z, 0-9, -_., ()!?. String: %s", itemId.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::BusinessEvent;
                out.action = http::EGASdkErrorAction::InvalidEventPartCharacters;
                out.parameter = http::EGASdkErrorParameter::ItemId;

                out.reason = itemType;

                return;
            }

            out.result = true;
        }

        void GAValidator::validateResourceEvent(
            EGAResourceFlowType flowType,
            std::string const& currency,
            double amount,
            std::string const& itemType,
            std::string const& itemId,
            ValidationResult& out
            )
        {
            std::string resourceFlowTypeString = events::GAEvents::resourceFlowTypeString(flowType);

            if (resourceFlowTypeString.empty())
            {
                logging::GALogger::w("Validation fail - resource event - flowType: Invalid flow type.");
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ResourceEvent;
                out.action = http::EGASdkErrorAction::InvalidFlowType;
                out.parameter = http::EGASdkErrorParameter::FlowType;
                
                out.reason = "flow type";

                return;
            }
            if (currency.empty())
            {
                logging::GALogger::w("Validation fail - resource event - currency: Cannot be (null)");
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ResourceEvent;
                out.action = http::EGASdkErrorAction::StringEmptyOrNull;
                out.parameter = http::EGASdkErrorParameter::Currency;
                
                out.reason = "currency";

                return;
            }
            if (!state::GAState::hasAvailableResourceCurrency(currency))
            {
                logging::GALogger::w("Validation fail - resource event - currency: Not found in list of pre-defined available resource currencies. String: %s", currency.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ResourceEvent;
                out.action = http::EGASdkErrorAction::NotFoundInAvailableCurrencies;
                out.parameter = http::EGASdkErrorParameter::Currency;
                
                out.reason = currency;

                return;
            }
            if (!(amount > 0))
            {
                logging::GALogger::w("Validation fail - resource event - amount: Float amount cannot be 0 or negative. Value: %f", amount);
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ResourceEvent;
                out.action = http::EGASdkErrorAction::InvalidAmount;
                out.parameter = http::EGASdkErrorParameter::Amount;

                out.reason = std::to_string(amount);

                return;
            }
            if (itemType.empty())
            {
                logging::GALogger::w("Validation fail - resource event - itemType: Cannot be (null)");
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ResourceEvent;
                out.action = http::EGASdkErrorAction::StringEmptyOrNull;
                out.parameter = http::EGASdkErrorParameter::ItemType;
                out.reason = "item type is empty";

                return;
            }
            if (!GAValidator::validateEventPartLength(itemType, false))
            {
                logging::GALogger::w("Validation fail - resource event - itemType: Cannot be (null), empty or above 64 characters. String: %s", itemType.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ResourceEvent;
                out.action = http::EGASdkErrorAction::InvalidEventPartLength;
                out.parameter = http::EGASdkErrorParameter::ItemType;
                
                out.reason = itemType;

                return;
            }
            if (!GAValidator::validateEventPartCharacters(itemType))
            {
                logging::GALogger::w("Validation fail - resource event - itemType: Cannot contain other characters than A-z, 0-9, -_., ()!?. String: %s", itemType.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ResourceEvent;
                out.action = http::EGASdkErrorAction::InvalidEventPartCharacters;
                out.parameter = http::EGASdkErrorParameter::ItemType;
                
                out.reason = itemType;

                return;
            }
            if (!state::GAState::hasAvailableResourceItemType(itemType))
            {
                logging::GALogger::w("Validation fail - resource event - itemType: Not found in list of pre-defined available resource itemTypes. String: %s", itemType.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ResourceEvent;
                out.action = http::EGASdkErrorAction::NotFoundInAvailableItemTypes;
                out.parameter = http::EGASdkErrorParameter::ItemType;
                
                out.reason = itemType;

                return;
            }
            if (!GAValidator::validateEventPartLength(itemId, false))
            {
                logging::GALogger::w("Validation fail - resource event - itemId: Cannot be (null), empty or above 64 characters. String: %s", itemId.c_str());
                
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ResourceEvent;
                out.action = http::EGASdkErrorAction::InvalidEventPartLength;
                out.parameter = http::EGASdkErrorParameter::ItemId;
                out.reason = itemId;

                return;
            }
            if (!GAValidator::validateEventPartCharacters(itemId))
            {
                logging::GALogger::w("Validation fail - resource event - itemId: Cannot contain other characters than A-z, 0-9, -_., ()!?. String: %s", itemId.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ResourceEvent;
                out.action = http::EGASdkErrorAction::InvalidEventPartCharacters;
                out.parameter = http::EGASdkErrorParameter::ItemId;
                
                out.reason = itemId;

                return;
            }
            out.result = true;
        }

        bool checkProgressionStringsOrder(std::string const& progression01, std::string const& progression02, std::string const& progression03)
        {
            if(!progression03.empty() && (progression01.empty() || progression02.empty()))
                return false;

            if(!progression02.empty() && progression01.empty())
                return false;

            if(progression01.empty())
                return false;

            return true;
        }

        bool validateProgressionString(std::string const& progression, ValidationResult& out, int progressionLvl)
        {
            if (!GAValidator::validateEventPartLength(progression, true))
            {
                logging::GALogger::w("Validation fail - progression event - - progression0%d: Cannot be empty or above 64 characters. String: %s", progressionLvl + 1, progression.c_str());
                    out.category = http::EGASdkErrorCategory::EventValidation;
                    out.area = http::EGASdkErrorArea::ProgressionEvent;
                    out.action = http::EGASdkErrorAction::InvalidEventPartLength;
                    out.parameter = http::EGASdkErrorParameter((int)http::EGASdkErrorParameter::Progression01 + progressionLvl);
                    out.reason = progression;
                    return false;
            }

            if (!GAValidator::validateEventPartCharacters(progression))
            {
                    logging::GALogger::w("Validation fail - progression event - progression0%d: Cannot contain other characters than A-z, 0-9, -_., ()!?. String:\"%s\"", progressionLvl + 1, progression.c_str());
                    out.category = http::EGASdkErrorCategory::EventValidation;
                    out.area = http::EGASdkErrorArea::ProgressionEvent;
                    out.action = http::EGASdkErrorAction::InvalidEventPartCharacters;
                    out.parameter = http::EGASdkErrorParameter((int)http::EGASdkErrorParameter::Progression01 + progressionLvl);
                    out.reason = progression;
                    return false;
            }

            return true;
        }

        void GAValidator::validateProgressionEvent(
            EGAProgressionStatus progressionStatus,
            std::string const& progression01,
            std::string const& progression02,
            std::string const& progression03,
            ValidationResult& out
            )
        {
            std::string progressionStatusString = events::GAEvents::progressionStatusString(progressionStatus);
            if (progressionStatusString.empty())
            {
                logging::GALogger::w("Validation fail - progression event: Invalid progression status.");
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ProgressionEvent;
                out.action = http::EGASdkErrorAction::InvalidProgressionStatus;
                out.parameter = http::EGASdkErrorParameter::ProgressionStatus;
                out.reason = "Invalid progression";
                return;
            }

            // Make sure progressions are defined as either 01, 01+02 or 01+02+03
            if (!checkProgressionStringsOrder(progression01, progression02, progression03))
            {
                logging::GALogger::w("Validation fail - progression event. Progression must be set as either 01, 01+02 or 01+02+03.");
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ProgressionEvent;
                out.action = http::EGASdkErrorAction::WrongProgressionOrder;
                out.parameter = static_cast<http::EGASdkErrorParameter>(0);
                out.reason = progression01 + ':' + progression02 + ':' + progression03;
                return;
            }

			bool isProgression01Valid = validateProgressionString(progression01, out, 0);
			bool isProgression02Valid = validateProgressionString(progression02, out, 1);
			bool isProgression03Valid = validateProgressionString(progression03, out, 2);
			
			out.result = isProgression01Valid || (isProgression01Valid && isProgression02Valid) || (isProgression01Valid && isProgression02Valid && isProgression03Valid);

        }


        void GAValidator::validateDesignEvent(std::string const& eventId, ValidationResult& out)
        {
            if (!GAValidator::validateEventIdLength(eventId))
            {
                logging::GALogger::w("Validation fail - design event - eventId: Cannot be (null) or empty. Only 5 event parts allowed seperated by :. Each part need to be 64 characters or less. String: %s", eventId.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::DesignEvent;
                out.action = http::EGASdkErrorAction::InvalidEventIdLength;
                out.parameter = http::EGASdkErrorParameter::EventId;
                out.reason = eventId;
                return;
            }
            if (!GAValidator::validateEventIdCharacters(eventId))
            {
                logging::GALogger::w("Validation fail - design event - eventId: Non valid characters. Only allowed A-z, 0-9, -_., ()!?. String: %s", eventId.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::DesignEvent;
                out.action = http::EGASdkErrorAction::InvalidEventIdCharacters;
                out.parameter = http::EGASdkErrorParameter::EventId;
                out.reason = eventId;
                return;
            }
            // value: allow 0, negative and nil (not required)
            out.result = true;
        }


        void GAValidator::validateErrorEvent(EGAErrorSeverity severity, std::string const& message, ValidationResult& out)
        {
            const std::string errorSeverityString = events::GAEvents::errorSeverityString(severity);
            if (errorSeverityString.empty())
            {
                logging::GALogger::w("Validation fail - error event - severity: Severity was unsupported value.");
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ErrorEvent;
                out.action = http::EGASdkErrorAction::InvalidSeverity;
                out.parameter = http::EGASdkErrorParameter::Severity;
                out.reason =  "Invalid severity";
                return;
            }

            if (!GAValidator::validateLongString(message, true))
            {
                logging::GALogger::w("Validation fail - error event - message: Message cannot be above 8192 characters. message=%s", message.c_str());
                out.category = http::EGASdkErrorCategory::EventValidation;
                out.area = http::EGASdkErrorArea::ErrorEvent;
                out.action = http::EGASdkErrorAction::InvalidLongString;
                out.parameter = http::EGASdkErrorParameter::Message;
                out.reason = message;
                return;
            }
            out.result = true;
        }

        bool GAValidator::validateSdkErrorEvent(std::string const& gameKey, std::string const& gameSecret, http::EGASdkErrorCategory category, http::EGASdkErrorArea area, http::EGASdkErrorAction action)
        {
            if(!validateKeys(gameKey, gameSecret))
            {
                 logging::GALogger::w("validateSdkErrorEvent failed. Game key or secret key is invalid. Can only contain characters A-z 0-9, gameKey is 32 length, gameSecret is 40 length. Failed keys - gameKey: %s, secretKey: %s", gameKey.c_str(), gameSecret.c_str());
                return false;
            }

            const std::string categoryString = http::GAHTTPApi::sdkErrorCategoryString(category);
            if (categoryString.empty())
            {
                logging::GALogger::w("Validation fail - sdk error event - category: Category was unsupported value.");
                return false;
            }

            const std::string areaString = http::GAHTTPApi::sdkErrorAreaString(area);
            if (areaString.empty())
            {
                logging::GALogger::w("Validation fail - sdk error event - area: Area was unsupported value.");
                return false;
            }

            std::string const actionString = http::GAHTTPApi::sdkErrorActionString(action);
            if (actionString.empty())
            {
                logging::GALogger::w("Validation fail - sdk error event - action: Action was unsupported value.");
                return false;
            }

            return true;
        }


        // event params
        bool GAValidator::validateKeys(std::string const& gameKey, std::string const& gameSecret)
        {
            if (utilities::GAUtilities::stringMatch(gameKey, "^[A-z0-9]{32}$"))
            {
                if (utilities::GAUtilities::stringMatch(gameSecret, "^[A-z0-9]{40}$"))
                {
                    return true;
                }
            }
            return false;
        }

        bool GAValidator::validateCurrency(std::string const& currency)
        {
            if (currency.empty())
            {
                return false;
            }
            if (!utilities::GAUtilities::stringMatch(currency, "^[A-Z]{3}$"))
            {
                return false;
            }
            return true;
        }

        bool GAValidator::validateEventPartLength(std::string const& eventPart, bool allowNull)
        {
            constexpr uint32_t MAX_SIZE = 64u;

            size_t size = eventPart.length();
            if (allowNull == true && size == 0)
            {
                return true;
            }

            if (size == 0)
            {
                return false;
            }

            if (size > MAX_SIZE)
            {
                return false;
            }

            return true;
        }

        bool GAValidator::validateEventPartCharacters(std::string const& eventPart)
        {
            if (!utilities::GAUtilities::stringMatch(eventPart, "^[A-Za-z0-9\\s\\-_\\.\\(\\)\\!\\?]{1,64}$"))
            {
                return false;
            }
            return true;
        }

        bool GAValidator::validateEventIdLength(std::string const& eventId)
        {
            if (eventId.empty())
            {
                return false;
            }

            if (!utilities::GAUtilities::stringMatch(eventId, "^[^:]{1,64}(?::[^:]{1,64}){0,4}$"))
            {
                return false;
            }
            return true;
        }

        bool GAValidator::validateEventIdCharacters(std::string const& eventId)
        {
            if (eventId.empty())
            {
                return false;
            }

            if (!utilities::GAUtilities::stringMatch(eventId, "^[A-Za-z0-9\\s\\-_\\.\\(\\)\\!\\?]{1,64}(:[A-Za-z0-9\\s\\-_\\.\\(\\)\\!\\?]{1,64}){0,4}$"))
            {
                return false;
            }
            return true;
        }

        bool validateStringLength(std::string const& str, const std::size_t maxSize, bool canBeEmpty = false)
        {
            if(str.empty())
            {
               return canBeEmpty; 
            }

            return maxSize >= str.length();
        }

        bool GAValidator::validateShortString(std::string const& shortString, bool canBeEmpty = false)
        {
            constexpr uint32_t MAX_SIZE = 32u;
            return validateStringLength(shortString, MAX_SIZE, canBeEmpty);
        }

        bool GAValidator::validateString(std::string const& string, bool canBeEmpty = false)
        {
            constexpr uint32_t MAX_SIZE = 64u;
            return validateStringLength(string, MAX_SIZE, canBeEmpty);
        }

        bool GAValidator::validateLongString(std::string const& longString, bool canBeEmpty = false)
        {
            constexpr uint32_t MAX_SIZE = 8192;
            return validateStringLength(longString, MAX_SIZE, canBeEmpty);
        }

        // validate wrapper version, build, engine version, store
        bool GAValidator::validateSdkWrapperVersion(std::string const& wrapperVersion)
        {
            // todo add beta/alpha version validation
            if (!utilities::GAUtilities::stringMatch(wrapperVersion, "^(unity|unreal|corona|cocos2d|lumberyard|air|gamemaker|defold|godot) [0-9]{0,5}(\\.[0-9]{0,5}){0,2}(-alpha|-beta|a|b){0,1}$"))
            {
                return false;
            }
            return true;
        }

        bool GAValidator::validateBuild(std::string const& build)
        {
            if (!GAValidator::validateShortString(build))
            {
                return false;
            }
            return true;
        }

        bool GAValidator::validateEngineVersion(std::string const& engineVersion)
        {
            if (!utilities::GAUtilities::stringMatch(engineVersion, "^(unity|unreal|corona|cocos2d|lumberyard|gamemaker|defold|godot) [0-9]{0,5}(\\.[0-9]{0,5}){0,2}$"))
            {
                return false;
            }
            return true;
        }

        bool GAValidator::validateStore(std::string const& store)
        {
            return utilities::GAUtilities::stringMatch(store, "^(apple|google_play)$");
        }

        bool GAValidator::validateConnectionType(std::string const& connectionType)
        {
            return utilities::GAUtilities::stringMatch(connectionType, "^(wwan|wifi|lan|offline)$");
        }

        // dimensions
        bool GAValidator::validateCustomDimensions(const StringVector& customDimensions)
        {
            constexpr uint32_t MAX_NUM_DIMENSIONS = 20;
            constexpr uint32_t MAX_DIMENSION_LEN  = 32;

            return GAValidator::validateArrayOfStrings(customDimensions, MAX_NUM_DIMENSIONS, MAX_DIMENSION_LEN, false, "custom dimensions");
        }

        bool GAValidator::validateResourceCurrencies(const StringVector& resourceCurrencies)
        {
            constexpr uint32_t MAX_NUM_CURRENCIES   = 20;
            constexpr uint32_t MAX_CURRENCY_LEN     = 64;

            if (!GAValidator::validateArrayOfStrings(resourceCurrencies, MAX_NUM_CURRENCIES, MAX_CURRENCY_LEN, false, "resource currencies"))
            {
                return false;
            }

            // validate each string for regex
            for (std::string const& resourceCurrency : resourceCurrencies)
            {
                if (!utilities::GAUtilities::stringMatch(resourceCurrency, "^[A-Za-z]+$"))
                {
                    logging::GALogger::w("resource currencies validation failed: a resource currency can only be A-Z, a-z. String was: %s", resourceCurrency.c_str());
                    return false;
                }
            }
            return true;
        }

        bool GAValidator::validateResourceItemTypes(const StringVector& resourceItemTypes)
        {
            constexpr uint32_t MAX_RESOURCE_LEN  = 32;
            constexpr uint32_t MAX_NUM_RESOURCES = 20;

            if (!GAValidator::validateArrayOfStrings(resourceItemTypes, MAX_NUM_RESOURCES, MAX_RESOURCE_LEN, false, "resource item types"))
            {
                return false;
            }

            // validate each resourceItemType for eventpart validation
            for (auto& resourceItemType : resourceItemTypes)
            {
                if (!GAValidator::validateEventPartCharacters(resourceItemType))
                {
                    logging::GALogger::w("resource item types validation failed: a resource item type cannot contain other characters than A-z, 0-9, -_., ()!?. String was: %s", resourceItemType.c_str());
                    return false;
                }
            }
            return true;
        }


        bool GAValidator::validateDimension01(std::string const& dimension01)
        {
            // allow nil
            if (dimension01.empty())
            {
                return true;
            }
            if (!state::GAState::hasAvailableCustomDimensions01(dimension01))
            {
                return false;
            }
            return true;
        }

        bool GAValidator::validateDimension02(std::string const& dimension02)
        {
            // allow nil
            if (dimension02.empty())
            {
                return true;
            }
            if (!state::GAState::hasAvailableCustomDimensions02(dimension02))
            {
                return false;
            }
            return true;
        }

        bool GAValidator::validateDimension03(std::string const& dimension03)
        {
            // allow nil
            if (dimension03.empty())
            {
                return true;
            }
            if (!state::GAState::hasAvailableCustomDimensions03(dimension03))
            {
                return false;
            }
            return true;
        }

        bool GAValidator::validateArrayOfStrings(
            const StringVector& arrayOfStrings,
            size_t maxCount,
            size_t maxStringLength,
            bool allowNoValues,
            std::string const& logTag
            )
        {
            if(arrayOfStrings.empty())
            {
                return allowNoValues;
            }

            if(arrayOfStrings.size() > maxCount)
            {
                logging::GALogger::w("[%s] Failed array validation, max capacity is %d but array size is %d", logTag.c_str(), maxCount, arrayOfStrings.size());
                return false;
            }

            for(std::string const& str : arrayOfStrings)
            {
                if(str.empty())
                {
					logging::GALogger::w("[%s] Failed array validation, empty value inside the array", logTag.c_str());
					return false;
                }

                if(str.length() > maxStringLength)
                {
                    logging::GALogger::w("[%s] Failed array validation, string %s exceeds max string size %d, size was %d", logTag.c_str(), str.c_str(), maxCount, str.size());
                    return false;
                }
            }

            return true;
        }

        bool GAValidator::validateClientTs(int64_t clientTs)
        {
            constexpr int64_t k_maxTs = 99999999999;
            if (clientTs < 0 || clientTs > k_maxTs)
            {
                return false;
            }
            return true;
        }

        bool GAValidator::validateUserId(std::string const& uId)
        {
            if (uId.empty())
            {
                logging::GALogger::w("Validation fail - user id cannot be empty.");
                return false;
            }
            return true;
        }

        void GAValidator::validateAndCleanInitRequestResponse(const json& initResponse, json& out, bool configsCreated)
        {
            // make sure we have a valid dict
            if (initResponse.is_null())
            {
                logging::GALogger::w("validateInitRequestResponse failed - no response dictionary.");
                return;
            }

            // validate server_ts
            if (initResponse.contains("server_ts") && initResponse["server_ts"].is_number_integer())
            {
                int64_t serverTsNumber = initResponse["server_ts"].get<int64_t>();
                if (serverTsNumber > 0)
                {
                    out["server_ts"] = serverTsNumber;
                }
            }

            if(configsCreated)
            {
                if (initResponse.contains("configs") && initResponse["configs"].is_array())
                {
                    utilities::copyValueIfExistent(out, initResponse, "configs");
                }
                if (initResponse.contains("configs_hash") && initResponse["configs_hash"].is_string())
                {
                    utilities::copyValueIfExistent(out, initResponse, "configs_hash");
                }
                if (initResponse.contains("ab_id") && initResponse["ab_id"].is_string())
                {
                    utilities::copyValueIfExistent(out, initResponse, "ab_id");
                }
                if (initResponse.contains("ab_variant_id") && initResponse["ab_variant_id"].is_string())
                {
                    utilities::copyValueIfExistent(out, initResponse, "ab_variant_id");
                }
            }
        }
    }
}
