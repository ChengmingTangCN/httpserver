#include <timer/TimerManager.h>
#include <cassert>

using std::unique_lock;
using std::mutex;

void TimerManager::add(int fd, time_t expire_time, const TimeOutCallBack &callback)
{
    unique_lock<mutex> guard(lock_);
    assert(fd_to_pos_.count(fd) == 0);
    container_.emplace_back(fd, expire_time, callback);
    fd_to_pos_[fd] = container_.size() - 1;
    siftUp(fd_to_pos_[fd]);
}

void TimerManager::cancel(int fd)
{
    unique_lock<mutex> guard(lock_);
    if (fd_to_pos_.count(fd) == 0)
    {
        return;
    }
    int pos = fd_to_pos_[fd];
    swap(pos, container_.size() - 1);
    container_.pop_back();
    fd_to_pos_.erase(fd);
    if (pos < container_.size())
    {
        siftDown(pos);
    }
}

void TimerManager::adjustTime(int fd, time_t expire_time)
{
    unique_lock<mutex> guard(lock_);
    assert(fd_to_pos_.count(fd) == 1);
    container_[fd_to_pos_[fd]].expire_time = expire_time;
    // siftUp(fd_to_pos_[fd]);
    siftDown(fd_to_pos_[fd]);
}

int TimerManager::millisecondsToNextExpired() const
{
    unique_lock<mutex> guard(lock_);
    int res = -1;
    if (!container_.empty())
    {
        time_t now = ::time(nullptr);
        res = container_.front().expire_time - now;
        if (res < 0)
        {
            res = 0;
        }
        else
        {
            res *= 1000;
        }
    }
    return res;
}

void TimerManager::checkAndHandleTimer()
{
    unique_lock<mutex> guard(lock_);
    time_t now = ::time(nullptr);
    while (!container_.empty())
    {
        if (container_.front().expire_time > now)
        {
            break;
        }
        popAndRunCallBack();
    }
}

void TimerManager::swap(int i, int j)
{
    assert(i < container_.size());
    assert(j < container_.size());
    using std::swap;
    // int fd_i = container_[i].fd, fd_j = container_[j].fd;
    swap(container_[i], container_[j]);
    // fd_to_pos_[fd_i] = j;
    // fd_to_pos_[fd_j] = i;
    fd_to_pos_[container_[i].fd] = i;
    fd_to_pos_[container_[j].fd] = j;
}

void TimerManager::popAndRunCallBack()
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

void TimerManager::siftDown(int i)
{
    assert(i < container_.size());
    int min_pos = i;

    int left_child = leftChild(i);
    if (left_child < container_.size() && container_[left_child] < container_[min_pos])
    {
        min_pos = left_child;
    }

    int right_child = rightChild(i);
    if (right_child < container_.size() && container_[right_child] < container_[min_pos])
    {
        min_pos = right_child;
    }

    if (min_pos != i)
    {
        swap(min_pos, i);
        siftDown(min_pos);
    }
}

void TimerManager::siftUp(int i)
{
    assert(i < container_.size());

    int parent_node = parent(i);
    if (container_[i] < container_[parent_node])
    {
        swap(i, parent_node);
        siftUp(parent_node);
    }
}
