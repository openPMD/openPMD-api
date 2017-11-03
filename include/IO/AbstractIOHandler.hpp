#pragma once


#include <exception>
#include <future>
#include <queue>

#include "AccessType.hpp"
#include "Format.hpp"
#include "IOTask.hpp"


class unsupported_data_error : public std::runtime_error
{
public:
    unsupported_data_error(char const* what_arg)
            : std::runtime_error(what_arg)
    { }
    unsupported_data_error(std::string const& what_arg)
            : std::runtime_error(what_arg)
    { }
    virtual ~unsupported_data_error() { }
};


class AbstractIOHandler
{
public:
    static std::shared_ptr< AbstractIOHandler > createIOHandler(std::string const& path, AccessType, Format);

    AbstractIOHandler(std::string const& path, AccessType);
    virtual ~AbstractIOHandler();

    virtual void enqueue(IOTask const);
    virtual std::future< void > flush() = 0;

    std::string const directory;
    AccessType const accessType;
    std::queue< IOTask > m_work;
};  //AbstractIOHandler


class NONEIOHandler : public AbstractIOHandler
{
public:
    NONEIOHandler(std::string const&, AccessType);
    ~NONEIOHandler();

    void enqueue(IOTask const) override;
    std::future< void > flush();
};  //NONEIOHandler
