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
        std::unique_ptr<GADevice> GADevice::_instance = nullptr;

        std::unique_ptr<GADevice>& GADevice::GetInstance()
        {
            if (!_instance)
            {
                _instance = std::make_unique<GADevice>();
            }

            return _instance;
        }

        GADevice::GADevice()
        {
        }

        void GADevice::disableDeviceInfo()
        {
            GetInstance()->_useDeviceInfo = false;
        }

        void GADevice::setSdkGameEngineVersion(std::string const& sdkGameEngineVersion)
        {
            GetInstance()->_sdkGameEngineVersion;
        }

        std::string GADevice::getGameEngineVersion()
        {
            return GetInstance()->_gameEngineVersion;
        }

        void GADevice::setGameEngineVersion(std::string const& gameEngineVersion)
        {
            GetInstance()->_gameEngineVersion = gameEngineVersion;
        }

        void GADevice::setConnectionType(std::string const& connectionType)
        {
            GetInstance()->_connectionType = connectionType;
        }

        std::string GADevice::getConnectionType()
        {
            return GetInstance()->_connectionType;
        }

        std::string GADevice::getRelevantSdkVersion()
        {
            if (!GetInstance()->_sdkGameEngineVersion.empty())
            {
                return _instance->_sdkGameEngineVersion;
            }

            return GetInstance()->_sdkWrapperVersion;
        }

        std::string GADevice::getBuildPlatform()
        {
            if(GetInstance()->_buildPlatform.empty())
            {
                _instance->initRuntimePlatform();
            }

            return GetInstance()->_buildPlatform;
        }

        void GADevice::setBuildPlatform(std::string const& platform)
        {
            GetInstance()->_buildPlatform = platform;
        }

        std::string GADevice::getOSVersion()
        {
            if(GetInstance()->_osVersion.empty())
            {
                _instance->initOSVersion();
            }

            return GetInstance()->_osVersion;
        }

        void GADevice::setDeviceModel(std::string const& deviceModel)
        {
            if (deviceModel.empty())
            {
                GetInstance()->_deviceModel = UNKNOWN_VALUE;
            }
            else
            {
                GetInstance()->_deviceModel = deviceModel;
            }
        }

        std::string GADevice::getDeviceModel()
        {
            if(GetInstance()->_deviceModel.empty())
            {
                _instance->initDeviceModel();
            }

            return GetInstance()->_deviceModel;
        }

        void GADevice::setDeviceManufacturer(std::string const& deviceManufacturer)
        {
            GetInstance()->_deviceManufacturer = deviceManufacturer;
        }

        std::string GADevice::getDeviceManufacturer()
        {
            if(GetInstance()->_deviceManufacturer.empty())
            {
                _instance->initDeviceManufacturer();
            }

            return GetInstance()->_deviceManufacturer;
        }

        void GADevice::setWritablePath(std::string const& writablepath)
        {
            GetInstance()->_writablepath = writablepath;
        }

        std::string GADevice::getWritablePath()
        {
            if(GetInstance()->_writablepath.empty())
            {
                GetInstance()->initPersistentPath();
            }

            return GetInstance()->_writablepath;
        }

        bool GADevice::getWritablePathStatus()
        {
            return !_instance->_writablepath.empty();
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
            return GetInstance()->_deviceId;
        }

        std::string GADevice::getAdvertisingId()
        {
            return GetInstance()->_advertisingId;
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
