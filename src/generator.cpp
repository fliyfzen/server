#include "generator.h"

namespace Server
{

Generator::Generator(int id) :
    mId(id)
{

}

int Generator::id() const
{
    return mId;
}

uint32_t Generator::generate()
{
    return mersenne();
}

} // namespace Server
