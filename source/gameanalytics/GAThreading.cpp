//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#include "GAThreading.h"
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include "GALogger.h"
#include <thread>
#include <exception>

namespace gameanalytics
{
    namespace threading
    {
        using namespace std::chrono_literals;

        constexpr std::chrono::milliseconds THREAD_TASK_FREQUENCY = 100ms;

        GAThreading& GAThreading::getInstance()
        {
            static GAThreading instance;
            return instance;
        }

        GAThreading::GAThreading()
        {
            _thread = std::thread(
                [this]()
                { 
                    work();
                }
            );
        }

        GAThreading::~GAThreading()
        {
            _thread.join();
        }

        void GAThreading::queueBlock(Block&& b)
        {
            std::unique_lock<std::mutex> guard(_mutex);
            _blocks.push(std::forward<Block>(b));
        }

        GAThreading::Block GAThreading::getNextBlock()
        {
            std::unique_lock<std::mutex> guard(_mutex);
            Block b = _blocks.front();
            _blocks.pop();
            return b;
        }

        void GAThreading::updateTasks()
        {
            std::unique_lock<std::mutex> guard(_taskMutex);
            for(auto& task : _tasks)
            {   
                task.tick();
            }
        }

        void GAThreading::work()
        {
            using namespace std::chrono_literals;

            while(!_endThread)
            {
                while(!_blocks.empty())
                {
                    Block b = getNextBlock();
                    std::invoke(b);
                }

                updateTasks();

                std::this_thread::sleep_for(THREAD_TASK_FREQUENCY);
            }
        }

        void GAThreading::performTaskOnGAThread(Block b)
        {
            getInstance().queueBlock(std::move(b));
        }

        void GAThreading::endThread()
        {
            getInstance()._endThread = true;
        }

        bool GAThreading::isThreadFinished()
        {
            return getInstance()._endThread;
        }

        GAThreading::ScheduledTask::ScheduledTask(std::chrono::milliseconds freq, Block&& task):
            task(std::forward<Block>(task)),
            frequency(freq)
        {
            _lastCall = std::chrono::high_resolution_clock::now();
        }

        void GAThreading::scheduleTask(std::chrono::milliseconds freq, Block&& task)
        {
            std::unique_lock<std::mutex> guard(_taskMutex);
            _tasks.push_back(ScheduledTask(freq, std::forward<Block>(task)));
        }

        void GAThreading::scheduleTimer(std::chrono::milliseconds freq, Block task)
        {
            return getInstance().scheduleTask(freq, std::move(task));
        }

        bool GAThreading::ScheduledTask::tick()
        {
            auto now = std::chrono::high_resolution_clock::now();
            if(now - _lastCall > frequency)
            {
                _lastCall = now;
                std::invoke(task);
                return true;
            }

            return false;
        }

    }
}
