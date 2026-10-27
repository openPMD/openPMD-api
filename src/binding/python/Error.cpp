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

void init_Error(nb::module_ &m)
{
    // nanobind tries exception translators in reverse order of registration,
    // so the base class must be registered first
    nb::exception<Error> baseError(m, "Error");
    nb::exception<error::OperationUnsupportedInBackend>(
        m, "ErrorOperationUnsupportedInBackend", baseError);
    nb::exception<error::WrongAPIUsage>(m, "ErrorWrongAPIUsage", baseError);
    nb::exception<error::BackendConfigSchema>(
        m, "ErrorBackendConfigSchema", baseError);
    nb::exception<error::Internal>(m, "ErrorInternal", baseError);
    nb::exception<error::NoSuchAttribute>(m, "ErrorNoSuchAttribute", baseError);
    nb::exception<error::IllegalInOpenPMDStandard>(
        m, "ErrorIllegalInOpenPMDStandard", baseError);

#ifndef NDEBUG
    m.def("test_throw", [](std::string description) {
        throw error::OperationUnsupportedInBackend("json", description);
    });
#endif
}
