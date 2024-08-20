#include "Platform/GALinux.h"

#if IS_LINUX

std::string gameanalytics::GAPlatformLinux::getOSVersion()
{
    struct utsname info;
    uname(&info);

    std::string version;
    int const strSize = strlen(info.release));

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


#endif
