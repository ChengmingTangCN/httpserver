#ifndef HTTPSERVER_SERVER_EPOLLER_H
#define HTTPSERVER_SERVER_EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>

#include <vector>

class Epoller
{
public:
    explicit Epoller(int max_ep_events = 1024)
      : epfd_(::epoll_create1(0)),
        ep_events_ret_(1024) {}

    Epoller(const Epoller &) = delete;

    Epoller(Epoller &&) = default;

    Epoller &operator=(const Epoller &) = delete;

    Epoller &operator=(Epoller &&) = default;

    ~Epoller()
    {
        ::close(epfd_);
    }

public:
    bool addFd(int fd, uint32_t events);

    bool modFd(int fd, uint32_t events);

    bool delFd(int fd);

    int wait(int timeoutMs);

    int getFdOf(int idx) const;

    uint32_t getEventsOf(int idx) const;

private:
    int epfd_;

    std::vector<struct epoll_event> ep_events_ret_;
};

#endif
