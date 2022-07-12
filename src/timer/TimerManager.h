#ifndef TIMER_TIMER_MANAGER_H
#define TIMER_TIMER_MANAGER_H

#include <cstdint>
#include <cassert>
#include <ctime>
#include <functional>
#include <vector>
#include <unordered_map>

using TimeOutCallBack = std::function<void()>;

struct Timer
{
    int fd;               // 定时器对应的fd
    time_t expire_time;   // 到期时间
    TimeOutCallBack callback;  // 回调函数

    Timer(int fd, time_t expire_time, const TimeOutCallBack &callback)
    : fd(fd),
      expire_time(expire_time),
      callback(callback)
    {}

    bool operator<(const Timer &other)
    {
        return expire_time < other.expire_time;
    }
};

class TimerManager
{
public:

    TimerManager() = default;

    TimerManager(const TimerManager &) = default;

    TimerManager(TimerManager &&) = default;

    TimerManager &operator=(const TimerManager &) = default;

    TimerManager &operator=(TimerManager &&) = default;

    ~TimerManager() = default;

public:

    void add(int fd, time_t expire_time, const TimeOutCallBack &callback);

    void cancel(int fd);

    void adjustTime(int fd, time_t expire_time);

    // 没有计时器返回-1
    int millisecondsToNextExpired() const;

    void checkAndHandleTimer();

private:

    void swap(int i, int j);

    void siftDown(int i);

    void siftUp(int i);

    int leftChild(int node) const
    {
        return node * 2 + 1;
    }

    int rightChild(int node) const
    {
        return node * 2 + 2;
    }

    int parent(int node) const
    {
        return (node - 1) / 2;
    }

    void pop()
    {
        assert(!container_.empty());
        swap(0, container_.size() - 1);
        int fd = container_.back().fd;
        fd_to_pos_.erase(fd);
        container_.back().callback();
        container_.pop_back();
        if (container_.size() > 0)
        {
            siftDown(0);
        }
    }

    std::vector<Timer> container_;

    std::unordered_map<int, int> fd_to_pos_;  // 定时器fd映射到container_下标
};

#endif