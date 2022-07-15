#ifndef TIMER_TIMER_MANAGER_H
#define TIMER_TIMER_MANAGER_H

#include <cstdint>
#include <cassert>
#include <ctime>
#include <functional>
#include <vector>
#include <unordered_map>
#include <mutex>

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

    TimerManager(const TimerManager &) = delete;

    TimerManager(TimerManager &&) = delete;

    TimerManager &operator=(const TimerManager &) = delete;

    TimerManager &operator=(TimerManager &&) = delete;

    ~TimerManager() = default;

public:

    void add(int fd, time_t expire_time, const TimeOutCallBack &callback);

    void cancel(int fd);

    void adjustTime(int fd, time_t expire_time);

    // 没有计时器返回-1
    int millisecondsToNextExpired() const;

    void checkAndHandleTimer();

private:
    void popAndRunCallBack();

    void swap(int i, int j);

private:
    void siftDown(int i);

    void siftUp(int i);

private:
    int leftChild(int node) const { return node * 2 + 1; }

    int rightChild(int node) const { return node * 2 + 2; }

    int parent(int node) const { return (node - 1) / 2; }

private:
    std::vector<Timer> container_;

    std::unordered_map<int, int> fd_to_pos_;  // 定时器fd映射到container_下标

    mutable std::mutex lock_;
};

#endif