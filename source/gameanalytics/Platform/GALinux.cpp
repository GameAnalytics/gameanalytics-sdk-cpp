#include "Platform/GALinux.h"

#if IS_LINUX

#include "GAState.h"

#include <execinfo.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <mach/mach.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sysinfo.h>

std::string gameanalytics::GAPlatformLinux::getOSVersion()
{
    struct utsname info;
    uname(&info);

    std::string version;
    int const strSize = strlen(info.release);

    for (size_t i = 0; i < strSize; ++i)
    {
        if (!isdigit(info.release[i]) && info.release[i] != '.')
        {
            version = std::string(info.release, info.release + i);
            break;
        }
    }

    return getBuildPlatform() + " " + version;
}

std::string gameanalytics::GAPlatformLinux::getDeviceManufacturer()
{
    return UNKNOWN_VALUE;
}

std::string gameanalytics::GAPlatformLinux::getBuildPlatform()
{
    return "linux";
}

std::string gameanalytics::GAPlatformLinux::getPersistentPath()
{
    std::string path = std::getenv("HOME");
    path += "/GameAnalytics";

    mode_t nMode = 0733;
    int result = mkdir(path.c_str(), nMode);
    if (result == 0 || errno == EEXIST)
    {
        return path;
    }
    else
    {
        return "";
    }
}

std::string gameanalytics::GAPlatformLinux::getDeviceModel()
{
    return UNKNOWN_VALUE;
}

void gameanalytics::GAPlatformLinux::setupUncaughtExceptionHandler()
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

void gameanalytics::GAPlatformLinux::signalHandler(int sig, siginfo_t* info, void* context)
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
            events::GAEvents::addErrorEvent(EGAErrorSeverity::Critical, stackTrace, {}, false, false);
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

std::string gameanalytics::GAPlatformLinux::getCpuModel() const
{
    struct utsname systemInfo;
    uname(&systemInfo);

    return systemInfo.machine;
}

std::string gameanalytics::GAPlatformLinux::getGpuModel() const 
{
    return UNKNOWN_VALUE;
}

int gameanalytics::GAPlatformLinux::getNumCpuCores() const
{
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

int64_t gameanalytics::GAPlatformLinux::getTotalDeviceMemory() const 
{
    struct sysinfo info = {};
    if(sysinfo(&info) == 0)
    {
        return utilities::convertBytesToMB(info.totalram);
    }

    return 0;
}

int64_t gameanalytics::GAPlatformLinux::getAppMemoryUsage() const
{
    struct task_basic_info info;
    
    mach_msg_type_number_t infoSize = TASK_BASIC_INFO_COUNT;
    kern_return_t result = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &infoSize);
    
    if(result == KERN_SUCCESS) 
    {
        return utilities::convertBytesToMB(info.resident_size);
    }

    return 0;
}

int64_t gameanalytics::GAPlatformLinux::getSysMemoryUsage() const
{
    mach_port_t port = mach_host_self();
    mach_msg_type_number_t hostSize = sizeof(vm_statistics_data_t) / sizeof(integer_t);
    
    vm_size_t pageSize;
    host_page_size(port, &pageSize);
    
    vm_statistics_data_t stats;
    
    if(host_statistics(port, HOST_VM_INFO, (host_info_t)&stats, &hostSize) == KERN_SUCCESS)
    {
        const int64_t freeMemory = (stats.free_count + stats.inactive_count) * pageSize;
        return getTotalDeviceMemory() - utilities::convertBytesToMB(freeMemory);
    }

    return 0;
}

int64_t gameanalytics::GAPlatformLinux::getBootTime() const
{
    const size_t len = 4;
    int mib[len] = {0,0,0,0};
    struct kinfo_proc kp = {};

    const size_t pidId = 3;
    
    size_t num = len;
    sysctlnametomib("kern.proc.pid", mib, &num);
    mib[pidId] = getpid();
    
    num = sizeof(kp);
    sysctl(mib, len, &kp, &num, NULL, 0);

    struct timeval startTime = kp.kp_proc.p_un.__p_starttime;
    struct timeval currentTime = {};
    
    gettimeofday(&currentTime, NULL);
    
    return currentTime.tv_sec - startTime.tv_sec;
}

#endif
