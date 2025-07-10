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
    case Access::CREATE_LINEAR:
        o << std::string("CREATE_LINEAR");
        break;
    case Access::APPEND_LINEAR:
        o << std::string("APPEND_LINEAR");
        break;
    case Access::CREATE_RANDOM_ACCESS:
        o << std::string("CREATE_RANDOM_ACCESS");
        break;
    case Access::APPEND_RANDOM_ACCESS:
        o << std::string("APPEND_RANDOM_ACCESS");
        break;
    }
    return o;
}
} // namespace openPMD
