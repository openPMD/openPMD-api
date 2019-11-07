#include "openPMD/IO/JSON/JSONFilePosition.hpp"


namespace openPMD
{
    JSONFilePosition::JSONFilePosition( json::json_pointer ptr):
        id( ptr )
    {}
}
