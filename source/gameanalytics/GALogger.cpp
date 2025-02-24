#include "GALogger.h"
#include "GameAnalytics/GameAnalytics.h"
#include <iostream>
#include "GADevice.h"
#include <cstdarg>
#include <exception>
#include "GACommon.h"
#include "GAUtilities.h"
#include "GAState.h"

#define ZF_LOG_SRCLOC ZF_LOG_SRCLOC_NONE
#define ZF_LOG_LEVEL ZF_LOG_VERBOSE
#define ZF_LOG_BUF_SZ 2048
#include "zf_log.h"

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
            return state::GAState::getInstance()._gaLogger;
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

            logFile.open(logFilePath, std::ios::out | std::ios::trunc);

            if (!logFile.is_open())
            {
                ZF_LOGW("Failed to open log file %s", logFilePath.c_str());
                return;
            }

            //zf_log_set_output_v(ZF_LOG_PUT_STD, 0, file_output_callback);

            logInitialized  = true;
            currentLogCount = 0;

            GALogger::i("Log file added under: %s", logFilePath.c_str());
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

            switch(type)
            {
                case LogError:
                    ZF_LOGE("%s", message.c_str());
                    break;

                case LogWarning:
                    ZF_LOGW("%s", message.c_str());
                    break;

                case LogDebug:
                    ZF_LOGD("%s", message.c_str());
                    break;

                case LogInfo:
                    ZF_LOGI("%s", message.c_str());
                    break;

                case LogVerbose:
                default:
                    ZF_LOGV("%s", message.c_str());
            }
        }
    }
}
