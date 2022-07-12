#ifndef HTTPSERVER_POOL_THREAD_POOL_H
#define HTTPSERVER_POOL_THREAD_POOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <thread>

class ThreadPool
{

public:
    explicit ThreadPool(int threads_num)
    : isRunning_(true),
      lock_(),
      cond_(),
      task_queue_()
    {
        for (int i = 0; i < threads_num; ++i)
        {
            std::thread t(&ThreadPool::threadFunc, this);
            t.detach();
        }
    }

    ThreadPool(const ThreadPool &) = delete;

    ThreadPool(ThreadPool &&) = delete;

    ThreadPool &operator=(const ThreadPool &) = delete;

    ThreadPool &operator=(ThreadPool &&) = delete;

    ~ThreadPool();

    template <typename Task>
    void addTask(Task &&task)
    {
        {
            std::unique_lock<std::mutex> guard(lock_);
            task_queue_.emplace(std::forward<Task>(task));
        }
        cond_.notify_one();
    }

    void threadFunc();

private:

    bool isRunning_;   // 是否运行
    std::mutex lock_;  // 互斥量
    std::condition_variable cond_;  // 条件变量
    std::queue<std::function<void()>> task_queue_;  // 任务队列
};

#endif
