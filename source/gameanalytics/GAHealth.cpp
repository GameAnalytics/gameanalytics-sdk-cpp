#include "GAHealth.h"
#include "GAUtilities.h"

namespace gameanalytics
{
    GAHealth::GAHealth(GAPlatform* platform)
    {
        if(platform)
        {
            _cpuModel = platform->getCpuModel();
            _numCores = platform->getNumCpuCores();
            _hardware = platform->getHardware();
            _gpuModel = platform->getGpuModel();
            _totalMemory = platform->getTotalDeviceMemory();
        }
    }

    void GAHealth::doFpsReading(float fps)
    {
        int fpsBucket = std::round(fps);
        if(fpsBucket >= 0 && fpsBucket < MAX_FPS_COUNT)
        {
            _fpsReadings[fpsBucket]++;
        }
    }

    int GAHealth::getMemoryPercent(int64_t memory)
    {
        if((memory > 0) && (_totalMemory > 0))
        {
            int memoryPercent = std::round(static_cast<double>(memory) / static_cast<double>(_totalMemory) * 100.0);
            return memoryPercent;
        }

        return -1;
    }

    void GAHealth::doAppMemoryReading(int64_t memory)
    {
        int percent = getMemoryPercent(memory);
        if(percent >= 0 && percent < MAX_MEMORY_COUNT)
        {
            _appMemoryUsage[percent]++;
        }
    }

    void GAHealth::doSysMemoryReading(int64_t memory)
    {
        int percent = getMemoryPercent(memory);
        if(percent >= 0 && percent < MAX_MEMORY_COUNT)
        {
            _sysMemoryUsage[percent]++;
        }
    }

    void GAHealth::addHealthAnnotations(json& out)
    {
        if(enableHardwareTracking)
        {
            utilities::addIfNotEmpty(out, "cpu_model", _cpuModel);
            utilities::addIfNotEmpty(out, "hardware",  _hardware);
            
            if(_numCores > 0)
            {
                out["cpu_num_cores"] = _numCores;
            }
        }

        if (_totalMemory > 0 && enableMemoryTracking)
        {
            out["memory_sys_total"] = _totalMemory;
        }
    }

    void GAHealth::addHealthTables(json& out)
    {
        if(enableFPSTracking)
        {
            json fps = json::array();
            for(uint32_t i: _fpsReadings)
            {
                fps.push_back(i);
            }

            out["fps_data_table"] = fps;
        }

        if (_totalMemory > 0 && enableMemoryTracking)
        {
            json sysMemUsed = json::array();
            for(uint32_t i: _sysMemoryUsage)
            {
                sysMemUsed.push_back(i);
            }

            json appMemUsed = json::array();
            for(uint32_t i: _appMemoryUsage)
            {
                appMemUsed.push_back(i);
            }

            out["memory_sys_data_table"] = sysMemUsed;
            out["memory_app_data_table"] = appMemUsed;
        }
    }
}