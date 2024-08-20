//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#include "GADevice.h"
#include "GAUtilities.h"
#include "Platform/GADevicePlatform.h"

namespace gameanalytics
{
    namespace device
    {
        GADevice& GADevice::getInstance()
        {
            static GADevice instance;
            return instance;
        }

        GADevice::GADevice():
            _platform(MakePlatform()),
            _sdkWrapperVersion(GA_VERSION_STR)
        {
        }

        void GADevice::initPlatform()
        {
            if(getInstance()._platform)
            {
                getInstance()._platform->onInit();
            }
        }

        void GADevice::disableDeviceInfo()
        {
            getInstance()._useDeviceInfo = false;
        }

        void GADevice::setSdkGameEngineVersion(std::string const& sdkGameEngineVersion)
        {
            getInstance()._sdkGameEngineVersion = sdkGameEngineVersion;
        }

        std::string GADevice::getGameEngineVersion()
        {
            return getInstance()._gameEngineVersion;
        }

        void GADevice::setGameEngineVersion(std::string const& gameEngineVersion)
        {
            getInstance()._gameEngineVersion = gameEngineVersion;
        }

        void GADevice::setConnectionType(std::string const& connectionType)
        {
            getInstance()._connectionType = connectionType;
        }

        std::string GADevice::getConnectionType()
        {
            if(getInstance()._platform)
                return getInstance()._platform->getConnectionType();

            return "";
        }

        std::string GADevice::getRelevantSdkVersion()
        {
            if (!getInstance()._sdkGameEngineVersion.empty())
            {
                return getInstance()._sdkGameEngineVersion;
            }

            return getInstance()._sdkWrapperVersion;
        }

        std::string GADevice::getBuildPlatform()
        {
            if(getInstance()._buildPlatform.empty())
            {
                getInstance().initRuntimePlatform();
            }

            return getInstance()._buildPlatform;
        }

        void GADevice::setBuildPlatform(std::string const& platform)
        {
            getInstance()._buildPlatform = platform;
        }

        std::string GADevice::getOSVersion()
        {
            if(getInstance()._osVersion.empty())
            {
                getInstance().initOSVersion();
            }

            return getInstance()._osVersion;
        }

        void GADevice::setDeviceModel(std::string const& deviceModel)
        {
            if (deviceModel.empty())
            {
                getInstance()._deviceModel = UNKNOWN_VALUE;
            }
            else
            {
                getInstance()._deviceModel = deviceModel;
            }
        }

        std::string GADevice::getDeviceModel()
        {
            if(getInstance()._deviceModel.empty())
            {
                getInstance().initDeviceModel();
            }

            return getInstance()._deviceModel;
        }

        void GADevice::setDeviceManufacturer(std::string const& deviceManufacturer)
        {
            getInstance()._deviceManufacturer = deviceManufacturer;
        }

        std::string GADevice::getDeviceManufacturer()
        {
            if(getInstance()._deviceManufacturer.empty())
            {
                getInstance().initDeviceManufacturer();
            }

            return getInstance()._deviceManufacturer;
        }

        void GADevice::setWritablePath(std::string const& writablepath)
        {
            getInstance()._writablepath = writablepath;
        }

        std::string GADevice::getWritablePath()
        {
            if(getInstance()._writablepath.empty())
            {
                getInstance().initPersistentPath();
            }

            return getInstance()._writablepath;
        }

        bool GADevice::getWritablePathStatus()
        {
            return !getInstance()._writablepath.empty();
        }

        void GADevice::initDeviceId()
        {
            if (_platform)
            {
                _deviceId = _platform->getDeviceId();
            }
        }

        void GADevice::initOSVersion()
        {
            if (_platform)
            {
                _osVersion = _platform->getOSVersion();
            }
        }

        void GADevice::initDeviceManufacturer()
        {
            if (_platform && _useDeviceInfo)
            {
                _deviceManufacturer = _platform->getDeviceManufacturer();
            }
        }

        void GADevice::initDeviceModel()
        {
            if (_platform && _useDeviceInfo)
            {
                _deviceModel = _platform->getDeviceModel();
            }
        }

        std::string GADevice::getDeviceId()
        {
            return getInstance()._deviceId;
        }

        std::string GADevice::getAdvertisingId()
        {
            return getInstance()._advertisingId;
        }

        void GADevice::initRuntimePlatform()
        {
            if (_platform)
            {
                _buildPlatform = _platform->getBuildPlatform();
            }
        }

        void GADevice::initPersistentPath()
        {
            if (_platform)
            {
                _writablepath = _platform->getPersistentPath();
            }
        }
    }
}
