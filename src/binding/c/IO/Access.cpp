#include <openPMD/binding/c/IO/Access.h>

#include <openPMD/IO/Access.hpp>

bool openPMD_Access_readOnly(openPMD_Access access)
{
    return openPMD::access::readOnly(openPMD::Access(access));
}

bool openPMD_Access_write(openPMD_Access access)
{
    return openPMD::access::write(openPMD::Access(access));
}

bool openPMD_Access_writeOnly(openPMD_Access access)
{
    return openPMD::access::writeOnly(openPMD::Access(access));
}

bool openPMD_Access_read(openPMD_Access access)
{
    return openPMD::access::read(openPMD::Access(access));
}
