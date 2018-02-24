#include "network.h"

#include <iostream>

int main(int, char *argv[])
{
    char* delay = getenv("DELAY");

    uint16_t port = 0;

    port = std::atoi(argv[1]);

    if(!port)
    {
        std::cout << "No set port";
        return 1;
    }

    Server::Network net(port, std::atoi(delay));

    net.startServer();

    return 0;
}
