//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once


#include <functional>
#include <atomic>
#include <vector>
#include <chrono>
#include <memory>
#include <future>
#include <mutex>
#include <queue>
#include <algorithm>

#include "GACommon.h"

namespace gameanalytics
{
    namespace threading
    {
        class GAThreading
        {
            friend class state::GAState;

         public:

            using Block = std::function<void()>;

            static void performTaskOnGAThread(Block taskBlock);

            static void endThread();

            static bool isThreadFinished();

            static void scheduleTimer(std::chrono::milliseconds freq, Block task);
            
            static void flushTasks();

         private:

            struct ScheduledTask
            {
                Block task;
                std::chrono::milliseconds frequency;

                ScheduledTask(std::chrono::milliseconds frequency, Block&& task);
                bool tick(bool force = false);

                private:
                    std::chrono::high_resolution_clock::time_point _lastCall;
            };

            static GAThreading& getInstance();
            
            GAThreading();
            ~GAThreading();

            void work();
            void queueBlock(Block&& block);
            void scheduleTask(std::chrono::milliseconds freq, Block&& task);
            
            void flush();

            Block getNextBlock();
            void  runBlocks();
            void  updateTasks(bool force = false);
            
            std::vector<ScheduledTask> _tasks;
            std::queue<Block> _blocks;
            std::thread       _thread;
            std::mutex        _blockMutex;
            std::mutex        _taskMutex;
            std::atomic<bool> _endThread = false;
            std::atomic<bool> _hasJoined = false;
        };
    }
}
