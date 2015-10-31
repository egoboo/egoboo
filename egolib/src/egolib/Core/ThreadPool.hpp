#pragma once

#include <IdLib/IdLib.hpp>
//#include <vector>
//#include <queue>
//#include <memory>
//#include <thread>
//#include <mutex>
//#include <condition_variable>
//#include <future>
//#include <functional>
//#include <stdexcept>

class ThreadPool /*: public Ego::Core::Singleton<ThreadPool>*/
{
public:
    ThreadPool(size_t threads) :
        _threads(),
        _tasks(),
        _queueMutex(),
        _condition(),
        _terminateRequested(false),
        _semaphore(),
        _activeThreads(0)
    {
        for(size_t i = 0; i < threads; ++i) {            
            _threads.emplace_back( [this] {
                while(true)
                {
                    std::function<void()> task;

                    //Conditional wait for next task to be submitted
                    {
                        std::unique_lock<std::mutex> lock(_queueMutex);
                        _condition.wait(lock, [this]{ return _terminateRequested || !_tasks.empty(); });

                        if(_terminateRequested && _tasks.empty()) {
                            return;
                        }

                        //Fetch next task
                        _activeThreads++;
                        task = std::move(_tasks.front());
                        _tasks.pop();
                    }

                    //Execute assigned task
                    task();
                    _activeThreads--;
                    _semaphore.notify_all();
                }
            });
        }
    }

    ~ThreadPool()
    {
        //Terminate all executing threads
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _terminateRequested = true;
        }
        
        //Wake em up
        _condition.notify_all();

        //Wait for all threads to finish up
        for(std::thread &worker: _threads) {
            worker.join();
        }
    }

    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
            
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(_queueMutex);

            // don't allow new tasks after stopping the pool
            if(_terminateRequested) {
                throw std::runtime_error("submitted task on stopped ThreadPool");
            }

            _tasks.emplace([task](){ (*task)(); });
        }
        _condition.notify_one();
        return res;
    }

    void wait()
    {
        if(_tasks.empty() && _activeThreads == 0) {
            return;
        }

        std::unique_lock<std::mutex> lock(_queueMutex);
        _semaphore.wait(lock, [this]{ return _activeThreads == 0 && _tasks.empty(); });
    }

/*
protected:

    // Befriend with singleton to grant access to ThreadPool::ThreadPool and ThreadPool::~ThreadPool.
    using TheSingleton = Ego::Core::Singleton<ThreadPool>;
    friend TheSingleton;

    ThreadPool();
    ~ThreadPool();
*/

private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> _threads;

    // the task queue
    std::queue<std::function<void()>> _tasks;
    
    // synchronization
    std::mutex _queueMutex;
    std::condition_variable _condition;
    bool _terminateRequested;

    std::condition_variable _semaphore;
    std::atomic<size_t> _activeThreads;
};
