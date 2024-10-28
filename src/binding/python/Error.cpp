/* Copyright 2019-2023 The openPMD Community
 *
 * This header is used to centrally define classes that shall not violate the
 * C++ one-definition-rule (ODR) for various Python translation units.
 *
 * Authors: Franz Poeschel, Axel Huebl
 * License: LGPL-3.0-or-later
 */
#include "openPMD/Error.hpp"

#include "openPMD/binding/python/Common.hpp"

void init_Error(py::module &m)
{
    auto &baseError = py::register_exception<Error>(m, "Error");
    py::register_exception<error::OperationUnsupportedInBackend>(
        m, "ErrorOperationUnsupportedInBackend", baseError);
    py::register_exception<error::WrongAPIUsage>(
        m, "ErrorWrongAPIUsage", baseError);
    py::register_exception<error::BackendConfigSchema>(
        m, "ErrorBackendConfigSchema", baseError);
    py::register_exception<error::Internal>(m, "ErrorInternal", baseError);
    py::register_exception<error::NoSuchAttribute>(
        m, "ErrorNoSuchAttribute", baseError);
    py::register_exception<error::IllegalInOpenPMDStandard>(
        m, "ErrorIllegalInOpenPMDStandard", baseError);

#ifndef NDEBUG
    m.def("test_throw", [](std::string description) {
        throw error::OperationUnsupportedInBackend("json", description);
    });
#endif
}
