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
        std::string backend_in, std::string what)
        : Error("Operation unsupported in " + backend_in + ": " + what)
        , backend{std::move(backend_in)}
    {}

    WrongAPIUsage::WrongAPIUsage(std::string what)
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

    Internal::Internal(std::string const &what)
        : Error("Internal error: " + what + "\nThis is a bug. Please report.")
    {}
} // namespace error
} // namespace openPMD
