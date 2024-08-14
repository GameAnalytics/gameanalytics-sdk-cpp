//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#include "GACommon.h"
#include "GameAnalytics.h"

#define ZF_LOG_SRCLOC ZF_LOG_SRCLOC_NONE
#include "zf_log.h"

namespace gameanalytics
{
    namespace logging
    {
        class GALogger
        {
            using LogHandler = GameAnalytics::LogHandler;

            template<typename ...args_t>
            static void sendMessage(EGALoggerMessageType logType, std::string const& fmt, args_t&&... args)
            {
                GALogger* ga = GALogger::getInstance();
                if (ga)
                {
                    if (logType == LogVerbose && !ga->infoLogVerboseEnabled)
                    {
                        return;
                    }

                    std::string tag = ga->tag + " :";
                    switch (logType)
                    {
                        case LogError:
                            tag = "Error/" + tag;
                            break;
                        
                        case LogWarning:
                            tag = "Warning/" + tag;
                            break;

                        case LogDebug:
                            tag = "Debug/" + tag;
                            break;

                        case LogInfo:
                            tag = "Info/" + tag;
                            break;

                        case LogVerbose:
                        default:
                            tag = "Verbose/" + tag;

                    }

                    try
                    {
                        const std::string msg = tag + utilities::printString(fmt, std::forward<args_t>(args)...);
                        ga->sendNotificationMessage(msg, logType);
                    }
                    catch (std::exception const& e)
                    {
                        std::cerr << "Error/GameAnalytics:" << e.what() << "\n";
                    }
                }
            }

            public:

                // set debug enabled (client)
                static void setInfoLog(bool enabled);
                static void setVerboseInfoLog(bool enabled);
                static void setCustomLogHandler(LogHandler handler);

                // Debug (w/e always shows, d only shows during SDK development, i shows when client has set debugEnabled to YES)
                template<typename ...args_t>
                static void w(std::string const& fmt, args_t&&... args)
                {
                    sendMessage(LogWarning, fmt, std::forward<args_t>(args)...);
                }

                template<typename ...args_t>
                static void e(std::string const& fmt, args_t&&... args)
                {
                    sendMessage(LogError, fmt, std::forward<args_t>(args)...);
                }

                template<typename ...args_t>
                static void d(std::string const& fmt, args_t&&... args)
                {
                    sendMessage(LogDebug, fmt, std::forward<args_t>(args)...);
                }

                template<typename ...args_t>
                static void i(std::string const& fmt, args_t&&... args)
                {
                    sendMessage(LogInfo, fmt, std::forward<args_t>(args)...);
                }

                template<typename ...args_t>
                static void ii(std::string const& fmt, args_t&&... args)
                {
                    sendMessage(LogVerbose, fmt, std::forward<args_t>(args)...);
                }

         private:

            static constexpr const char* tag = "GameAnalytics";
            static std::unique_ptr<GALogger> _instance;
            static GALogger* getInstance();

            GALogger();
            ~GALogger();
            GALogger(const GALogger&) = delete;
            GALogger& operator=(const GALogger&) = delete;

            void sendNotificationMessage(std::string const& message, EGALoggerMessageType type);
            void initializeLog();
           
            std::unique_ptr<LogHandler> customLogHandler;

            // Settings
            bool infoLogEnabled         = false;
            bool infoLogVerboseEnabled  = false;
            bool debugEnabled           = false;

            static void file_output_callback(const zf_log_message *msg, void *arg);
            
            bool            logInitialized;
            std::fstream    logFile;
            int             currentLogCount;
            int             maxLogCount;
        };
    }
}
