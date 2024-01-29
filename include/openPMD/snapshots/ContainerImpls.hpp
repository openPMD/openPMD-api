#pragma once

#include "openPMD/snapshots/ContainerTraits.hpp"
#include "openPMD/snapshots/RandomAccessIterator.hpp"

namespace openPMD
{
class StatefulSnapshotsContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;
    std::function<OpaqueSeriesIterator()> m_begin;
    StatefulSnapshotsContainer(std::function<OpaqueSeriesIterator()> begin)
        : m_begin(std::move(begin))
    {}

public:
    iterator_t begin() override;
    iterator_t end() override;
};

/*
 * @todo how to deal with iteration::open() iteration::close() ?
 */
class RandomAccessIteratorContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;
    Container<value_t, index_t> m_cont;
    RandomAccessIteratorContainer(Container<value_t, index_t> cont)
        : m_cont(std::move(cont))
    {}

public:
    inline iterator_t begin() override
    {
        return OpaqueSeriesIterator(std::unique_ptr<DynamicSeriesIterator>{
            new RandomAccessIterator(m_cont.begin())});
    }
    inline iterator_t end() override
    {
        return OpaqueSeriesIterator(std::unique_ptr<DynamicSeriesIterator>{
            new RandomAccessIterator(m_cont.end())});
    }
};
} // namespace openPMD
