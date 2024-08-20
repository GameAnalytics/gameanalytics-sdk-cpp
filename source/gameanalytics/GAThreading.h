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

namespace gameanalytics
{
    namespace threading
    {
        class GAThreading
        {
         public:

            using Block = std::function<void()>;

            static void performTaskOnGAThread(Block taskBlock);

            static void endThread();

            static bool isThreadFinished();

            static void scheduleTimer(std::chrono::milliseconds freq, Block task);

         private:

            struct ScheduledTask
            {
                Block task;
                std::chrono::milliseconds frequency;

                ScheduledTask(std::chrono::milliseconds frequency, Block&& task);
                bool tick();

                private:
                    std::chrono::high_resolution_clock::time_point _lastCall;
            };

            static GAThreading& getInstance();

            GAThreading();
            ~GAThreading();

            void work();
            void queueBlock(Block&& block);
            void scheduleTask(std::chrono::milliseconds freq, Block&& task);

            Block getNextBlock();
            void  updateTasks();
            
            std::vector<ScheduledTask> _tasks;
            std::queue<Block> _blocks;
            std::thread       _thread;
            std::mutex        _mutex;
            std::mutex        _taskMutex;
            std::atomic<bool> _endThread = false;
        };
    }
}
