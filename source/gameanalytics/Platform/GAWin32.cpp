#include "GAWin32.h"

#if IS_WIN32

#include "GAUtilities.h"

#include "GAState.h"
#include "GAEvents.h"
#include "GALogger.h"

#include <psapi.h>
#include <wininet.h>

#pragma comment(lib, "Wininet.lib")

#include <stacktrace/call_stack.hpp>

namespace gameanalytics
{

void (*GAPlatformWin32::old_state_ill)	(int) = nullptr;
void (*GAPlatformWin32::old_state_abrt)	(int) = nullptr;
void (*GAPlatformWin32::old_state_fpe)	(int) = nullptr;
void (*GAPlatformWin32::old_state_segv)	(int) = nullptr;

// no official helper function exists for win11 (for the time being at least)
bool IsWin11OrGreater()
{
    constexpr DWORD MAJOR_VERSION = 10;
    constexpr DWORD MINOR_VERSION = 0;
    constexpr DWORD BUILD_NUM     = 21996;

    return IsWindowsVersionOrGreater(MAJOR_VERSION, MINOR_VERSION, BUILD_NUM);
}

std::string GAPlatformWin32::getOSVersion()
{
    std::string osVersion = getBuildPlatform() + " ";

    #if (_MSC_VER >= 1900)
        if(IsWin11OrGreater())
        {
            return osVersion + "11";
        }
        else if (IsWindows10OrGreater())
        {
            return osVersion + "10";
        }
    #endif

        if (IsWindows8Point1OrGreater())
        {
            return osVersion + "6.3";
        }
        else if (IsWindows8OrGreater())
        {
            return osVersion + "6.2";
        }
        else if (IsWindows7OrGreater())
        {
            return osVersion + "6.1";
        }
        else if (IsWindowsVistaOrGreater())
        {
            return osVersion + "6.1";
        }
        else if (IsWindowsXPOrGreater())
        {
            return osVersion + "5.1";
        }

    return osVersion + "0.0.0";
}

std::string GAPlatformWin32::getConnectionType()
{
    DWORD flags = {};
    if(InternetGetConnectedState(&flags, 0))
    {
        if(INTERNET_CONNECTION_OFFLINE & flags)
        {
            return CONNECTION_OFFLINE;
        }
        else if (INTERNET_CONNECTION_LAN & flags)
        {
            return CONNECTION_LAN;
        }
        else
        {
            return CONNECTION_WIFI;
        }
    }

    return "";
}

std::string GAPlatformWin32::getBuildPlatform()
{
    return "windows";
}

std::string GAPlatformWin32::getPersistentPath()
{
    std::string path = "GameAnalytics";

    char* appData = std::getenv("LOCALAPPDATA");
    if (appData && strlen(appData))
    {
        path = std::string(appData) + '\\' + path;
    }

    if(!std::filesystem::exists(path))
    {
        std::filesystem::create_directories(path);
    }

    return path;
}

std::string getRegistryKey(HKEY key, const TCHAR* subkey, const TCHAR* value)
{
    {
        constexpr DWORD maxBufSize = 128;

        DWORD size = maxBufSize * sizeof(TCHAR);
        TCHAR buffer[maxBufSize] = _T("");
        RegGetValue(key, subkey, value, RRF_RT_REG_SZ, NULL, buffer, &size);

        if (!GetLastError() && size > 0)
        {
            std::string val;

#ifdef UNICODE
            std::wstring wstr(buffer, buffer + size);
            val = utilities::GAUtilities::ws2s(wstr);
#else
            val = std::string(buffer, buffer + size);
#endif

            return val;
        }
    }

    return UNKNOWN_VALUE;
}

std::string GAPlatformWin32::getDeviceModel()
{
    constexpr const TCHAR* subkey = _T("SYSTEM\\CurrentControlSet\\Control\\SystemInformation");
    constexpr const TCHAR* value  = _T("SystemProductName");

    return getRegistryKey(HKEY_LOCAL_MACHINE, subkey, value);
}

std::string GAPlatformWin32::getDeviceManufacturer()
{
    constexpr const TCHAR* subkey = _T("SYSTEM\\CurrentControlSet\\Control\\SystemInformation");
    constexpr const TCHAR* value  = _T("SystemManufacturer");

    return getRegistryKey(HKEY_LOCAL_MACHINE, subkey, value);
}

std::string GAPlatformWin32::getCpuModel() const
{
    constexpr const TCHAR* subkey = _T("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");

    constexpr const TCHAR* values[] = { _T("ProcessorName"), _T("ProcessorNameString") };

    for (auto& val : values)
    {
        std::string s = getRegistryKey(HKEY_LOCAL_MACHINE, subkey, val);
        if (!s.empty())
            return s;
    }

    return UNKNOWN_VALUE;
}

void GAPlatformWin32::setupUncaughtExceptionHandler()
{
    signal(SIGILL,  signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGFPE,  signalHandler);
    signal(SIGSEGV, signalHandler);
}

void GAPlatformWin32::signalHandler(int sig)
{
    static int errorCount = 0;

    if (state::GAState::useErrorReporting())
    {
        if (errorCount <= MAX_ERROR_TYPE_COUNT)
        {
            stacktrace::call_stack st;

            std::string msg = "Uncaught Signal:" + std::to_string(sig) + "\n Stack trace: \n";

            std::size_t const strSize = st.to_string_size();

            std::unique_ptr<char[]> buffer = std::make_unique<char[]>(strSize);

            if (buffer && strSize)
            {
                errorCount++;

                std::string truncatedMsg(buffer.get(), std::min<std::size_t>(strSize, MAX_ERROR_MSG_LEN));
                events::GAEvents::addErrorEvent(EGAErrorSeverity::Critical, truncatedMsg, "", -1, {}, false);
                events::GAEvents::processEvents("error", false);
            }
        }
    }
    if (sig == SIGILL && old_state_ill != NULL)
    {
        old_state_ill(sig);
    }
    else if (sig == SIGABRT && old_state_abrt != NULL)
    {
        old_state_abrt(sig);
    }
    else if (sig == SIGFPE && old_state_fpe != NULL)
    {
        old_state_fpe(sig);
    }
    else if (sig == SIGSEGV && old_state_segv != NULL)
    {
        old_state_segv(sig);
    }
}

std::string GAPlatformWin32::getGpuModel() const
{
    DISPLAY_DEVICE device;
    ZeroMemory(&device, sizeof(DISPLAY_DEVICE));
    
    if(EnumDisplayDevices(NULL, 0, &device, 0))
    {
#ifdef UNICODE
        return utilities::GAUtilities::ws2s(device.DeviceName);
#else
        return device.DeviceName;
#endif
    }

    return UNKNOWN_VALUE;
}

int GAPlatformWin32::getNumCpuCores() const
{
    DWORD len = 0;
    GetLogicalProcessorInformation(nullptr, &len);

    if(len && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        const int size = len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        auto buffer = std::make_unique<SYSTEM_LOGICAL_PROCESSOR_INFORMATION[]>(size);
        if(buffer)
        {
            if(GetLogicalProcessorInformation(buffer.get(), &len))
            {
                int numProcessors = 0;
                for(int i = 0; i < size; ++i)
                {
                    if(buffer[i].Relationship == RelationProcessorCore)
                    {
                        ++numProcessors;
                    }
                }

                return numProcessors;
            }
        }
    }

    return 0;
}

int64_t GAPlatformWin32::getTotalDeviceMemory() const
{
    MEMORYSTATUSEX memInfo;
    ZeroMemory(&memInfo, sizeof(MEMORYSTATUSEX));

    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return utilities::convertBytesToMB(memInfo.ullTotalPhys);
}

int64_t GAPlatformWin32::getAppMemoryUsage() const
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    ZeroMemory(&pmc, sizeof(PROCESS_MEMORY_COUNTERS_EX));

    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    return utilities::convertBytesToMB(pmc.PrivateUsage);
}

int64_t GAPlatformWin32::getSysMemoryUsage() const
{
    MEMORYSTATUSEX memInfo;
    ZeroMemory(&memInfo, sizeof(MEMORYSTATUSEX));

    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return utilities::convertBytesToMB(memInfo.ullTotalPhys - memInfo.ullAvailPhys);
}

int64_t GAPlatformWin32::getBootTime() const
{
    FILETIME creationTime = {};
    FILETIME exitTime = {};
    FILETIME kernelTime = {};
    FILETIME userTime = {};

    if(GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime))
    {
        ULARGE_INTEGER creation;
        creation.LowPart  = creationTime.dwLowDateTime;
        creation.HighPart = creationTime.dwHighDateTime;
        
        FILETIME currentTime;
        GetSystemTimeAsFileTime(&currentTime);

        ULARGE_INTEGER current;
        current.LowPart  = currentTime.dwLowDateTime;
        current.HighPart = currentTime.dwHighDateTime;

        int64_t value = static_cast<int64_t>(current.QuadPart - creation.QuadPart);
        if(value < 0ll)
        {
            return 0ll;
        }
        
        // filetime is expressed in 100s of nanoseconds
        std::chrono::nanoseconds timeInNs = std::chrono::nanoseconds(value * 100);
        return std::chrono::duration_cast<std::chrono::seconds>(timeInNs).count();
    }

    return 0ll;
}

}
#endif
