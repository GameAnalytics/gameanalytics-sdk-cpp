//
// GA-SDK-CPP
// Copyright 2018 GameAnalytics C++ SDK. All rights reserved.
//

#pragma once

#if !defined(__APPLE__)

#include <thread>

namespace gameanalytics::threading
{
    using GAThread = std::thread;
}

#else

#include <functional>
#include <pthread.h>

namespace gameanalytics::threading
{
    // std::thread frees its TLS block inside libc++.dylib.
    class GAThread
    {
     public:
        GAThread() = default;

        explicit GAThread(std::function<void()> fn)
        {
            auto* heapFn = new std::function<void()>(std::move(fn));
            _joinable = pthread_create(&_handle, nullptr, &GAThread::entry, heapFn) == 0;
            if (!_joinable)
            {
                delete heapFn;
            }
        }

        ~GAThread()
        {
            join();
        }

        GAThread(GAThread&& other) noexcept
        {
            *this = std::move(other);
        }

        GAThread& operator=(GAThread&& other) noexcept
        {
            join();
            _handle   = other._handle;
            _joinable = other._joinable;
            other._joinable = false;
            return *this;
        }

        GAThread(GAThread const&) = delete;
        GAThread& operator=(GAThread const&) = delete;

        bool joinable() const { return _joinable; }

        void join()
        {
            if (_joinable)
            {
                _joinable = false;
                pthread_join(_handle, nullptr);
            }
        }

     private:
        static void* entry(void* arg)
        {
            auto* fn = static_cast<std::function<void()>*>(arg);
            (*fn)();
            delete fn;
            return nullptr;
        }

        pthread_t _handle   = {};
        bool      _joinable = false;
    };
}

#endif
