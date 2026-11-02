#include "GAWin32.h"

#if IS_WIN32

#include "GAUtilities.h"

#include "GAState.h"
#include "GAEvents.h"
#include "GALogger.h"

#include <psapi.h>
#include <wininet.h>
#include <intrin.h>

#include <cctype>
#include <cstring>
#include <vector>

#pragma comment(lib, "Wininet.lib")

#include <stacktrace/call_stack.hpp>

namespace gameanalytics
{

void (*GAPlatformWin32::old_state_ill)	(int) = nullptr;
void (*GAPlatformWin32::old_state_abrt)	(int) = nullptr;
void (*GAPlatformWin32::old_state_fpe)	(int) = nullptr;
void (*GAPlatformWin32::old_state_segv)	(int) = nullptr;

// RtlGetVersion reports the real OS version. GetVersionEx and the VersionHelpers.h functions
// are subject to the app manifest: without a supportedOS entry for Windows 10 they report 6.2 (Windows 8),
// and they have no notion of a build number so cannot tell Windows 11 apart from Windows 10.
bool getWindowsVersion(DWORD& major, DWORD& minor, DWORD& build)
{
    using RtlGetVersionFn = LONG (WINAPI*)(PRTL_OSVERSIONINFOW);

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll)
    {
        return false;
    }

    auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
    if (!rtlGetVersion)
    {
        return false;
    }

    RTL_OSVERSIONINFOW info = {};
    info.dwOSVersionInfoSize = sizeof(info);

    // returns an NTSTATUS, 0 is STATUS_SUCCESS
    if (rtlGetVersion(&info) != 0)
    {
        return false;
    }

    major = info.dwMajorVersion;
    minor = info.dwMinorVersion;
    build = info.dwBuildNumber;

    return true;
}

std::string GAPlatformWin32::getOSVersion()
{
    std::string osVersion = getBuildPlatform() + " ";

    DWORD major = 0;
    DWORD minor = 0;
    DWORD build = 0;

    if (getWindowsVersion(major, minor, build))
    {
        // Windows 11 kept the 10.0 version number, the first release build is 22000
        constexpr DWORD WIN11_FIRST_BUILD = 22000;

        if (major == 10)
        {
            return osVersion + (build >= WIN11_FIRST_BUILD ? "11" : "10");
        }
        else if (major > 10)
        {
            return osVersion + std::to_string(major);
        }

        return osVersion + std::to_string(major) + "." + std::to_string(minor);
    }

    // fallback, manifest dependent (see above)
    #if (_MSC_VER >= 1900)
        if (IsWindows10OrGreater())
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
        return osVersion + "6.0";
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
    // RegGetValue reports errors through its return value, not GetLastError,
    // and the size it reports is in bytes (including the null terminator)
    DWORD size = 0;
    if (RegGetValue(key, subkey, value, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS || size <= sizeof(TCHAR))
    {
        return UNKNOWN_VALUE;
    }

    std::vector<TCHAR> buffer(size / sizeof(TCHAR) + 1, 0);
    size = static_cast<DWORD>(buffer.size() * sizeof(TCHAR));

    if (RegGetValue(key, subkey, value, RRF_RT_REG_SZ, nullptr, buffer.data(), &size) != ERROR_SUCCESS)
    {
        return UNKNOWN_VALUE;
    }

    std::string val;

#ifdef UNICODE
    val = utilities::GAUtilities::ws2s(std::wstring(buffer.data()));
#else
    val = std::string(buffer.data());
#endif

    return val.empty() ? UNKNOWN_VALUE : val;
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

// trims and collapses repeated whitespace (brand strings are often padded, e.g. "       Intel(R) Core(TM) i7 CPU")
std::string normalizeCpuName(std::string const& name)
{
    std::string result;
    result.reserve(name.size());

    for (char c : name)
    {
        if (c == '\0')
        {
            break;
        }

        if (std::isspace(static_cast<unsigned char>(c)))
        {
            if (!result.empty() && result.back() != ' ')
            {
                result += ' ';
            }
        }
        else
        {
            result += c;
        }
    }

    if (!result.empty() && result.back() == ' ')
    {
        result.pop_back();
    }

    return result;
}

std::string getCpuidBrandString()
{
#if (defined(_M_X64) || defined(_M_IX86)) && !defined(_M_ARM64EC)
    int regs[4] = {};

    __cpuid(regs, 0x80000000);
    if (static_cast<unsigned int>(regs[0]) < 0x80000004)
    {
        return "";
    }

    char brand[49] = {};
    for (int i = 0; i < 3; ++i)
    {
        __cpuid(regs, 0x80000002 + i);
        std::memcpy(brand + i * sizeof(regs), regs, sizeof(regs));
    }

    return normalizeCpuName(brand);
#else
    return "";
#endif
}

std::string getCpuArchitecture()
{
    SYSTEM_INFO info = {};
    GetNativeSystemInfo(&info);

    switch (info.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64:  return "x86_64";
        case PROCESSOR_ARCHITECTURE_ARM64:  return "arm64";
        case PROCESSOR_ARCHITECTURE_INTEL:  return "x86";
        case PROCESSOR_ARCHITECTURE_ARM:    return "arm";
        default:                            return UNKNOWN_VALUE;
    }
}

std::string GAPlatformWin32::getCpuModel() const
{
    // the registry holds the name of the physical cpu (also correct for x64 apps emulated on arm64)
    constexpr const TCHAR* subkey = _T("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
    constexpr const TCHAR* value  = _T("ProcessorNameString");

    std::string name = getRegistryKey(HKEY_LOCAL_MACHINE, subkey, value);
    if (name != UNKNOWN_VALUE)
    {
        name = normalizeCpuName(name);
        if (!name.empty())
        {
            return name;
        }
    }

    name = getCpuidBrandString();
    if (!name.empty())
    {
        return name;
    }

    return getCpuArchitecture();
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
    
    device.cb = sizeof(DISPLAY_DEVICE);

    if(EnumDisplayDevices(NULL, 0, &device, EDD_GET_DEVICE_INTERFACE_NAME))
    {
#ifdef UNICODE
        return utilities::GAUtilities::ws2s(device.DeviceString);
#else
        return device.DeviceString;
#endif
    }

    return "";
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
        return std::chrono::duration_cast<std::chrono::milliseconds>(timeInNs).count();
    }

    return 0ll;
}

}
#endif
