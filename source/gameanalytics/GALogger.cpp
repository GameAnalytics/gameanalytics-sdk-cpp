#include "GALogger.h"
#include "GameAnalytics.h"
#include <iostream>
#include "GADevice.h"
#include <cstdarg>
#include <exception>
#include "GACommon.h"
#include "zf_log.h"
#include "GAUtilities.h"

constexpr const char* LOG_FILE_NAME = "ga_log.txt";
constexpr int         MAX_LOG_COUNT = 5000;

namespace gameanalytics
{
    namespace logging
    {
        GALogger::GALogger()
        {
            infoLogEnabled = false;
            customLogHandler = {};
            logInitialized = false;
            currentLogCount = 0;
            maxLogCount = MAX_LOG_COUNT;


#if defined(_DEBUG)
            // log debug is in dev mode
            debugEnabled = true;
#else
            debugEnabled = false;
#endif
        }

        GALogger::~GALogger()
        {
        }

        GALogger& GALogger::getInstance()
        {
            static GALogger instance;
            return instance;
        }

        void GALogger::setCustomLogHandler(LogHandler handler)
        {
            getInstance().customLogHandler = std::make_unique<LogHandler>(handler);
        }

        void GALogger::setInfoLog(bool enabled)
        {
            getInstance().infoLogEnabled = enabled;
        }

        void GALogger::setVerboseInfoLog(bool enabled)
        {
            getInstance().infoLogVerboseEnabled = enabled;
        }

        void GALogger::file_output_callback(const zf_log_message *msg, void *arg)
        {
            try
            {
                if (!msg)
                {
                    return;
                }

                (void)arg;
                *msg->p = '\n';

                getInstance().logFile.write(msg->buf, msg->p - msg->buf + 1);
                getInstance().logFile.flush();
            }
            catch (std::exception& e)
            {
                std::cerr << "Error/GameAnalytics:" << e.what() << "\n";
            }
        }

        void GALogger::initializeLog()
        {
            std::string writablepath = device::GADevice::getWritablePath();
            if(writablepath.empty())
            {
                return;
            }

            std::string logFilePath = writablepath + "/" + LOG_FILE_NAME;

            logFile.open(logFilePath);

            if (!logFile.is_open())
            {
                ZF_LOGW("Failed to open log file %s", logFilePath.c_str());
                return;
            }

            zf_log_set_output_v(ZF_LOG_PUT_STD, 0, file_output_callback);

            logInitialized  = true;
            currentLogCount = 0;

            GALogger::i("Log file added under: %s", device::GADevice::getWritablePath().c_str());
        }


        void GALogger::sendNotificationMessage(std::string const& message, EGALoggerMessageType type)
        {
            if(customLogHandler)
            {
                auto& handler = *customLogHandler;
                handler(message, type);
                return;
            }

            if(!logInitialized)
            {
                initializeLog();
            }
            
            if (logFile.is_open())
            {
                logFile << message << '\n';
            }


            //std::lock_guard<std::mutex> guard(_mutex);
            switch(type)
            {
                case LogError:
                case LogWarning:
                    std::cerr << message << '\n';
                    break;

                case LogDebug:
                case LogInfo:
                case LogVerbose:
                default:
                    std::cout << message << '\n';
            }
        }
    }
}
