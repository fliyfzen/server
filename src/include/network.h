#ifndef NETWORK_H
#define NETWORK_H

#include "generator.h"

#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>

namespace Server
{

class Network
{
public:
    Network(uint16_t port, int millisec = 500);
    ~Network();

    void startServer();

private:
    uint16_t mPort;
    int mMillisec;
    std::atomic<uint32_t> timestamp;
    std::mutex mutex;

    std::unordered_map<int, bool> generators;

    void sendLastTimestamp(int socket);

    void sendValue(std::unique_ptr<Generator> generator, int socket);

    int setNonblock(int fd);
};

} // namespace Server

#endif // NETWORK_H
