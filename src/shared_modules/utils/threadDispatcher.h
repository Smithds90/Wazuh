/*
 * Wazuh shared modules utils
 * Copyright (C) 2015-2020, Wazuh Inc.
 * July 14, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef THREAD_DISPATCHER_H
#define THREAD_DISPATCHER_H
#include <vector>
#include <thread>
#include <atomic>
#include <future>
#include <functional>
#include "threadSafeQueue.h"
namespace Utils
{
    // *
    //  * @brief Minimal Dispatcher interface
    //  * @details Handle dispatching of messages of type Type
    //  * to be processed by calling Functor.
    //  * 
    //  * @tparam Type Messages types.
    //  * @tparam Functor Entity that processes the messages.
     
    // template <typename Type, typename Functor>
    // class DispatcherInterface
    // {
    // public:
    //  /**
    //   * @brief Ctor
    //   * 
    //   * @param functor Callable entity.
    //   * @param int Maximun number of threads to be used by the dispatcher.
    //   */
    //  DispatcherInterface(Functor functor, const unsigned int numberOfThreads);
    //  *
    //   * @brief Pushes a message to be processed by the functor.
    //   * @details The implementation decides whether the processing is sync or async.
    //   * 
    //   * @param data Message value.
         
    //  void push(const Type& data);
    //  /**
    //   * @brief Rundowns the pending messages until reaches 0.
    //   * @details It should be a blocking call.
    //   */     
    //  void rundown();
    //  /**
    //   * @brief Cancels the dispatching.
    //   */
    //  void cancel();
    // };

    template
    <
    typename Type,
    typename Functor
    >
    class AsyncDispatcher
    {
    public:
        AsyncDispatcher(Functor functor, const unsigned int numberOfThreads = std::thread::hardware_concurrency())
        : m_functor{ functor }
        , m_running{ true }
        , m_numberOfThreads{ numberOfThreads }
        {
            m_threads.reserve(m_numberOfThreads);
            for (unsigned int i = 0; i < m_numberOfThreads; ++i)
            {
                m_threads.push_back(std::thread{ &AsyncDispatcher<Type, Functor>::dispatch, this });
            }
        }
        AsyncDispatcher& operator=(const AsyncDispatcher&) = delete;
        AsyncDispatcher(AsyncDispatcher& other) = delete;
        ~AsyncDispatcher()
        {
            cancel();
        }

        void push(const Type& value)
        {
            if (m_running)
            {
                m_queue.push
                (
                    [value, this]()
                    {
                        this->m_functor(value);
                    }
                );
            }
        }

        void rundown()
        {
            if (m_running)
            {
                std::promise<void> promise;
                auto fut { promise.get_future() };
                m_queue.push
                (
                    [&promise]()
                    {
                        promise.set_value();
                    }
                );
                fut.wait();
                cancel();
            }
        }
        void cancel()
        {
            m_queue.cancel();
            joinThreads();
        }

        bool cancelled() const
        {
            return !m_running;
        }
        unsigned int numberOfThreads() const
        {
            return m_numberOfThreads;
        }
        size_t size() const
        {
            return m_queue.size();
        }

    private:
        void dispatch()
        {
            while(m_running)
            {
                std::function<void()> fnc;
                if(m_queue.pop(fnc))
                {
                    fnc();
                }
            }
        }
        void joinThreads()
        {
            if (m_running)
            {
                m_running = false;
                for (auto& thread : m_threads)
                {
                    if (thread.joinable())
                    {
                        thread.join();
                    }
                }
            }
        }
        Functor m_functor;
        SafeQueue<std::function<void()>> m_queue;
        std::vector<std::thread> m_threads;
        std::atomic_bool m_running;
        const unsigned int m_numberOfThreads;
    };

    template <typename Input, typename Functor>
    class SyncDispatcher
    {
    public:
        SyncDispatcher(Functor functor, const unsigned int /*numberOfThreads = 0*/)
        : m_functor{functor}
        {
        }
        void push(const Input& data)
        {
            m_functor(data);
        }
        size_t size() const {return 0;}
        void rundown(){}
        void cancel(){}
        ~SyncDispatcher() = default;
    private:
        Functor m_functor;
    };
}//namespace Utils
#endif //THREAD_DISPATCHER_H