#include <openPMD/IO/AbstractIOHandler.hpp>
#include <openPMD/backend/Writable.hpp>


Writable::Writable()
        : abstractFilePosition{nullptr},
          parent{nullptr},
          IOHandler{nullptr},
          dirty{true},
          written{false}
{ }

Writable::~Writable()
{ }

