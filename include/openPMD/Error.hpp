#pragma once

#include "openPMD/ThrowError.hpp"
#include "openPMD/auxiliary/Visibility.hpp"

#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace openPMD
{
/**
 * @brief Base class for all openPMD-specific error types.
 *
 * Should not be instantiated directly, but only by implementing child classes
 * in the openPMD::error namespace.
 *
 */
class OPENPMD_PUBLIC Error : public std::exception
{
private:
    std::string m_what;

protected:
    Error(std::string what) : m_what(std::move(what))
    {}

public:
    virtual const char *what() const noexcept;

    Error(Error const &) = default;
    Error(Error &&) = default;

    Error &operator=(Error const &) = default;
    Error &operator=(Error &&) = default;

    virtual ~Error() noexcept = default;
};

namespace error
{
    /**
     * @brief An operation was requested that is not supported in a specific
     *     backend.
     *
     * Example: Append mode is not available in JSON.
     */
    class OPENPMD_PUBLIC OperationUnsupportedInBackend : public Error
    {
    public:
        std::string backend;
        OperationUnsupportedInBackend(
            std::string backend_in, std::string const &what);
    };

    /**
     * @brief The API was used in an illegal way.
     *
     * Example: File-based iteration encoding is selected without specifying an
     *     expansion pattern.
     */
    class OPENPMD_PUBLIC WrongAPIUsage : public Error
    {
    public:
        WrongAPIUsage(std::string const &what);
    };

    class OPENPMD_PUBLIC BackendConfigSchema : public Error
    {
    public:
        std::vector<std::string> errorLocation;

        BackendConfigSchema(std::vector<std::string>, std::string what);
    };

    /**
     * @brief Internal errors that should not happen. Please report.
     *
     * Example: A nullpointer is observed somewhere.
     */
    class OPENPMD_PUBLIC Internal : public Error
    {
    public:
        Internal(std::string const &what);
    };

    /*
     * Read error concerning a specific object.
     */
    class OPENPMD_PUBLIC ReadError : public Error
    {
    public:
        AffectedObject affectedObject;
        Reason reason;
        // If empty, then the error is thrown by the frontend
        std::optional<std::string> backend;
        std::string description; // object path, further details, ...

        ReadError(
            AffectedObject,
            Reason,
            std::optional<std::string> backend_in,
            std::string description_in);
    };

    class NoSuchAttribute : public Error
    {
    public:
        NoSuchAttribute(std::string attributeName);
    };

    class IllegalInOpenPMDStandard : public Error
    {
    public:
        IllegalInOpenPMDStandard(std::string what);
    };
} // namespace error

/**
 * @brief Backward-compatibility alias for no_such_file_error.
 *
 */
using no_such_file_error = error::ReadError;

/**
 * @brief Backward-compatibility alias for unsupported_data_error.
 *
 */
using unsupported_data_error = error::OperationUnsupportedInBackend;

/**
 * @brief Backward-compatibility alias for no_such_attribute_error.
 *
 */
using no_such_attribute_error = error::NoSuchAttribute;
} // namespace openPMD
