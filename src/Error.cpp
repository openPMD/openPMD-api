#include "openPMD/Error.hpp"

#include <sstream>

namespace openPMD
{
const char *Error::what() const noexcept
{
    return m_what.c_str();
}

namespace error
{
    OperationUnsupportedInBackend::OperationUnsupportedInBackend(
        std::string backend_in, std::string const &what)
        : Error("Operation unsupported in " + backend_in + ": " + what)
        , backend{std::move(backend_in)}
    {}

    void throwOperationUnsupportedInBackend(
        std::string backend, std::string const &what)
    {
        throw OperationUnsupportedInBackend(std::move(backend), what);
    }

    WrongAPIUsage::WrongAPIUsage(std::string const &what)
        : Error("Wrong API usage: " + what)
    {}

    static std::string concatVector(
        std::vector<std::string> const &vec,
        std::string const &intersperse = ".")
    {
        if (vec.empty())
        {
            return "";
        }
        std::stringstream res;
        res << vec[0];
        for (size_t i = 1; i < vec.size(); ++i)
        {
            res << intersperse << vec[i];
        }
        return res.str();
    }

    BackendConfigSchema::BackendConfigSchema(
        std::vector<std::string> errorLocation_in, std::string what)
        : Error(
              "Wrong JSON/TOML schema at index '" +
              concatVector(errorLocation_in) + "': " + std::move(what))
        , errorLocation(std::move(errorLocation_in))
    {}

    void throwBackendConfigSchema(
        std::vector<std::string> jsonPath, std::string what)
    {
        throw BackendConfigSchema(std::move(jsonPath), std::move(what));
    }

    Internal::Internal(std::string const &what)
        : Error(
              "Internal error: " + what +
              "\nThis is a bug. Please report at ' "
              "https://github.com/openPMD/openPMD-api/issues'.")
    {}

    namespace
    {
        std::string asString(AffectedObject obj)
        {
            switch (obj)
            {
                using AO = AffectedObject;
            case AO::Attribute:
                return "Attribute";
            case AO::Dataset:
                return "Dataset";
            case AO::File:
                return "File";
            case AO::Group:
                return "Group";
            case AO::Other:
                return "Other";
            }
            return "Unreachable";
        }
        std::string asString(Reason obj)
        {
            switch (obj)
            {
                using Re = Reason;
            case Re::NotFound:
                return "NotFound";
            case Re::CannotRead:
                return "CannotRead";
            case Re::UnexpectedContent:
                return "UnexpectedContent";
            case Re::Inaccessible:
                return "Inaccessible";
            case Re::Other:
                return "Other";
            }
            return "Unreachable";
        }
    } // namespace

    ReadError::ReadError(
        AffectedObject affectedObject_in,
        Reason reason_in,
        std::optional<std::string> backend_in,
        std::string description_in)
        : Error(
              (backend_in ? ("Read Error in backend " + *backend_in)
                          : "Read Error in frontend ") +
              "\nObject type:\t" + asString(affectedObject_in) +
              "\nError type:\t" + asString(reason_in) +
              "\nFurther description:\t" + description_in)
        , affectedObject(affectedObject_in)
        , reason(reason_in)
        , backend(std::move(backend_in))
        , description(std::move(description_in))
    {}

    IllegalInOpenPMDStandard::IllegalInOpenPMDStandard(std::string what_in)
        : Error(
              "Operation leads to illegal use of the openPMD standard:\n" +
              std::move(what_in))
    {}

    void throwReadError(
        AffectedObject affectedObject,
        Reason reason,
        std::optional<std::string> backend,
        std::string description)
    {
        throw ReadError(
            affectedObject, reason, std::move(backend), std::move(description));
    }

    NoSuchAttribute::NoSuchAttribute(std::string attributeName)
        : Error(std::move(attributeName))
    {}

    void throwNoSuchAttribute(std::string attributeName)
    {
        throw NoSuchAttribute(std::move(attributeName));
    }
} // namespace error
} // namespace openPMD
