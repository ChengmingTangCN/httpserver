// 测试定时器模块
#include <timer/TimerManager.h>

#include <sys/epoll.h>
#include <unistd.h>

#include <iostream>

using namespace std;

int main()
{
    TimerManager manager;
    manager.add(1, ::time(nullptr) + 3, [](){
        cout << "3s timer\n";
    });
    manager.add(2, ::time(nullptr) + 4, [](){
        cout << "4s timer\n";
    });


    int epfd = ::epoll_create1(0);
    assert(epfd >= 0);

    manager.adjustTime(1, ::time(nullptr) + 5);

    while (true)
    {
        int timeout = manager.millisecondsToNextExpired();
        cout << "timeout: " << timeout << "ms\n";

        epoll_event epev;
        int ret = ::epoll_wait(epfd, &epev, 1, timeout);
        if (ret < 0)
        {
            ::perror("epoll_wait()");
            return 1;
        }
        manager.checkAndHandleTimer();
    }

    ::close(epfd);

    return 0;
}