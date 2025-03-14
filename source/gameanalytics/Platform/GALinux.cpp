#include "Platform/GALinux.h"

#if IS_LINUX

#include "GAState.h"

#include <sstream>
#include <errno.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <execinfo.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <ifaddrs.h>

struct sigaction gameanalytics::GAPlatformLinux::prevSigAction;

struct ProcessStat 
{
    int pid;
    std::string comm;
    char state;
    long ppid;
    long pgrp;
    long session;
    long tty_nr;
    long tpgid;
    unsigned long flags;
    unsigned long minflt;
    unsigned long cminflt;
    unsigned long majflt;
    unsigned long cmajflt;
    unsigned long utime;
    unsigned long stime;
    long cutime;
    long cstime;
    long priority;
    long nice;
    long num_threads;
    long itrealvalue;
    long starttime;
    long vsize;
    long rss;
};

ProcessStat readProcessStat() 
{
    ProcessStat stat{};
    std::ifstream statFile("/proc/self/stat");
    
    if (!statFile.is_open()) 
    {
        return {};
    }

    std::string line;
    std::getline(statFile, line);
    std::istringstream iss(line);

    iss >> stat.pid >> stat.comm >> stat.state >> stat.ppid >> stat.pgrp >> stat.session
        >> stat.tty_nr >> stat.tpgid >> stat.flags >> stat.minflt >> stat.cminflt
        >> stat.majflt >> stat.cmajflt >> stat.utime >> stat.stime
        >> stat.cutime >> stat.cstime >> stat.priority >> stat.nice
        >> stat.num_threads >> stat.itrealvalue >> stat.starttime >> stat.vsize >> stat.rss;

    return stat;
}

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

std::string gameanalytics::GAPlatformLinux::getCpuModel() const
{
    struct utsname systemInfo;
    uname(&systemInfo);

    return systemInfo.machine;
}

std::string gameanalytics::GAPlatformLinux::getConnectionType()
{
    struct ifaddrs* list = nullptr;
    struct ifaddrs* current = nullptr;

    std::string connection = CONNECTION_OFFLINE;

    if(getifaddrs(&list) == -1)
    {
        return connection;
    }

    current = list;
    while(current)
    {
        int sock = -1;
        if (!current->ifa_addr || current->ifa_addr->sa_family != AF_PACKET) 
        {
            struct iwreq req = {};
            strncpy(req.ifr_name, current->ifa_name, IFNAMSIZ);

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == -1) 
            {
                connection = CONNECTION_LAN;
            }
            else
            {
                if (ioctl(sock, SIOCGIWNAME, &req) != -1) 
                {
                    connection = CONNECTION_WIFI;
                }
            }
        }

        if(sock != -1)
            close(sock);

        current = current->ifa_next;
    }

    freeifaddrs(list);
    return connection;
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
    constexpr int k_pageSize = 1024;
    ProcessStat proc = readProcessStat();

    int64_t vmUsage      = proc.vsize / k_pageSize;
    int64_t resident     = proc.rss * (sysconf(_SC_PAGE_SIZE) / k_pageSize);

    int64_t inBytes = (vmUsage + resident) * 1024;

    return utilities::convertBytesToMB(inBytes);
}

int64_t gameanalytics::GAPlatformLinux::getSysMemoryUsage() const
{
    struct sysinfo info = {};
    if(sysinfo(&info) == 0)
    {
        return utilities::convertBytesToMB(info.totalram - info.freeram);
    }

    return 0;
}

int64_t gameanalytics::GAPlatformLinux::getBootTime() const
{
    ProcessStat procStat = readProcessStat();

    struct sysinfo info = {};
    if(sysinfo(&info) != 0)
    {
        return 0;
    }

    int64_t startTime = procStat.starttime / sysconf(_SC_CLK_TCK);
    int64_t bootTime = info.uptime - startTime;

    return bootTime;
}

#endif
