#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>

int main()
{

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = ::htons(3333);
    ::inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    char buffer[1024];
    int ret = ::connect(sock, (struct sockaddr *)(&server_addr), sizeof(server_addr));
    assert(ret >= 0);

    const char *msg = "GET /index.html HTTP/1.1\r\n"
                      "Host: 127.0.0.1\r\n"
                      "\r\n";

    ssize_t send_len = ::send(sock, msg, std::strlen(msg), 0);
    if (send_len < std::strlen(msg))
    {
        fprintf(stderr, "send_len to small\n");
        return 1;
    }

    while (true)
    {
        ssize_t recv_len = ::recv(sock, buffer, 1023, 0);
        if (recv_len < 0)
        {
            std::perror("::recv()");
            return 1;
        }
        if (recv_len == 0)
        {
            break;
        }
        buffer[recv_len] = '\0';
        printf("%s\n", buffer);
    }

    ::close(sock);

    return 0;
}