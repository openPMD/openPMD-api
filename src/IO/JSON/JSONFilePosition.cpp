#include "openPMD/IO/JSON/JSONFilePosition.hpp"


namespace openPMD {

#if openPMD_HAVE_JSON
    JSONFilePosition::JSONFilePosition( json::json_pointer ptr):
        id( ptr )
    {}
#endif
}
