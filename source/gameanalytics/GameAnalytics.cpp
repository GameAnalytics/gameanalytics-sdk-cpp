//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#include "GameAnalytics.h"

#include "GAThreading.h"
#include "GALogger.h"
#include "GAState.h"
#include "GADevice.h"
#include "GAHTTPApi.h"
#include "GAValidator.h"
#include "GAEvents.h"
#include "GAUtilities.h"
#include "GAStore.h"
#include "GAUncaughtExceptionHandler.h"
#include <cstdlib>
#include <thread>
#include <array>

namespace gameanalytics
{
    bool GameAnalytics::_endThread = false;

    constexpr std::size_t maxFieldsSize     = 4096u;
    constexpr std::size_t maxErrMsgSize     = 8182u;
    constexpr std::size_t maxDimensionSize  = 64u;

    template<std::size_t size>
    std::string TrimString(std::string const& _str)
    {
        std::string str = _str;

        if (str.size() > size)
            str = _str.substr(0, size);

        return str;
    }

    // ----------------------- CONFIGURE ---------------------- //

    void GameAnalytics::configureAvailableCustomDimensions01(const StringVector& customDimensions)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([customDimensions]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("Available custom dimensions must be set before SDK is initialized");
                return;
            }
            state::GAState::setAvailableCustomDimensions01(customDimensions);
        });
    }

    void GameAnalytics::configureAvailableCustomDimensions02(const StringVector& customDimensions)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([customDimensions]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("Available custom dimensions must be set before SDK is initialized");
                return;
            }
            state::GAState::setAvailableCustomDimensions02(customDimensions);
        });
    }

    void GameAnalytics::configureAvailableCustomDimensions03(const StringVector& customDimensions)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([customDimensions]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("Available custom dimensions must be set before SDK is initialized");
                return;
            }
            state::GAState::setAvailableCustomDimensions03(customDimensions);
        });
    }

    void GameAnalytics::configureAvailableResourceCurrencies(const StringVector& resourceCurrencies)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([resourceCurrencies]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("Available resource currencies must be set before SDK is initialized");
                return;
            }
            state::GAState::setAvailableResourceCurrencies(resourceCurrencies);
        });
    }

    void GameAnalytics::configureAvailableResourceCurrencies(std::string const& resourceCurrenciesJson)
    {
        rapidjson::Document json;
        json.Parse(resourceCurrenciesJson.c_str());
        StringVector list;

        for (rapidjson::Value::ConstValueIterator itr = json.Begin(); itr != json.End(); ++itr)
        {
            list.push_back((*itr).GetString());
        }

        configureAvailableResourceCurrencies(list);
    }

    void GameAnalytics::configureAvailableResourceItemTypes(const StringVector& resourceItemTypes)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([resourceItemTypes]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("Available resource item types must be set before SDK is initialized");
                return;
            }
            state::GAState::setAvailableResourceItemTypes(resourceItemTypes);
        });
    }

    void GameAnalytics::configureAvailableResourceItemTypes(std::string const& resourceItemTypesJson)
    {
        rapidjson::Document json;
        json.Parse(resourceItemTypesJson.c_str());
        StringVector list;

        for (rapidjson::Value::ConstValueIterator itr = json.Begin(); itr != json.End(); ++itr)
        {
            list.push_back((*itr).GetString());
        }

        configureAvailableResourceItemTypes(list);
    }

    void GameAnalytics::configureBuild(std::string const& build)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([build]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("Build version must be set before SDK is initialized.");
                return;
            }
            if (!validators::GAValidator::validateBuild(build))
            {
                logging::GALogger::i("Validation fail - configure build: Cannot be null, empty or above 32 length. String: %s", build.data());
                return;
            }
            state::GAState::setBuild(build);
        });
    }

    void GameAnalytics::configureWritablePath(std::string const& writablePath)
    {
        if(_endThread)
        {
            return;
        }

        if (isSdkReady(true, false))
        {
            logging::GALogger::w("Writable path must be set before SDK is initialized.");
            return;
        }
        device::GADevice::setWritablePath(writablePath);

    }

    void GameAnalytics::configureBuildPlatform(std::string const& platform)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([platform]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("Platform must be set before SDK is initialized.");
                return;
            }
            if (!validators::GAValidator::validateShortString(platform, false))
            {
                logging::GALogger::i("Validation fail - configure platform: Cannot be null, empty or above 32 length. String: %s", platform.c_str());
                return;
            }
            device::GADevice::setBuildPlatform(platform);
        });
    }

    void GameAnalytics::configureCustomLogHandler(const LogHandler &logHandler)
    {
        if (_endThread)
        {
            return;
        }

        if (isSdkReady(true, false))
        {
            logging::GALogger::w("Writable path must be set before SDK is initialized.");
            return;
        }

        logging::GALogger::setCustomLogHandler(logHandler);
    }

    void GameAnalytics::disableDeviceInfo()
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("Disable device info must be set before SDK is initialized.");
                return;
            }
            device::GADevice::disableDeviceInfo();
        });
    }

    void GameAnalytics::configureDeviceModel(std::string const& deviceModel)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([deviceModel]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("Device model must be set before SDK is initialized.");
                return;
            }
            if (!validators::GAValidator::validateString(deviceModel, true))
            {
                logging::GALogger::i("Validation fail - configure device model: Cannot be null, empty or above 64 length. String: %s", deviceModel.c_str());
                return;
            }
            device::GADevice::setDeviceModel(deviceModel);
        });
    }

    void GameAnalytics::configureDeviceManufacturer(std::string const& deviceManufacturer)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([deviceManufacturer]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("Device manufacturer must be set before SDK is initialized.");
                return;
            }
            if (!validators::GAValidator::validateString(deviceManufacturer, true))
            {
                logging::GALogger::i("Validation fail - configure device manufacturer: Cannot be null, empty or above 64 length. String: %s", deviceManufacturer.c_str());
                return;
            }
            device::GADevice::setDeviceManufacturer(deviceManufacturer);
        });
    }

    void GameAnalytics::configureSdkGameEngineVersion(std::string const& sdkGameEngineVersion)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([sdkGameEngineVersion]()
        {
            if (isSdkReady(true, false))
            {
                return;
            }
            if (!validators::GAValidator::validateSdkWrapperVersion(sdkGameEngineVersion))
            {
                logging::GALogger::i("Validation fail - configure sdk version: Sdk version not supported. String: %s", sdkGameEngineVersion.c_str());
                return;
            }
            device::GADevice::setSdkGameEngineVersion(sdkGameEngineVersion);
        });
    }

    void GameAnalytics::configureGameEngineVersion(std::string const& gameEngineVersion)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([gameEngineVersion]()
        {
            if (isSdkReady(true, false))
            {
                return;
            }
            if (!validators::GAValidator::validateEngineVersion(gameEngineVersion.data()))
            {
                logging::GALogger::i("Validation fail - configure engine: Engine version not supported. String: %s", gameEngineVersion.data());
                return;
            }
            device::GADevice::setGameEngineVersion(gameEngineVersion.data());
        });
    }

    void GameAnalytics::configureUserId(std::string const& uId)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([uId]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("A custom user id must be set before SDK is initialized.");
                return;
            }
            if (!validators::GAValidator::validateUserId(uId))
            {
                logging::GALogger::i("Validation fail - configure user_id: Cannot be null, empty or above 64 length. Will use default user_id method. Used string: %s", uId.c_str());
                return;
            }

            state::GAState::setUserId(uId);
        });
    }

    // ----------------------- INITIALIZE ---------------------- //

    void GameAnalytics::initialize(std::string const& gameKey, std::string const& gameSecret)
    {
        if(_endThread)
        {
            return;
        }


#if USE_UWP
        Windows::ApplicationModel::Core::CoreApplication::Suspending += ref new Windows::Foundation::EventHandler<Windows::ApplicationModel::SuspendingEventArgs^>(&GameAnalytics::OnAppSuspending);
        Windows::ApplicationModel::Core::CoreApplication::Resuming += ref new Windows::Foundation::EventHandler<Platform::Object^>(&GameAnalytics::OnAppResuming);
#endif
        threading::GAThreading::performTaskOnGAThread([gameKey, gameSecret]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("SDK already initialized. Can only be called once.");
                return;
            }
#if !USE_UWP && !USE_TIZEN
            errorreporter::GAUncaughtExceptionHandler::setUncaughtExceptionHandlers();
#endif

            if (!validators::GAValidator::validateKeys(gameKey, gameSecret))
            {
                logging::GALogger::w("SDK failed initialize. Game key or secret key is invalid. Can only contain characters A-z 0-9, gameKey is 32 length, gameSecret is 40 length. Failed keys - gameKey: %s, secretKey: %s", gameKey.c_str(), gameSecret.c_str());
                return;
            }

            state::GAState::setKeys(gameKey, gameSecret);

            if (!store::GAStore::ensureDatabase(false, gameKey))
            {
                logging::GALogger::w("Could not ensure/validate local event database: %s", device::GADevice::getWritablePath().c_str());
            }

            state::GAState::internalInitialize();
        });
    }

    // ----------------------- ADD EVENTS ---------------------- //


    void GameAnalytics::addBusinessEvent(
        std::string const& currency,
        int amount,
        std::string const& itemType,
        std::string const& itemId,
        std::string const& cartType)
    {
        addBusinessEvent(currency, amount, itemType, itemId, cartType, "");
    }

    void GameAnalytics::addBusinessEvent(
        std::string const& currency_,
        int amount,
        std::string const& itemType_,
        std::string const& itemId_,
        std::string const& cartType_,
        std::string const& fields_)
    {
        addBusinessEvent(currency_, amount, itemType_, itemId_, cartType_, fields_, false);
    }

    void GameAnalytics::addBusinessEvent(
        std::string const& currency,
        int amount,
        std::string const& itemType,
        std::string const& itemId,
        std::string const& cartType,
        std::string const& fields,
        bool mergeFields)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([currency, amount, itemType, itemId, cartType, fields, mergeFields]()
        {
            if (!isSdkReady(true, true, "Could not add business event"))
            {
                return;
            }

            // Send to events

            try
            {
                json fieldsJson = json::parse(fields);
                events::GAEvents::addBusinessEvent(currency, amount, itemType, itemId, cartType, fieldsJson, mergeFields);
            }
            catch(std::exception& e)
            {
                logging::GALogger::e(e.what());
            }
        });
    }


    void GameAnalytics::addResourceEvent(EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId)
    {
        addResourceEvent(flowType, currency, amount, itemType, itemId, "");
    }

    void GameAnalytics::addResourceEvent(EGAResourceFlowType flowType, std::string const& currency_, float amount, std::string const& itemType_, std::string const& itemId_, std::string const& fields_)
    {
        addResourceEvent(flowType, currency_, amount, itemType_, itemId_, fields_, false);
    }

    void GameAnalytics::addResourceEvent(EGAResourceFlowType flowType, std::string const& currency, float amount, std::string const& itemType, std::string const& itemId, std::string const& fields, bool mergeFields)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([flowType, currency, amount, itemType, itemId, fields, mergeFields]()
        {
            if (!isSdkReady(true, true, "Could not add resource event"))
            {
                return;
            }

            try
            {
                json fieldsJson = json::parse(fields);
                events::GAEvents::addResourceEvent(flowType, currency, amount, itemType, itemId, fieldsJson, mergeFields);
            }
            catch (std::exception& e)
            {
                logging::GALogger::e(e.what());
            }
        });
    }

    void GameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03)
    {
        addProgressionEvent(progressionStatus, progression01, progression02, progression03, "");
    }

    void GameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01_, std::string const& progression02_, std::string const& progression03_, std::string const& fields_)
    {
        addProgressionEvent(progressionStatus, progression01_, progression02_, progression03_, fields_, false);
    }

    void GameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, std::string const& fields_, bool mergeFields)
    {
        if(_endThread)
        {
            return;
        }

        constexpr std::size_t maxFieldsSize = 4096;

        std::string fields = fields_;

        if(fields.size() > maxFieldsSize)
            fields = fields.substr(0, maxFieldsSize);

        threading::GAThreading::performTaskOnGAThread([progressionStatus, progression01, progression02, progression03, fields, mergeFields]()
        {
            if (!isSdkReady(true, true, "Could not add progression event"))
            {
                return;
            }

            // Send to events
            rapidjson::Document fieldsJson;
            fieldsJson.Parse(fields.data());
            events::GAEvents::addProgressionEvent(progressionStatus, progression01, progression02, progression03, 0, false, fieldsJson, mergeFields);
        });
    }

    void GameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, int score)
    {
        addProgressionEvent(progressionStatus, progression01, progression02, progression03, score, "");
    }

    void GameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01_, std::string const& progression02_, std::string const& progression03_, int score, std::string const& fields_)
    {
        addProgressionEvent(progressionStatus, progression01_, progression02_, progression03_, score, fields_, false);
    }

    void GameAnalytics::addDesignEvent(std::string const& eventId)
    {
        addDesignEvent(eventId, "");
    }

    void GameAnalytics::addDesignEvent(std::string const& eventId_, std::string const& fields_)
    {
        addDesignEvent(eventId_, fields_, false);
    }

    void GameAnalytics::addDesignEvent(std::string const& eventId, std::string const& fields_, bool mergeFields)
    {
        if(_endThread)
        {
            return;
        }

        std::string fields = TrimString<maxFieldsSize>(fields_);

        threading::GAThreading::performTaskOnGAThread([eventId, fields, mergeFields]()
        {
            if (!isSdkReady(true, true, "Could not add design event"))
            {
                return;
            }
            rapidjson::Document fieldsJson;
            fieldsJson.Parse(fields.c_str());
            events::GAEvents::addDesignEvent(eventId, 0, false, fieldsJson, mergeFields);
        });
    }

    void GameAnalytics::addDesignEvent(std::string const& eventId, double value)
    {
        addDesignEvent(eventId, value, "");
    }

    void GameAnalytics::addDesignEvent(std::string const& eventId_, double value, std::string const& fields_)
    {
        addDesignEvent(eventId_, value, fields_, false);
    }

    void GameAnalytics::addDesignEvent(std::string const& eventId, double value, std::string const& fields_, bool mergeFields)
    {
        if(_endThread)
        {
            return;
        }

        auto fields = TrimString<maxFieldsSize>(fields_);

        threading::GAThreading::performTaskOnGAThread([eventId, value, fields, mergeFields]()
        {
            if (!isSdkReady(true, true, "Could not add design event"))
            {
                return;
            }
            rapidjson::Document fieldsJson;
            fieldsJson.Parse(fields.data());
            events::GAEvents::addDesignEvent(eventId, value, true, fieldsJson, mergeFields);
        });
    }

    void GameAnalytics::addErrorEvent(EGAErrorSeverity severity, std::string const& message)
    {
        addErrorEvent(severity, message, "");
    }

    void GameAnalytics::addErrorEvent(EGAErrorSeverity severity, std::string const& message_, std::string const& fields_)
    {
        addErrorEvent(severity, message_, fields_, false);
    }

    void GameAnalytics::addErrorEvent(EGAErrorSeverity severity, std::string const& message_, std::string const& fields_, bool mergeFields)
    {
        if(_endThread)
        {
            return;
        }

        std::string message = TrimString<maxErrMsgSize>(message_);
        std::string fields  = TrimString<maxFieldsSize>(fields_);

        threading::GAThreading::performTaskOnGAThread([severity, message, fields, mergeFields]()
        {
            if (!isSdkReady(true, true, "Could not add error event"))
            {
                return;
            }
            rapidjson::Document fieldsJson;
            fieldsJson.Parse(fields.c_str());
            events::GAEvents::addErrorEvent(severity, message, fieldsJson, mergeFields);
        });
    }

    // ------------- SET STATE CHANGES WHILE RUNNING ----------------- //

    void GameAnalytics::setEnabledInfoLog(bool flag)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([flag]()
        {
            if (flag)
            {
                logging::GALogger::setInfoLog(flag);
                logging::GALogger::i("Info logging enabled");
            }
            else
            {
                logging::GALogger::i("Info logging disabled");
                logging::GALogger::setInfoLog(flag);
            }
        });
    }

    void GameAnalytics::setEnabledVerboseLog(bool flag)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([flag]()
        {
            if (flag)
            {
                logging::GALogger::setVerboseInfoLog(flag);
                logging::GALogger::i("Verbose logging enabled");
            }
            else
            {
                logging::GALogger::i("Verbose logging disabled");
                logging::GALogger::setVerboseInfoLog(flag);
            }
        });
    }

    void GameAnalytics::setEnabledManualSessionHandling(bool flag)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([flag]()
        {
            state::GAState::setManualSessionHandling(flag);
        });
    }

    void GameAnalytics::setEnabledErrorReporting(bool flag)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([flag]()
        {
            state::GAState::setEnableErrorReporting(flag);
        });
    }

    void GameAnalytics::setEnabledEventSubmission(bool flag)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([flag]()
        {
            if (flag)
            {
                state::GAState::setEnabledEventSubmission(flag);
                logging::GALogger::i("Event submission enabled");
            }
            else
            {
                logging::GALogger::i("Event submission disabled");
                state::GAState::setEnabledEventSubmission(flag);
            }
        });
    }

    void GameAnalytics::setCustomDimension01(std::string const& dimension_)
    {
        if(_endThread)
        {
            return;
        }

        std::string dimension = TrimString<maxDimensionSize>(dimension_);

        threading::GAThreading::performTaskOnGAThread([dimension]()
        {
            if (!validators::GAValidator::validateDimension01(dimension))
            {
                logging::GALogger::w("Could not set custom01 dimension value to '%s'. Value not found in available custom01 dimension values", dimension.c_str());
                return;
            }
            state::GAState::setCustomDimension01(dimension.data());
        });
    }

    void GameAnalytics::setCustomDimension02(std::string const& dimension_)
    {
        if(_endThread)
        {
            return;
        }

        std::string dimension = TrimString<maxDimensionSize>(dimension_);
        threading::GAThreading::performTaskOnGAThread([dimension]()
        {
            if (!validators::GAValidator::validateDimension02(dimension))
            {
                logging::GALogger::w("Could not set custom02 dimension value to '%s'. Value not found in available custom02 dimension values", dimension.c_str());
                return;
            }
            state::GAState::setCustomDimension02(dimension);
        });
    }

    void GameAnalytics::setCustomDimension03(std::string const& dimension_)
    {
        if(_endThread)
        {
            return;
        }

        std::string dimension = TrimString<maxDimensionSize>(dimension_);
        threading::GAThreading::performTaskOnGAThread([dimension]()
        {
            if (!validators::GAValidator::validateDimension03(dimension))
            {
                logging::GALogger::w("Could not set custom03 dimension value to '%s'. Value not found in available custom02 dimension values", dimension.c_str());
                return;
            }
            state::GAState::setCustomDimension03(dimension);
        });
    }

    void GameAnalytics::setGlobalCustomEventFields(std::string const& customFields_)
    {
        if (_endThread)
        {
            return;
        }

        std::string fields = TrimString<maxFieldsSize>(customFields_);
        threading::GAThreading::performTaskOnGAThread([fields]()
        {
            state::GAState::setGlobalCustomEventFields(fields);
        });
    }

    std::vector<char> GameAnalytics::getRemoteConfigsValueAsString(std::string const& key)
    {
        return getRemoteConfigsValueAsString(key, "");
    }

    std::vector<char> GameAnalytics::getRemoteConfigsValueAsString(std::string const& key, std::string const& defaultValue)
    {
        return state::GAState::getRemoteConfigsStringValue(key, defaultValue);
    }

    bool GameAnalytics::isRemoteConfigsReady()
    {
        return state::GAState::isRemoteConfigsReady();
    }

    void GameAnalytics::addRemoteConfigsListener(const std::shared_ptr<IRemoteConfigsListener>& listener)
    {
        state::GAState::addRemoteConfigsListener(listener);
    }

    void GameAnalytics::removeRemoteConfigsListener(const std::shared_ptr<IRemoteConfigsListener>& listener)
    {
        state::GAState::removeRemoteConfigsListener(listener);
    }

    std::vector<char> GameAnalytics::getRemoteConfigsContentAsString()
    {
        return state::GAState::getRemoteConfigsContentAsString();
    }

    std::vector<char> GameAnalytics::getABTestingId()
    {
        return state::GAState::getAbId();
    }

    std::vector<char> GameAnalytics::getABTestingVariantId()
    {
        return state::GAState::getAbVariantId();
    }

    void GameAnalytics::startSession()
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([]()
        {
            if(state::GAState::useManualSessionHandling())
            {
                if (!state::GAState::isInitialized())
                {
                    return;
                }

                if(state::GAState::isEnabled() && state::GAState::sessionIsStarted())
                {
                    state::GAState::endSessionAndStopQueue(false);
                }

                state::GAState::resumeSessionAndStartQueue();
            }
        });
    }

    void GameAnalytics::endSession()
    {
        if (state::GAState::useManualSessionHandling())
        {
            onSuspend();
        }
    }


    // -------------- SET GAME STATE CHANGES --------------- //

    void GameAnalytics::onResume()
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([]()
        {
            if(!state::GAState::useManualSessionHandling())
            {
                state::GAState::resumeSessionAndStartQueue();
            }
        });
    }

    void GameAnalytics::onSuspend()
    {
        if(_endThread)
        {
            return;
        }

        try
        {
            threading::GAThreading::performTaskOnGAThread([]()
            {
                state::GAState::endSessionAndStopQueue(false);
            });
        }
        catch (const std::exception&)
        {
        }
    }

    void GameAnalytics::onQuit()
    {
        if(_endThread)
        {
            return;
        }

        try
        {
            threading::GAThreading::performTaskOnGAThread([]()
            {
                _endThread = true;
                state::GAState::endSessionAndStopQueue(true);
            });

#if !USE_TIZEN
            while (!threading::GAThreading::isThreadFinished())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
#endif
        }
        catch (const std::exception&)
        {
        }
    }

    bool GameAnalytics::isThreadEnding()
    {
        return _endThread || threading::GAThreading::isThreadEnding();
    }

    // --------------PRIVATE HELPERS -------------- //

    bool GameAnalytics::isSdkReady(bool needsInitialized)
    {
        return isSdkReady(needsInitialized, true);
    }

    bool GameAnalytics::isSdkReady(bool needsInitialized, bool warn)
    {
        return isSdkReady(needsInitialized, warn, "");
    }

    bool GameAnalytics::isSdkReady(bool needsInitialized, bool warn, std::string const& message)
    {
        constexpr std::size_t maxMsgLen = 64u;
        std::string m = TrimString<maxMsgLen>(message);

        // Make sure database is ready
        if (!store::GAStore::getTableReady())
        {
            if (warn)
            {
                logging::GALogger::w("%sDatastore not initialized", m.c_str());
            }
            return false;
        }
        // Is SDK initialized
        if (needsInitialized && !state::GAState::isInitialized())
        {
            if (warn)
            {
                logging::GALogger::w("%sSDK is not initialized", m.c_str());
            }
            return false;
        }
        // Is SDK enabled
        if (needsInitialized && !state::GAState::isEnabled())
        {
            if (warn)
            {
                logging::GALogger::w("%sSDK is disabled", m.c_str());
            }
            return false;
        }

        // Is session started
        if (needsInitialized && !state::GAState::sessionIsStarted())
        {
            if (warn)
            {
                logging::GALogger::w("%sSession has not started yet", m.c_str());
            }
            return false;
        }
        return true;
    }

#if USE_UWP
    void GameAnalytics::OnAppSuspending(Platform::Object ^sender, Windows::ApplicationModel::SuspendingEventArgs ^e)
    {
        (void)sender;    // Unused parameter

        auto deferral = e->SuspendingOperation->GetDeferral();

        Concurrency::create_task([deferral]()
        {
            if (!state::GAState::useManualSessionHandling())
            {
                onSuspend();

                while(!threading::GAThreading::isThreadFinished())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            else
            {
                logging::GALogger::i("OnSuspending: Not calling GameAnalytics.OnSuspend() as using manual session handling");
            }
            deferral->Complete();
        });


    }

    void GameAnalytics::OnAppResuming(Platform::Object ^sender, Platform::Object ^args)
    {
        (void)sender;    // Unused parameter

        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([]()
        {
            if (!state::GAState::useManualSessionHandling())
            {
                onResume();
            }
            else
            {
                logging::GALogger::i("OnResuming: Not calling GameAnalytics.OnResume() as using manual session handling");
            }
        });
    }

#endif

} // namespace gameanalytics
