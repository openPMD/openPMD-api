#pragma once


#include <memory>

class AbstractFilePosition;
class AbstractIOHandler;

class Writable
{
    template<
            typename T,
            typename T_key
    >
    friend class Container;
    friend class Iteration;
    friend class HDF5IOHandler;
    friend class HDF5IOHandlerImpl;

public:
    Writable()
            : abstractFilePosition{nullptr},
              parent{nullptr},
              dirty{true},
              written{false}
    { }
    virtual ~Writable()
    { }

protected:
    std::shared_ptr< AbstractFilePosition > abstractFilePosition;
    Writable* parent;
    std::shared_ptr< AbstractIOHandler > IOHandler;
    bool dirty;
    bool written;
};
