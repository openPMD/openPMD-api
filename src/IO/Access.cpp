#include "openPMD/IO/Access.hpp"

#include <iostream>
#include <string>

namespace openPMD
{
std::ostream &operator<<(std::ostream &o, Access const &a)
{
    switch (a)
    {
    case Access::READ_RANDOM_ACCESS:
        o << std::string("READ_RANDOM_ACCESS");
        break;
    case Access::READ_LINEAR:
        o << std::string("READ_LINEAR");
        break;
    case Access::READ_WRITE:
        o << std::string("READ_WRITE");
        break;
    case Access::CREATE:
        o << std::string("CREATE");
        break;
    case Access::APPEND:
        o << std::string("APPEND");
        break;
    }
    return o;
}
} // namespace openPMD
