#pragma once

#include "openPMD/snapshots/ContainerTraits.hpp"
#include "openPMD/snapshots/RandomAccessIterator.hpp"

namespace openPMD
{
class StatefulSnapshotsContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;
    std::function<OpaqueSeriesIterator<value_type>()> m_begin;
    StatefulSnapshotsContainer(
        std::function<OpaqueSeriesIterator<value_type>()> begin)
        : m_begin(std::move(begin))
    {}

public:
    iterator begin() override;
    iterator end() override;
    const_iterator begin() const override;
    const_iterator end() const override;
};

/*
 * @todo how to deal with iteration::open() iteration::close() ?
 */
class RandomAccessIteratorContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;
    Container<Iteration, key_type> m_cont;
    RandomAccessIteratorContainer(Container<Iteration, key_type> cont)
        : m_cont(std::move(cont))
    {}

public:
    inline iterator begin() override
    {
        return OpaqueSeriesIterator(
            std::unique_ptr<DynamicSeriesIterator<value_type>>{
                new RandomAccessIterator(m_cont.begin())});
    }
    inline iterator end() override
    {
        return OpaqueSeriesIterator(
            std::unique_ptr<DynamicSeriesIterator<value_type>>{
                new RandomAccessIterator(m_cont.end())});
    }
    inline const_iterator begin() const override
    {
        return OpaqueSeriesIterator(
            std::unique_ptr<DynamicSeriesIterator<value_type const>>{
                new RandomAccessIterator(m_cont.begin())});
    }
    inline const_iterator end() const override
    {
        return OpaqueSeriesIterator(
            std::unique_ptr<DynamicSeriesIterator<value_type const>>{
                new RandomAccessIterator(m_cont.end())});
    }
};
} // namespace openPMD
