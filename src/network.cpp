#include "network.h"
#include "generator.h"

#include <chrono>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_EVENTS 32

namespace Server
{

Network::Network(uint16_t port, int millisec) :
    mPort(port),
    mMillisec(millisec)
{

}

Network::~Network()
{

}

void Network::sendLastTimestamp(int socket)
{
    std::string buffer;
    char f = static_cast<char>(0xAA);
    char s = static_cast<char>(0x01);
    uint32_t value = timestamp;
    buffer.append(reinterpret_cast<char*>(&f), sizeof(f));
    buffer.append(reinterpret_cast<char*>(&s), sizeof(s));
    buffer.append(reinterpret_cast<char*>(&value), sizeof(value));
    send(socket, &buffer, sizeof(buffer), MSG_NOSIGNAL);
}

void Network::sendValue(std::unique_ptr<Generator> generator, int socket)
{
    bool stopGenerate = false;
    while(!stopGenerate)
    {
        uint32_t value = generator->generate();
        timestamp = static_cast<uint32_t>(std::chrono::seconds(std::time(nullptr)).count());
        send(socket, &value, sizeof(value), MSG_NOSIGNAL);

        std::this_thread::sleep_for(std::chrono::milliseconds(mMillisec));

        std::lock_guard<std::mutex> lock(mutex);
        stopGenerate = generators.find(generator->id())->second;
    }
    std::lock_guard<std::mutex> lock(mutex);
    generators.erase(generators.find(generator->id()));
}

void Network::startServer()
{
    auto masterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(mPort);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(masterSocket, (struct sockaddr*)(&sa), sizeof(sa));

    setNonblock(masterSocket);

    listen(masterSocket, SOMAXCONN);

    int epooll = epoll_create1(0);

    struct epoll_event event;
    event.data.fd = masterSocket;
    event.events = EPOLLIN;
    epoll_ctl(epooll, EPOLL_CTL_ADD, masterSocket, &event);

    while(true)
    {
        struct epoll_event events[MAX_EVENTS];
        int n = epoll_wait(epooll, events, MAX_EVENTS, -1);
        for(unsigned i = 0; i < n; i++)
        {
            if(events[i].data.fd == masterSocket)
            {
                auto slaveSocket = accept(masterSocket, nullptr, nullptr);
                setNonblock(slaveSocket);

                struct epoll_event event;
                event.data.fd = slaveSocket;
                event.events = EPOLLIN;
                epoll_ctl(epooll, EPOLL_CTL_ADD, slaveSocket, &event);

                struct sysinfo info;
                sysinfo(&info);
                struct tm * timeinfo;
                timeinfo = gmtime(&info.uptime);
                char uptime[15];
                strftime(uptime, 15, "%T", timeinfo);
                send(slaveSocket, uptime, sizeof(uptime), MSG_NOSIGNAL);
            }
            else
            {
                static char buffer[6] = {0};
                auto result = recv(events[i].data.fd, buffer, 6, MSG_NOSIGNAL);

                if((result == 0) && (errno != EAGAIN))
                {
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        generators[events[i].data.fd] = true;
                    }
                    shutdown(events[i].data.fd, SHUT_RDWR);
                    close(events[i].data.fd);

                    std::lock_guard<std::mutex> lock(mutex);
                }
                else if(result > 0)
                {
                    if((buffer[0] == static_cast<char>(0xFF)) && (buffer[1] == static_cast<char>(0x01)))
                    {
                        std::string str(buffer+2, 4);
                        int idGen = std::atoi(str.c_str());

                        auto gen = std::make_unique<Generator>(idGen);

                        {
                            std::lock_guard<std::mutex> lock(mutex);
                            generators.insert({gen->id(), false});
                        }

                        auto socket = events[i].data.fd;

                        std::thread thread(&Network::sendValue, this, std::move(gen), socket);

                        thread.detach();
                    }
                    else if((static_cast<int>(buffer[0]) == 0xFF) && (static_cast<int>(buffer[1]) == 0x02))
                    {
                        sendLastTimestamp(events[i].data.fd);
                    }
                }
            }
        }
    }
}

int Network::setNonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if(-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIONBIO, &flags);
#endif
}

} // namespace Server
