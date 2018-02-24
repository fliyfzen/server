#ifndef GENERATOR_H
#define GENERATOR_H

#include <random>

namespace Server
{

class Generator
{
public:
    Generator(int id);

    int id() const;
    uint32_t generate();

private:
    int mId;
    std::mt19937 mersenne{std::random_device()()};
};

} // namespace Server

#endif // GENERATOR_H
