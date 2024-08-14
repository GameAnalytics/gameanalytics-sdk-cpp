#include "GAWin32.h"

#ifdef _WIN32

#include "GAUtilities.h"

#include "GAState.h"
#include "GAEvents.h"
#include <stacktrace/call_stack.hpp>

namespace gameanalytics
{

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
            return osVersion + "6.1";
        }
        else if (IsWindowsXPOrGreater())
        {
            return osVersion + "5.1";
        }

    return osVersion + "0.0.0";
}

std::string GAPlatformWin32::getDeviceManufacturer()
{
#if !GA_SHARED_LIB
    IWbemLocator*  locator  = nullptr;
    IWbemServices* services = nullptr;

    auto hResult = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hResult))
    {
        return UNKNOWN_VALUE;
    }
    hResult = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);


    auto hasFailed = [&hResult]() {
        if (FAILED(hResult))
        {
            return true;
        }
        return false;
        };

    auto getValue = [&hResult, &hasFailed](IWbemClassObject* classObject, LPCWSTR property) {
        BSTR propertyValueText = L"unknown";
        VARIANT propertyValue;
        hResult = classObject->Get(property, 0, &propertyValue, 0, 0);
        if (!hasFailed()) {
            if ((propertyValue.vt == VT_NULL) || (propertyValue.vt == VT_EMPTY)) {
            }
            else if (propertyValue.vt & VT_ARRAY) {
                propertyValueText = L"unknown"; //Array types not supported
            }
            else {
                propertyValueText = propertyValue.bstrVal;
            }
        }
        VariantClear(&propertyValue);
        return propertyValueText;
        };

    BSTR manufacturer = L"unknown";
    if (!hasFailed()) 
    {
        // Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
        hResult = locator->ConnectServer(L"ROOT\\CIMV2", nullptr, nullptr, 0, NULL, 0, 0, &services);

        if (!hasFailed()) {
            // Set the IWbemServices proxy so that impersonation of the user (client) occurs.
            hResult = CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

            if (!hasFailed()) {
                IEnumWbemClassObject* classObjectEnumerator = nullptr;
                hResult = services->ExecQuery(L"WQL", L"SELECT * FROM Win32_ComputerSystem", WBEM_FLAG_FORWARD_ONLY |
                    WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &classObjectEnumerator);
                if (!hasFailed()) 
                {
                    IWbemClassObject* classObject;
                    ULONG uReturn = 0;
                    hResult = classObjectEnumerator->Next(WBEM_INFINITE, 1, &classObject, &uReturn);
                    if (uReturn != 0) {
                        manufacturer = getValue(classObject, (LPCWSTR)L"Manufacturer");
                    }

                    if(classObject)
                        classObject->Release();
                }
                classObjectEnumerator->Release();
            }
        }
    }

    if (locator) {
        locator->Release();
    }
    if (services) {
        services->Release();
    }
    CoUninitialize();

    std::string manufacturer = _com_util::ConvertBSTRToString(manufacturer);
    return manufacturer;

#else
    return UNKNOWN_VALUE;
#endif
}

std::string GAPlatformWin32::getBuildPlatform()
{
    return "windows";
}

std::string GAPlatformWin32::getPersistentPath()
{
    char* appData = std::getenv("LOCALAPPDATA");

    if (!appData)
        return "";

    std::string path = std::string(appData) + "\\GameAnalytics";

    int result = _mkdir(path.c_str());
    if (result == 0 || errno == EEXIST)
    {
        return path;
    }
    else
    {
        return "";
    }
}

std::string GAPlatformWin32::getDeviceModel()
{
    try
    {
        constexpr const TCHAR* subkey = _T("SYSTEM\\CurrentControlSet\\Control\\SystemInformation");
        constexpr const TCHAR* value  = _T("SystemProductName");

        constexpr DWORD maxBufSize = 128;

        DWORD size = maxBufSize;
        TCHAR buffer[maxBufSize] = _T("");
        RegGetValue(HKEY_LOCAL_MACHINE, subkey, value, RRF_RT_REG_SZ, NULL, buffer, &size);

        if (!GetLastError() && size > 0)
        {
            std::string modelName;

#ifdef UNICODE
            std::wstring wstr(buffer, buffer + size);
            modelName = utilities::GAUtilities::ws2s(wstr);
#else
            modelName = std::string(buffer, buffer + size);
#endif

            return modelName;
        }
    }
    catch (...)
    {
    }

#ifdef GA_USE_WBEM_SERVICES

    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;
    auto hResult = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hResult))
    {
        snprintf(GADevice::_deviceModel, sizeof(GADevice::_deviceModel), "unknown");
    }
    hResult = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);

    auto hasFailed = [&hResult]() {
        if (FAILED(hResult))
        {
            return true;
        }
        return false;
        };

    auto getValue = [&hResult, &hasFailed](IWbemClassObject* classObject, LPCWSTR property) {
        BSTR propertyValueText = L"unknown";
        VARIANT propertyValue = {};

        if(!classObject || !property)
            return propertyValueText;

        hResult = classObject->Get(property, 0, &propertyValue, 0, 0);
        if (!hasFailed()) {
            if ((propertyValue.vt == VT_NULL) || (propertyValue.vt == VT_EMPTY)) {
            }
            else if (propertyValue.vt & VT_ARRAY) {
                propertyValueText = L"unknown"; //Array types not supported
            }
            else {
                propertyValueText = propertyValue.bstrVal;
            }
        }
        VariantClear(&propertyValue);
        return propertyValueText;
        };

    BSTR model = L"unknown";
    if (!hasFailed()) {
        // Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
        hResult = locator->ConnectServer(L"ROOT\\CIMV2", nullptr, nullptr, 0, NULL, 0, 0, &services);

        if (!hasFailed()) {
            // Set the IWbemServices proxy so that impersonation of the user (client) occurs.
            hResult = CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

            if (!hasFailed()) {
                IEnumWbemClassObject* classObjectEnumerator = nullptr;
                hResult = services->ExecQuery(L"WQL", L"SELECT * FROM Win32_ComputerSystem", WBEM_FLAG_FORWARD_ONLY |
                    WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &classObjectEnumerator);
                if (!hasFailed()) {
                    IWbemClassObject* classObject;
                    ULONG uReturn = 0;
                    hResult = classObjectEnumerator->Next(WBEM_INFINITE, 1, &classObject, &uReturn);
                    if (uReturn != 0) {
                        model = getValue(classObject, (LPCWSTR)L"Model");
                    }

                    if(classObject)
                        classObject->Release();
                }

                if(classObjectEnumerator)
                    classObjectEnumerator->Release();
            }
        }
    }

    if (locator) 
    {
        locator->Release();
    }

    if (services) 
    {
        services->Release();
    }

    CoUninitialize();

    return _com_util::ConvertBSTRToString(model);
#else
    return "";
#endif // GA_USE_WBEM_SERVICES
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
                events::GAEvents::addErrorEvent(EGAErrorSeverity::Critical, truncatedMsg, {}, false);
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

}
#endif
