#pragma once

#include "GACommon.h"
#include "Platform/GAPlatform.h"

namespace gameanalytics
{
    class GAHealth
    {
        public:

            GAHealth(GAPlatform* platform);

            bool enableMemoryTracking      = false;
            bool enableFPSTracking         = false;
            bool enableHardwareTracking    = false;
            bool enableAppBootTimeTracking = false;

            void addHealthAnnotations(json& healthEvent);
            void addPerformanceData(json& healthEvent);
            void addSDKInitData(json& sdkInitEvent);

            void addMemoryTracker();
            void addFPSTracker(FPSTracker fpsTracker);

            virtual void doFpsReading(float fps);
            virtual void doAppMemoryReading(int64_t memory);
            virtual void doSysMemoryReading(int64_t memory);

        protected:

            static constexpr size_t MAX_FPS_COUNT      = 120 + 1;
            static constexpr size_t MAX_MEMORY_COUNT   = 100 + 1;

            static constexpr std::chrono::milliseconds MEMORY_TRACK_FREQ {5000};
            static constexpr std::chrono::milliseconds FPS_TRACK_FREQ    {1000};

            std::array<uint32_t, MAX_MEMORY_COUNT> _appMemoryUsage;
            std::array<uint32_t, MAX_MEMORY_COUNT> _sysMemoryUsage;
            std::array<uint32_t, MAX_FPS_COUNT>    _fpsReadings;

            int64_t  _totalMemory   = 0;
            uint32_t _numCores      = 0;

            std::string _cpuModel;
            std::string _hardware;
            std::string _gpuModel;
            std::string _screenResolution;

            GAPlatform* _platform = nullptr;

            bool _isMemoryTracked = false;
            bool _isFPSTracked    = false;

            FPSTracker _fpsTracker;

            int  getMemoryPercent(int64_t memory);
            void queryMemory();
    };
}
