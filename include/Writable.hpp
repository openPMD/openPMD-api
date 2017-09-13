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
    friend class HDF5IOHandlerImpl;
    friend class ParallelHDF5IOHandlerImpl;
    friend std::string concrete_h5_file_position(Writable*);

public:
    Writable();
    virtual ~Writable();

protected:
    std::shared_ptr< AbstractFilePosition > abstractFilePosition;
    Writable* parent;
    std::shared_ptr< AbstractIOHandler > IOHandler;
    bool dirty;
    bool written;
};
