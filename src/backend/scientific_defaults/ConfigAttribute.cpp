
#include "openPMD/backend/scientific_defaults/ConfigAttribute.hpp"

#include "openPMD/Datatype.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/ThrowError.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Writable.hpp"
#include <functional>

#include <iostream>
#include <optional>
#include <utility>

namespace openPMD::internal
{
ConfigAttribute::ConfigAttribute(
    Attributable &attributable_in, char const *attrName_in)
    : attributable(attributable_in), attrName(attrName_in)
{}

auto ConfigAttribute::withReader(
    std::deque<Datatype> eligibleDatatypes,
    std::optional<std::shared_ptr<ProcessParsedAttribute>> processAttribute)
    -> ConfigAttribute &
{
    this->attributeReaders.emplace_back(
        std::move(eligibleDatatypes), std::move(processAttribute));
    return *this;
}

void ConfigAttribute::write()
{
    if (this->attributable.containsAttribute(this->attrName) ||
        !this->initDefaultAttribute.has_value())
    {
        return;
    }
    this->initDefaultAttribute.operator*()(this->attributable);
}

void ConfigAttribute::read()
{
    if (attributeReaders.empty())
    {
        // No readers emplaced for this attribute
        return;
    }

    Parameter<Operation::READ_ATT> aRead;
    aRead.name = this->attrName;
    auto IOHandler = this->attributable.IOHandler();
    IOHandler->enqueue(IOTask(&this->attributable, aRead));
    try
    {
        IOHandler->flush(defaultFlushParams);
    }
    catch (error::ReadError const &e)
    {
        std::cerr << "Could not read expected attribute '" << this->attrName
                  << "' in '" << this->attributable.myPath().openPMDPath()
                  << ".";
        if (e.reason == error::Reason::NotFound &&
            this->initDefaultAttribute.has_value())
        {
            std::cerr << " Will initialize it with a default value.";
            this->initDefaultAttribute.operator*()(this->attributable);
        }
        std::cerr << " Original error: " << e.what() << std::endl;
        return;
    }
    Attribute attribute(Attribute::from_any, std::move(*aRead.m_resource));

    AttributeReadResult res = attribute_read_result::TypeUnmatched{};
    for (auto &attributeReader : attributeReaders)
    {
        // run the next attribute reader if there has been no match yet
        if (auto *not_matched =
                std::get_if<attribute_read_result::TypeUnmatched>(&res))
        {
            res = attributeReader(
                this->attributable,
                this->attrName,
                attribute,
                std::move(not_matched->expectedDatatypes));
        }
        else
        {
            break;
        }
    }

    /*
     * If any trouble has been had, print a warning. Don't throw errors or try
     * to correct anything at this place. Instead return parsed data to the user
     * as it was found on disk, even if it is not standard-compliant.
     */
    auto dt = attribute.dtype;
    std::visit(
        auxiliary::overloaded{
            [&](attribute_read_result::TypeUnmatched &&type_unmatched) {
                std::cerr << "Unexpected type '" << dt << "' for attribute '"
                          << this->attrName << "' in '"
                          << this->attributable.myPath().openPMDPath()
                          << "' with value '";
                write_to_stderr(attribute) << "'. Expected one of ";
                auxiliary::write_vec_to_stream(
                    std::cerr, type_unmatched.expectedDatatypes)
                    << " or convertible to such a type." << std::endl;
            },
            [&](error::ReadError const &err) {
                std::cerr << "Unexpected error while trying to read "
                             "attribute '"
                          << this->attrName << "' in '"
                          << this->attributable.myPath().openPMDPath()
                          << "'' with value '";
                write_to_stderr(attribute) << "': " << err.what() << std::endl;
            },
            [](attribute_read_result::Success) { /* no-op */ }},
        std::move(res));
}

void ConfigAttribute::operator()(WriteOrRead wor)
{
    switch (wor)
    {
    case WriteOrRead::Write:
        write();
        break;
    case WriteOrRead::Read:
        read();
        break;
    }
}

std::shared_ptr<ProcessParsedAttribute> require_scalar =
    std::make_shared<RequireScalar>();
// try converting to vectors (e.g. when a scalar or an array is given)
std::shared_ptr<ProcessParsedAttribute> require_vector =
    std::make_shared<RequireVector>();

auto get_float_types() -> std::deque<Datatype>
{
    std::deque<Datatype> res;
    for (auto dt : openPMD_Datatypes())
    {
        if (isFloatingPoint(dt))
        {
            res.push_back(dt);
        }
    }
    res.push_back(Datatype::ARR_DBL_7);
    return res;
}

auto get_int_types() -> std::deque<Datatype>
{
    std::deque<Datatype> res;
    for (auto dt : openPMD_Datatypes())
    {
        if (std::get<0>(isInteger(dt)))
        {
            res.push_back(dt);
        }
    }
    return res;
}

auto get_numerical_types() -> std::deque<Datatype>
{
    std::deque<Datatype> res;
    for (auto dt : openPMD_Datatypes())
    {
        if (isFloatingPoint(dt) || std::get<0>(isInteger(dt)))
        {
            res.push_back(dt);
        }
    }
    res.push_back(Datatype::ARR_DBL_7);
    return res;
}

auto get_string_types() -> std::deque<Datatype>
{
    return {
        Datatype::STRING,
        Datatype::VEC_STRING,
        Datatype::CHAR,
        Datatype::VEC_CHAR,
        Datatype::UCHAR,
        Datatype::VEC_UCHAR,
        Datatype::SCHAR,
        Datatype::VEC_SCHAR};
}
} // namespace openPMD::internal
