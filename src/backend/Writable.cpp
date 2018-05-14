#include <openPMD/IO/AbstractIOHandler.hpp>
#include <openPMD/backend/Writable.hpp>


namespace openPMD
{
Writable::Writable(Attributable* a)
        : abstractFilePosition{nullptr},
          IOHandler{nullptr},
          attributable{a},
          parent{nullptr},
          dirty{true},
          written{false}
{ }

Writable::~Writable()
{ }
} // openPMD
