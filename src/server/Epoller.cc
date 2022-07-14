#include "Epoller.h"

#include <cassert>


bool Epoller::addFd(int fd, uint32_t events)
{
    struct epoll_event ep_event = {0};
    ep_event.data.fd = fd;
    ep_event.events = events;
    int ret = ::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ep_event);
    return ret == 0;
}

bool Epoller::modFd(int fd, uint32_t events)
{
    assert(fd >= 0);
    struct epoll_event ep_event = {0};
    ep_event.data.fd = fd;
    ep_event.events = events;
    int ret = ::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ep_event);
    return ret == 0;
}

bool Epoller::delFd(int fd)
{
    assert(fd >= 0);
    int ret = ::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
    return ret == 0;
}

int Epoller::wait(int timeoutMs)
{
    int res = ::epoll_wait(epfd_, ep_events_ret_.data(),
                           ep_events_ret_.size(), timeoutMs);
    return res;
}

int Epoller::getFdOf(int idx) const
{
    assert(idx >= 0 && idx <= ep_events_ret_.size());
    return ep_events_ret_[idx].data.fd;
}

uint32_t Epoller::getEventsOf(int idx) const
{
    assert(idx >= 0 && idx <= ep_events_ret_.size());
    return ep_events_ret_[idx].events;
}
