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
#include <cstdlib>
#include <thread>
#include <array>
#include "stacktrace/call_stack.hpp"

namespace gameanalytics
{
    bool GameAnalytics::_endThread = false;

    constexpr std::size_t maxFieldsSize     = 4096u;
    constexpr std::size_t maxErrMsgSize     = 8182u;
    constexpr std::size_t maxDimensionSize  = 64u;

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
                logging::GALogger::i("Validation fail - configure build: Cannot be null, empty or above 32 length. String: %s", build.c_str());
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
            if (!validators::GAValidator::validateEngineVersion(gameEngineVersion))
            {
                logging::GALogger::i("Validation fail - configure engine: Engine version not supported. String: %s", gameEngineVersion.c_str());
                return;
            }
            device::GADevice::setGameEngineVersion(gameEngineVersion);
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

    void GameAnalytics::configureExternalUserId(std::string const& uId)
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

            state::GAState::setExternalUserId(uId);
        });
    }

    // ----------------------- INITIALIZE ---------------------- //

    void GameAnalytics::initialize(std::string const& gameKey, std::string const& gameSecret)
    {
        if(_endThread)
        {
            return;
        }


        threading::GAThreading::performTaskOnGAThread([gameKey, gameSecret]()
        {
            if (isSdkReady(true, false))
            {
                logging::GALogger::w("SDK already initialized. Can only be called once.");
                return;
            }

            device::GADevice::initPlatform();
            
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
        std::string const& cartType,
        std::string const& fields,
        bool mergeFields)
    {
        if(_endThread)
        {
            return;
        }

        threading::GAThreading::performTaskOnGAThread([=]()
        {
            if (!isSdkReady(true, true, "Could not add business event"))
            {
                return;
            }

            // Send to events

            try
            {
                json fieldsJson = utilities::parseFields(fields);
                events::GAEvents::addBusinessEvent(currency, amount, itemType, itemId, cartType, fieldsJson, mergeFields);
            }
            catch(json::exception const& e)
            {
                logging::GALogger::e("addBusinessEvent - Failed to parse json:", e.what());
            }
            catch(std::exception const& e)
            {
                logging::GALogger::e("addBusinessEvent - Exception thrown:", e.what());
            }
        });
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
                json fieldsJson = utilities::parseFields(fields);
                events::GAEvents::addResourceEvent(flowType, currency, amount, itemType, itemId, fieldsJson, mergeFields);
            }
            catch (std::exception& e)
            {
                logging::GALogger::e(e.what());
            }
        });
    }

    void GameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, int score, std::string const& progression01, std::string const& progression02, std::string const& progression03, std::string const& fields, bool mergeFields)
    {
        if(_endThread)
        {
            return;
        }

        if(fields.size() > maxFieldsSize)
        {
            logging::GALogger::w("Custom fields length exceeded, maximum allowed is %d, fields' size was %d", maxFieldsSize, fields.size());
            return;
        }

        threading::GAThreading::performTaskOnGAThread([=]()
        {
            if (!isSdkReady(true, true, "Could not add progression event"))
            {
                return;
            }

            try
            {
                // Send to events
                json fieldsJson = utilities::parseFields(fields);
                events::GAEvents::addProgressionEvent(progressionStatus, progression01, progression02, progression03, score, false, fieldsJson, mergeFields);
            }
            catch(const json::exception& e)
            {
                logging::GALogger::e("Failed to parse custom fields: %s", e.what());
            }
            catch(std::exception const& e)
            {
                logging::GALogger::e("Exception thrown: %s", e.what());
            }
        });
    }

    void GameAnalytics::addProgressionEvent(EGAProgressionStatus progressionStatus, std::string const& progression01, std::string const& progression02, std::string const& progression03, std::string const& fields, bool mergeFields)
    {
        return addProgressionEvent(progressionStatus, 0, progression01, progression02, progression03, fields, mergeFields);
    }

    void GameAnalytics::addDesignEvent(std::string const& eventId, double value, std::string const& fields, bool mergeFields)
    {
        if(_endThread)
        {
            return;
        }

        if(fields.size() > maxFieldsSize)
        {
            logging::GALogger::w("Custom fields length exceeded, maximum allowed is %d, fields size was %d", maxFieldsSize, fields.size());
            return;
        }

        threading::GAThreading::performTaskOnGAThread([=]()
        {
            if (!isSdkReady(true, true, "Could not add design event"))
            {
                return;
            }
            
            try
            {
                json fieldsJson = utilities::parseFields(fields);
                events::GAEvents::addDesignEvent(eventId, value, false, fieldsJson, mergeFields);
            }
            catch(json::exception const& e)
            {
                logging::GALogger::e("addDesignEvent - Failed to parse fields: %s", e.what());
            }
        });
    }

    void GameAnalytics::addDesignEvent(std::string const& eventId, std::string const& fields, bool mergeFields)
    {
        return addDesignEvent(eventId, 0.0, fields, mergeFields);
    }

    void GameAnalytics::addErrorEvent(EGAErrorSeverity severity, std::string const& message_, std::string const& fields, bool mergeFields)
    {
        if(_endThread)
        {
            return;
        }

        const std::string message = utilities::trimString(message_, maxErrMsgSize);

        std::string function;
        int32_t line = -1;

        std::pair<std::string, int32_t> inFunction = utilities::getRelevantFunctionFromCallStack();
        
        function = inFunction.first;
        line     = inFunction.second;
        
        if(fields.size() > maxFieldsSize)
        {
            logging::GALogger::w("Custom fields length exceeded, maximum allowed is %d, fields' size was %d", maxFieldsSize, fields.size());
            return;
        }

        threading::GAThreading::performTaskOnGAThread([=]()
        {
            if (!isSdkReady(true, true, "Could not add error event"))
            {
                return;
            }

            try
            {
                json fieldsJson = utilities::parseFields(fields);
                events::GAEvents::addErrorEvent(severity, message, function, line, fieldsJson, mergeFields);
            }
            catch(std::exception& e)
            {
                logging::GALogger::e("Failed to parse custom fields: %s", e.what());
            }
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

        std::string dimension = utilities::trimString(dimension_, maxDimensionSize);

        threading::GAThreading::performTaskOnGAThread([dimension]()
        {
            if (!validators::GAValidator::validateDimension01(dimension))
            {
                logging::GALogger::w("Could not set custom01 dimension value to '%s'. Value not found in available custom01 dimension values", dimension.c_str());
                return;
            }
            state::GAState::setCustomDimension01(dimension);
        });
    }

    void GameAnalytics::setCustomDimension02(std::string const& dimension_)
    {
        if(_endThread)
        {
            return;
        }

        std::string dimension = utilities::trimString(dimension_, maxDimensionSize);
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

        std::string dimension = utilities::trimString(dimension_, maxDimensionSize);
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

        std::string fields = utilities::trimString(customFields_, maxFieldsSize);
        threading::GAThreading::performTaskOnGAThread([fields]()
        {
            state::GAState::setGlobalCustomEventFields(fields);
        });
    }

    std::string GameAnalytics::getRemoteConfigsValueAsString(std::string const& key)
    {
        return getRemoteConfigsValueAsString(key, "");
    }

    std::string GameAnalytics::getRemoteConfigsValueAsString(std::string const& key, std::string const& defaultValue)
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

    std::string GameAnalytics::getRemoteConfigsContentAsString()
    {
        return state::GAState::getRemoteConfigsContentAsString();
    }

    std::string GameAnalytics::getABTestingId()
    {
        return state::GAState::getAbId();
    }

    std::string GameAnalytics::getABTestingVariantId()
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

            while (!threading::GAThreading::isThreadFinished())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        catch (const std::exception&)
        {
        }
    }

    bool GameAnalytics::isThreadEnding()
    {
        return _endThread || threading::GAThreading::isThreadFinished();
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
        std::string m = utilities::trimString(message, maxMsgLen);

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
                logging::GALogger::w("%s;SDK is disabled", m.c_str());
            }
            return false;
        }

        // Is session started
        if (needsInitialized && !state::GAState::sessionIsStarted())
        {
            if (warn)
            {
                logging::GALogger::w("%s;Session has not started yet", m.c_str());
            }
            return false;
        }
        return true;
    }

    void GameAnalytics::enableSDKInitEvent(bool value)
    {
        GAHealth* healthTracker = device::GADevice::getHealthTracker();
        if(healthTracker)
        {
            healthTracker->enableAppBootTimeTracking = value;
            events::GAEvents::getInstance().enableSDKInitEvent = value;
        }
    }

    void GameAnalytics::enableMemoryHistogram(bool value)
    {
        GAHealth* healthTracker = device::GADevice::getHealthTracker();
        if(healthTracker)
        {
            healthTracker->enableMemoryTracking = value;
            healthTracker->addMemoryTracker();
            if(value)
            {
                events::GAEvents::getInstance().enableHealthEvent = true;
            }
        }
    }
    
    void GameAnalytics::enableFPSHistogram(FPSTracker fpsTracker, bool value)
    {
        GAHealth* healthTracker = device::GADevice::getHealthTracker();
        if(healthTracker)
        {
            healthTracker->enableFPSTracking = value;
            healthTracker->addFPSTracker(fpsTracker);
            if(value)
            {
                events::GAEvents::getInstance().enableHealthEvent = true;
            }
        }
    }

    void GameAnalytics::enableHardwareTracking(bool value)
    {
        GAHealth* healthTracker = device::GADevice::getHealthTracker();
        if(healthTracker)
        {
            healthTracker->enableHardwareTracking = value;
        }
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
        (void)sender; // Unused parameter

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
