#include "openPMD/Error.hpp"

#include <pybind11/pybind11.h>

namespace py = pybind11;
using namespace openPMD;

void init_Error( py::module & m )
{
    auto & baseError = py::register_exception< Error >( m, "Error" );
    py::register_exception< error::OperationUnsupportedInBackend >(
        m, "ErrorOperationUnsupportedInBackend", baseError );
    py::register_exception< error::WrongAPIUsage >(
        m, "ErrorWrongAPIUsage", baseError );

#ifndef NDEBUG
    m.def( "test_throw", []( std::string description ) {
        throw error::OperationUnsupportedInBackend( "json", description );
    } );
#endif
}
