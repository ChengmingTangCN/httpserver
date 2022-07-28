#include <server/Server.h>
#include <logger/AsyncLogger.h>

int main()
{
    Server server(3333,
                  6,
                  false, LogLevel::DEBUG, "./log",
                  ".log", 5000, 1024);

    server.run();
    return 0;
}
