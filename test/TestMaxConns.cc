#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <cstdlib>

int main()
{

    struct sockaddr_in server_addr;
    ::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = ::htons(3333);
    int ret = ::inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    assert(ret == 1);

    int cnt = 0;
    while (true)
    {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
            ::perror("::socket()");
            return 1;
        }
        std::printf("fd: %d\n", fd);
        ret = ::connect(fd, (struct sockaddr *)(&server_addr), sizeof(server_addr));
        if (ret < 0)
        {
            ::perror("connect()");
            return 1;
        }

        const char *msg = "GET /index.html HTTP/1.1\r\nHost: 127.0.0.1:3333\r\nConnection: keep-alive\r\n\r\n";
        ssize_t send_len = ::send(fd, msg, strlen(msg), 0);
        assert(send_len >= 0);
        if (++cnt > 300)
        {
            ::sleep(1);
        }
    }


    return 0;
}