#include <IO/AbstractIOHandler.hpp>
#include <backend/Writable.hpp>

Writable::Writable()
        : abstractFilePosition{nullptr},
          parent{nullptr},
          IOHandler{nullptr},
          dirty{true},
          written{false}
{ }

Writable::~Writable()
{ }

