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
#include "GAState.h"

namespace gameanalytics
{
    namespace threading
    {
        using namespace std::chrono_literals;

        constexpr std::chrono::milliseconds THREAD_TASK_FREQUENCY = 100ms;

        GAThreading& GAThreading::getInstance()
        {
            return state::GAState::getInstance()._gaThread;
        }
    
        void GAThreading::flushTasks()
        {
            getInstance().flush();
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
            flush();
        }
    
        void GAThreading::flush()
        {
            if(!_hasJoined)
            {
                _endThread = true;
                _hasJoined = true;
                _thread.join();
                
                // if there are any other tasks queued, flush them
                runBlocks();
                updateTasks(true);
            }
        }

        void GAThreading::runBlocks()
        {
            while(!_blocks.empty())
            {
                Block b = getNextBlock();

                try
                {
                    std::invoke(b);
                }
                catch(const std::exception& e)
                {
                    logging::GALogger::e("Failed to run block on ga thread: %s", e.what());
                }
            }
        }

        void GAThreading::queueBlock(Block&& b)
        {
            std::unique_lock<std::mutex> guard(_blockMutex);
            _blocks.push(std::forward<Block>(b));
        }

        GAThreading::Block GAThreading::getNextBlock()
        {
            std::unique_lock<std::mutex> guard(_blockMutex);
            Block b = _blocks.front();
            _blocks.pop();
            return b;
        }

        void GAThreading::updateTasks(bool force)
        {
            std::unique_lock<std::mutex> guard(_taskMutex);
            for(auto& task : _tasks)
            {   
                task.tick(force);
            }
        }

        void GAThreading::work()
        {
            while(!_endThread)
            {
                runBlocks();
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

        bool GAThreading::ScheduledTask::tick(bool force)
        {
            auto now = std::chrono::high_resolution_clock::now();
            if(((now - _lastCall) >= frequency) || force)
            {
                _lastCall = now;
                
                try
                {
                    std::invoke(task);
                }
                catch(const std::exception& e)
                {
                    logging::GALogger::e("Failed to run scheduled task on ga thread: %s", e.what());
                    return false;
                }

                return true;
            }

            return false;
        }

    }
}
