#pragma once


#include <future>
#include <queue>

#include "AccessType.hpp"
#include "IOTask.hpp"


class AbstractIOHandler
{
public:
    AbstractIOHandler(std::string const& path, AccessType);
    virtual ~AbstractIOHandler();

    virtual void enqueue(IOTask const);
    virtual std::future< void > flush() = 0;

    std::string const directory;
    AccessType const accessType;
    std::queue<IOTask> m_work;
};

class NONEIOHandler : public AbstractIOHandler
{
public:
    NONEIOHandler(std::string const&, AccessType);
    ~NONEIOHandler();

    void enqueue(IOTask const) override;
    std::future< void > flush();
};
