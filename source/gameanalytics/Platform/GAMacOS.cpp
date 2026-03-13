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
#include <unistd.h>
#include <array>

static std::array<struct sigaction, NSIG> prevSigActions = {};
static std::array<bool, NSIG> hasPrevSigAction = {};

std::string gameanalytics::GAPlatformMacOS::getOSVersion()
{
    std::string osxVersion = getOSXVersion();
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
    constexpr int signalsToHandle[] = {
        SIGQUIT, SIGILL, SIGTRAP, SIGABRT,
        SIGFPE, SIGBUS, SIGSEGV, SIGSYS,
        SIGPIPE, SIGALRM, SIGXCPU, SIGXFSZ
    };

    for (int signalNumber : signalsToHandle)
    {
        struct sigaction previous = {};
        if (sigaction(signalNumber, NULL, &previous) == 0)
        {
            prevSigActions[signalNumber] = previous;
            hasPrevSigAction[signalNumber] = true;

            if (previous.sa_handler != SIG_IGN)
            {
                sigaction(signalNumber, &mySigAction, NULL);
            }
        }
    }
}

void gameanalytics::GAPlatformMacOS::signalHandler(int sig, siginfo_t* info, void* context)
{
    constexpr int NUM_MAX_FRAMES = 128;
    static volatile sig_atomic_t errorCount = 0;

    if (state::GAState::useErrorReporting())
    {
        void* frames[NUM_MAX_FRAMES];
        int len = backtrace(frames, NUM_MAX_FRAMES);
        char** symbols = backtrace_symbols(frames, len);

        /*
         *    Now format into a message for sending to the user
         */
        std::string stackTrace = "Stack trace:\n";
        for (int i = 0; symbols != NULL && i < len; ++i)
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

        if (symbols != NULL)
        {
            free(symbols);
        }

        struct sigaction newact;
        newact.sa_flags = 0;
        sigemptyset(&newact.sa_mask);
        newact.sa_handler = SIG_DFL;
        sigaction(sig, &newact, NULL);
    }

    if (sig >= 0 && sig < NSIG && hasPrevSigAction[sig])
    {
        const struct sigaction& previous = prevSigActions[sig];

        if ((previous.sa_flags & SA_SIGINFO) && previous.sa_sigaction != NULL)
        {
            previous.sa_sigaction(sig, info, context);
            return;
        }

        if (previous.sa_handler == SIG_IGN)
        {
            return;
        }

        if (previous.sa_handler != NULL && previous.sa_handler != SIG_DFL)
        {
            previous.sa_handler(sig);
            return;
        }
    }

    signal(sig, SIG_DFL);
    raise(sig);
}

std::string gameanalytics::GAPlatformMacOS::getCpuModel() const
{
    struct utsname systemInfo;
    uname(&systemInfo);

    return systemInfo.machine;
}

std::string gameanalytics::GAPlatformMacOS::getGpuModel() const 
{
    if(_gpuModel.empty())
    {
        _gpuModel = ::getGPUName();
    }
    
    return _gpuModel;
}

int gameanalytics::GAPlatformMacOS::getNumCpuCores() const
{
    return ::getNumCpuCores();
}

int64_t gameanalytics::GAPlatformMacOS::getTotalDeviceMemory() const 
{
    return utilities::convertBytesToMB(::getTotalDeviceMemory());
}

int64_t gameanalytics::GAPlatformMacOS::getAppMemoryUsage() const
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
        return getTotalDeviceMemory() - utilities::convertBytesToMB(freeMemory);
    }

    return 0;
}

int64_t gameanalytics::GAPlatformMacOS::getBootTime() const
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

    int64_t remainingMs = static_cast<double>(currentTime.tv_usec - startTime.tv_usec) * 1e-3;
    return (currentTime.tv_sec - startTime.tv_sec) * 1000 + remainingMs;
}

#endif
