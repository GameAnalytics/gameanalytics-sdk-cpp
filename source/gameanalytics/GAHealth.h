#pragma once

#include "GACommon.h"
#include "GAPlatform.h"

namespace gameanalytics
{
    class GAHealth
    {
        protected:

            static constexpr size_t MAX_FPS_COUNT      = 120 + 1;
            static constexpr size_t MAX_MEMORY_COUNT   = 100 + 1;

            std::array<uint32_t, MAX_MEMORY_COUNT> _appMemoryUsage;
            std::array<uint32_t, MAX_MEMORY_COUNT> _sysMemoryUsage;
            std::array<uint32_t, MAX_FPS_COUNT>    _fpsReadings;

            int64_t  _totalMemory   = 0;
            uint32_t _numCores      = 0;

            std::string _cpuModel;
            std::string _hardware;
            std::string _gpuModel;
            std::string _screenResolution;

            int getMemoryPercent(int64_t memory);

        public:

            bool enableMemoryTracking   = false;
            bool enableFPSTracking      = false;
            bool enableHardwareTracking = false;

            void addHealthAnnotations(json& healthEvent);
            void addHealthTables(json& healthEvent);

            virtual void doFpsReading(float fps);
            virtual void doAppMemoryReading(int64_t memory);
            virtual void doSysMemoryReading(int64_t memory);
    };
}