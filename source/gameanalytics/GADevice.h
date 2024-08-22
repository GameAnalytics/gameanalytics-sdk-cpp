//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include "GAConstants.h"
#include "GACommon.h"
#include "Platform/GAPlatform.h"

namespace gameanalytics
{
    namespace device
    {
        class GADevice
        {
            friend class state::GAState;
            static GADevice& getInstance();

            GADevice();
            GADevice(GADevice const&)            = delete;
            GADevice& operator=(GADevice const&) = delete;

            public:

                static void         initPlatform();

                static void         disableDeviceInfo();
                static void         setSdkGameEngineVersion(std::string const& sdkGameEngineVersion);
                static std::string  getGameEngineVersion();

                static void         setGameEngineVersion(std::string const& gameEngineVersion);
                static void         setConnectionType(std::string const& connectionType);

                static std::string  getConnectionType();
                static std::string  getRelevantSdkVersion();

                static std::string  getBuildPlatform();
                static void         setBuildPlatform(std::string const& platform);

                static std::string  getOSVersion();

                static void         setDeviceModel(std::string const& deviceModel);
                static std::string  getDeviceModel();

                static void         setDeviceManufacturer(std::string const& deviceManufacturer);
                static std::string  getDeviceManufacturer();

                static void         setWritablePath(std::string const& writablePath);
                static std::string  getWritablePath();
                static bool         getWritablePathStatus();

                static std::string  getDeviceId();
                static std::string  getAdvertisingId();

            private:

                std::unique_ptr<GAPlatform> _platform;

                void initDeviceId();
                void initOSVersion();
                void initDeviceManufacturer();
                void initDeviceModel();
                void initRuntimePlatform();
                void initPersistentPath();

                std::string _deviceId;
                std::string _advertisingId;

                bool        _useDeviceInfo = true;

                std::string _buildPlatform;
                std::string _osVersion;
                std::string _deviceModel;
                std::string _deviceManufacturer;

                std::string _writablepath;
                bool        _writablepathStatus{false};

                std::string _sdkGameEngineVersion;
                std::string _gameEngineVersion;
                std::string _connectionType;
                std::string _sdkWrapperVersion;
        };
    }
}
