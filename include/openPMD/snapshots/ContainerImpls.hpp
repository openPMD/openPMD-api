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
        std::function<OpaqueSeriesIterator<value_type>()> begin);

public:
    iterator begin() override;
    iterator end() override;
    const_iterator begin() const override;
    const_iterator end() const override;
    iterator rbegin() override;
    iterator rend() override;
    const_iterator rbegin() const override;
    const_iterator rend() const override;
};

/*
 * @todo how to deal with iteration::open() iteration::close() ?
 */
class RandomAccessIteratorContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;
    Container<Iteration, key_type> m_cont;
    RandomAccessIteratorContainer(Container<Iteration, key_type> cont);

public:
    iterator begin() override;
    iterator end() override;
    const_iterator begin() const override;
    const_iterator end() const override;
    iterator rbegin() override;
    iterator rend() override;
    const_iterator rbegin() const override;
    const_iterator rend() const override;
};
} // namespace openPMD
