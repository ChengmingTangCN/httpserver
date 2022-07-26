#ifndef HTTPSERVER_LOGGER_BLOCK_QUEUE
#define HTTPSERVER_LOGGER_BLOCK_QUEUE

#include <mutex>
#include <condition_variable>
#include <queue>

template <typename T>
class BlockQueue
{
public:
    explicit BlockQueue(int capacity = 1024)
      : queue_(),
        capacity_(capacity),
        stopped_(false),
        lock_(),
        not_empty_cond_(),
        not_full_cond_()
    {}

    BlockQueue(const BlockQueue &) = delete;

    BlockQueue(BlockQueue &&) = delete;

    BlockQueue &operator=(const BlockQueue &) = delete;

    BlockQueue &operator=(BlockQueue &&) = delete;

    ~BlockQueue() = default;

public:
    void put(const T &other)
    {
        add(other);
    }

    void put(T &&other)
    {
        add(other);
    }

    void take(T &t);

    void stop()
    {
        {
            std::lock_guard<std::mutex> guard(lock_);
            stopped_ = true;
        }
        not_empty_cond_.notify_all();
        not_full_cond_.notify_all();
    }

    bool stopped() const
    {
        std::lock_guard<std::mutex> guard(lock_);
        return stopped_;
    }

    int size() const
    {
        std::lock_guard<std::mutex> guard(lock_);
        return queue_.size();
    }

    int capacity() const
    {
        return capacity_;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> guard(lock_);
        return unlockedEmpty();
    }

    bool full() const
    {
        std::lock_guard<std::mutex> guard(lock_);
        return unlockedFull();
    }

private:
    template<typename U>
    void add(U &&arg);
    
    bool unlockedEmpty() const
    {
        return queue_.empty();
    }

    bool unlockedFull() const
    {
        return queue_.size() == capacity_;
    }

private:
    std::queue<T> queue_;      
    int capacity_;                            // 队列的容量
    bool stopped_;                            // 队列是否停止工作
    mutable std::mutex lock_;                 // 锁住队列
    std::condition_variable not_empty_cond_;  // 队列不为空
    std::condition_variable not_full_cond_;   // 队列没满
};

template<typename T>
void BlockQueue<T>::take(T &t)
{
    std::unique_lock<std::mutex> guard(lock_);
    not_empty_cond_.wait(guard, [this]()
    {
        return stopped_ || !unlockedEmpty();
    });
    if (stopped_)
    {
        return;
    }
    t = std::move(queue_.front());
    queue_.pop();
    not_full_cond_.notify_one();
}

template<typename T>
template<typename U>
void BlockQueue<T>::add(U &&arg)
{
    std::unique_lock<std::mutex> guard(lock_);
    not_full_cond_.wait(guard, [this]()
    {
        return stopped_ || !unlockedFull();
    });
    if (stopped_)
    {
        return;
    }
    queue_.emplace(std::forward<U>(arg));
    not_empty_cond_.notify_one();
}

#endif
