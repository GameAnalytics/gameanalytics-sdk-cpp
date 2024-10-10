#include "GAHealth.h"
#include "GAUtilities.h"
#include "GAThreading.h"

namespace gameanalytics
{
    GAHealth::GAHealth(GAPlatform* platform):
        _platform(platform)
    {
        std::fill(_fpsReadings.begin(), _fpsReadings.end(), 0);
        std::fill(_appMemoryUsage.begin(), _appMemoryUsage.end(), 0);
        std::fill(_sysMemoryUsage.begin(), _sysMemoryUsage.end(), 0);

        if(platform)
        {
            _cpuModel    = platform->getCpuModel();
            _numCores    = platform->getNumCpuCores();
            _hardware    = platform->getDeviceModel();
            _gpuModel    = platform->getGpuModel();
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
			return std::min(memoryPercent, 100);
        }

        return 0;
		
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

    void GAHealth::addPerformanceData(json& out)
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

    void GAHealth::addSDKInitData(json& out)
    {
        if(_platform && enableAppBootTimeTracking)
        {
            int64_t appBootTime = _platform->getBootTime();

            if(appBootTime > 0)
                out["app_boot_time"] = appBootTime;
        }
    }

    void GAHealth::queryMemory()
    {
        if(_platform && enableMemoryTracking)
        {
            int64_t appMemory = _platform->getAppMemoryUsage();
            if(appMemory > 0)
            {
                doAppMemoryReading(appMemory);
            }

            int64_t sysMemory = _platform->getSysMemoryUsage();
            if(sysMemory > 0)
            {
                doSysMemoryReading(sysMemory);
            }
        }
    }

    void GAHealth::addMemoryTracker()
    {
        if(!_isMemoryTracked)
        {
            _isMemoryTracked = true;
            threading::GAThreading::scheduleTimer(MEMORY_TRACK_FREQ, 
                [this]() 
                {
                    queryMemory();
                }
            );
        }
    }

    void GAHealth::addFPSTracker(FPSTracker fpsTracker)
    {
        _fpsTracker = fpsTracker;
        if(!_isFPSTracked)
        {
            _isFPSTracked = true;
            threading::GAThreading::scheduleTimer(FPS_TRACK_FREQ, 
                [this]()
                {
                    if(enableFPSTracking)
                    {
                        float fps = _fpsTracker();
                        doFpsReading(fps);
                    }
                }
            );

        }
    }
}
