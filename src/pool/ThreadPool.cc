#include "ThreadPool.h"

void ThreadPool::threadFunc()
{
    std::function<void()> task;
    while (true)
    {
        // 缩小临界区
        {
            std::unique_lock<std::mutex> guard(lock_);
            while (task_queue_.empty() && isRunning_)
            {
                cond_.wait(guard);
            }
            if (!isRunning_)
            {
                break;
            }
            task = std::move(task_queue_.front());
            task_queue_.pop();
        }
        task();
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> guard(lock_);
        isRunning_ = false;
    }
    cond_.notify_all();
}
