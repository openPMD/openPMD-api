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

    void enqueue(IOTask const);
    virtual std::future< void > flush() = 0;

private:
    std::queue<IOTask> m_work;
};
