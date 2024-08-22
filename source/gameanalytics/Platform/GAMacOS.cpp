#include "GAMacOS.h"

#if IS_MAC

#include "GADeviceOSX.h"
#include "GAState.h"
#include "GAEvents.h"

#include <execinfo.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <mach/mach.h>
#include <sys/stat.h>

static struct sigaction prevSigAction;

std::string gameanalytics::GAPlatformMacOS::getOSVersion()
{
    const char* osxVersion = getOSXVersion();
    return getBuildPlatform() + " " + osxVersion;
}

std::string gameanalytics::GAPlatformMacOS::getDeviceManufacturer()
{
    return "Apple";
}

std::string gameanalytics::GAPlatformMacOS::getBuildPlatform()
{
    return "mac_osx";
}

std::string gameanalytics::GAPlatformMacOS::getConnectionType()
{
    return ::getConnectionType();
}

std::string gameanalytics::GAPlatformMacOS::getPersistentPath()
{
    std::string path = "GameAnalytics";
    
    const char* homeDir = std::getenv("HOME");
    if(homeDir && strlen(homeDir))
    {
        if(std::filesystem::exists(homeDir))
        {
            path = std::string(homeDir) + "/" + path;
        }
    }
    
    if(!std::filesystem::exists(path))
    {
        std::filesystem::create_directory(path);
    }
    
    return path;
}

std::string gameanalytics::GAPlatformMacOS::getDeviceModel()
{
    size_t len = 0;
    sysctlbyname("hw.model", NULL, &len, NULL, 0);

    const size_t buffSize = len + 1;

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(buffSize);
    std::memset(buffer.get(), 0, buffSize);

    sysctlbyname("hw.model", buffer.get(), &len, NULL, 0);

    std::string model = buffer.get();
    return model;
}

void gameanalytics::GAPlatformMacOS::setupUncaughtExceptionHandler()
{
    struct sigaction mySigAction;
    mySigAction.sa_sigaction = signalHandler;
    mySigAction.sa_flags = SA_SIGINFO;

    sigemptyset(&mySigAction.sa_mask);
    sigaction(SIGQUIT, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGQUIT, &mySigAction, NULL);
    }
    sigaction(SIGILL, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGILL, &mySigAction, NULL);
    }
    sigaction(SIGTRAP, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGTRAP, &mySigAction, NULL);
    }
    sigaction(SIGABRT, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGABRT, &mySigAction, NULL);
    }

    sigaction(SIGFPE, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGFPE, &mySigAction, NULL);
    }
    sigaction(SIGBUS, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGBUS, &mySigAction, NULL);
    }
    sigaction(SIGSEGV, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGSEGV, &mySigAction, NULL);
    }
    sigaction(SIGSYS, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGSYS, &mySigAction, NULL);
    }
    sigaction(SIGPIPE, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGPIPE, &mySigAction, NULL);
    }
    sigaction(SIGALRM, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGALRM, &mySigAction, NULL);
    }
    sigaction(SIGXCPU, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGXCPU, &mySigAction, NULL);
    }
    sigaction(SIGXFSZ, NULL, &prevSigAction);
    if (prevSigAction.sa_handler != SIG_IGN)
    {
        sigaction(SIGXFSZ, &mySigAction, NULL);
    }
}

void gameanalytics::GAPlatformMacOS::signalHandler(int sig, siginfo_t* info, void* context)
{
    constexpr int NUM_MAX_FRAMES = 128;
    static int errorCount = 0;

    if (state::GAState::useErrorReporting())
    {
        void* frames[NUM_MAX_FRAMES];
        int len = backtrace(frames, NUM_MAX_FRAMES);
        char** symbols = backtrace_symbols(frames, len);

        /*
         *    Now format into a message for sending to the user
         */
        std::string stackTrace = "Stack trace:\n";
        for (int i = 0; i < len; ++i)
        {
            stackTrace += symbols[i];
            stackTrace += '\n';
        }

        if (errorCount <= MAX_ERROR_TYPE_COUNT)
        {
            errorCount++;
            events::GAEvents::addErrorEvent(EGAErrorSeverity::Critical, stackTrace, "", -1, {}, false, false);
            events::GAEvents::processEvents("error", false);
        }

        struct sigaction newact;
        newact.sa_flags = 0;
        sigemptyset(&newact.sa_mask);
        newact.sa_handler = SIG_DFL;
    }

    if (*prevSigAction.sa_handler != NULL)
    {
        (*prevSigAction.sa_handler)(sig);
    }
}

std::string gameanalytics::GAPlatformMacOS::getCpuModel() const
{
    struct utsname systemInfo;
    uname(&systemInfo);

    return systemInfo.machine;
}

std::string gameanalytics::GAPlatformMacOS::getGpuModel() const 
{
    return "Apple";
}

int gameanalytics::GAPlatformMacOS::getNumCpuCores() const
{
    return ::getNumCpuCores();
}

int64_t gameanalytics::GAPlatformMacOS::getTotalDeviceMemory() const 
{
    return ::getTotalDeviceMemory();
}

int64_t gameanalytics::GAPlatformMacOS::getAppMemoryUsage() const
{
    struct task_basic_info info;
    
    mach_msg_type_number_t infoSize = TASK_BASIC_INFO_COUNT;
    kern_return_t result = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &infoSize);
    
    if(result == KERN_SUCCESS) 
    {
        return (uint64_t)info.resident_size;
    }

    return 0;
}

int64_t gameanalytics::GAPlatformMacOS::getSysMemoryUsage() const
{
    mach_port_t port = mach_host_self();
    mach_msg_type_number_t hostSize = sizeof(vm_statistics_data_t) / sizeof(integer_t);
    
    vm_size_t pageSize;
    host_page_size(port, &pageSize);
    
    vm_statistics_data_t stats;
    
    if(host_statistics(port, HOST_VM_INFO, (host_info_t)&stats, &hostSize) == KERN_SUCCESS)
    {
        const int64_t freeMemory = (stats.free_count + stats.inactive_count) * pageSize;
        return getTotalDeviceMemory() - freeMemory;
    }

    return 0;
}

#endif
