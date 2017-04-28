#pragma once


#include <memory>
#include "IO/AbstractFilePosition.hpp"


class Writable
{
public:
    Writable()
            : abstractFilePosition{nullptr},
              dirty{false}
    { }
    virtual ~Writable()
    { }

    std::shared_ptr< AbstractFilePosition > abstractFilePosition;
    bool dirty;
    Writable* parent;
};
