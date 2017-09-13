#include <IO/AbstractIOHandler.hpp>
#include <Writable.hpp>

Writable::Writable()
        : abstractFilePosition{nullptr},
          parent{nullptr},
          IOHandler{new NONEIOHandler("", AccessType::READ_WRITE)},
          dirty{true},
          written{false}
{ }

Writable::~Writable()
{ }

