#pragma once


#include <memory>
#include "IO/AbstractFilePosition.hpp"
#include "IO/AbstractIOHandler.hpp"


class Writable
{
    template<
            typename T,
            typename T_key
    >
    friend class Container;
    friend class Mesh;
    friend class Output;

    friend class HDF5IOHandler;
public:
    Writable()
            : abstractFilePosition{nullptr},
              parent{nullptr},
              dirty{false}
    { }
    virtual ~Writable()
    { }

private:
    std::shared_ptr< AbstractFilePosition > abstractFilePosition;
    std::shared_ptr< Writable > parent;
    std::shared_ptr< AbstractIOHandler > IOHandler;
    bool dirty;
};
