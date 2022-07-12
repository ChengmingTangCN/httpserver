#include <server/Server.h>

int main()
{
    Server server(3333, 5);

    server.run();

    return 0;
}
