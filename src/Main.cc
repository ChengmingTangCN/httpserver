#include <server/Server.h>

int main()
{
    Server server(3333, 6);

    server.run();

    return 0;
}
