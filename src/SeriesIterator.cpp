#include "openPMD/SeriesIterator.hpp"
#include "openPMD/RandomAccessSnapshots.hpp"
#include "openPMD/ReadIterations.hpp"
#include "openPMD/Snapshots.hpp"
#include <memory>

namespace openPMD
{
auto DynamicSeriesIterator::dereference_operator() -> value_type &
{
    return const_cast<value_type &>(
        static_cast<DynamicSeriesIterator const *>(this)
            ->dereference_operator());
}

// dereference
template <typename ChildClass>
auto AbstractSeriesIterator<ChildClass>::operator*() -> value_type &
{
    return const_cast<value_type &>(
        static_cast<ChildClass const *>(this)->operator*());
}
template <typename ChildClass>
auto AbstractSeriesIterator<ChildClass>::operator->() const
    -> value_type const *
{
    return &this_child()->operator*();
}
template <typename ChildClass>
auto AbstractSeriesIterator<ChildClass>::operator->() -> value_type *
{
    return &this_child()->operator*();
}

// increment/decrement
template <typename ChildClass>
ChildClass AbstractSeriesIterator<ChildClass>::operator++(int)
{
    auto prev = *this_child();
    this_child()->operator++();
    return prev;
}
template <typename ChildClass>
ChildClass AbstractSeriesIterator<ChildClass>::operator--(int)
{
    auto prev = *this_child();
    this_child()->operator--();
    return prev;
}

// comparison
template <typename ChildClass>
bool AbstractSeriesIterator<ChildClass>::operator!=(
    ChildClass const &other) const
{
    return !this_child()->operator==(other);
}

// helpers
template <typename ChildClass>
ChildClass *AbstractSeriesIterator<ChildClass>::this_child()
{
    return static_cast<ChildClass *>(this);
}
template <typename ChildClass>
ChildClass const *AbstractSeriesIterator<ChildClass>::this_child() const
{
    return static_cast<ChildClass const *>(this);
}

// out-of-line arithmetic operators
template <typename ChildClass>
ChildClass operator+(
    Iteration::IterationIndex_t index,
    AbstractSeriesIterator<ChildClass> const &iterator)
{
    return static_cast<ChildClass const &>(iterator).operator+(index);
}

/*************
 * overrides *
 *************/

// dereference
template <typename ChildClass>
auto AbstractSeriesIterator<ChildClass>::dereference_operator() const
    -> value_type const &
{
    return this_child()->operator*();
}

// increment/decrement
template <typename ChildClass>
DynamicSeriesIterator &AbstractSeriesIterator<ChildClass>::increment_operator()
{
    return ++*this_child();
}
template <typename ChildClass>
DynamicSeriesIterator &AbstractSeriesIterator<ChildClass>::decrement_operator()
{
    return --*this_child();
}

// comparison
template <typename ChildClass>
bool AbstractSeriesIterator<ChildClass>::equality_operator(
    DynamicSeriesIterator const &other) const
{
    if (auto child = dynamic_cast<ChildClass const *>(&other); child)
    {
        return this_child()->operator==(*child);
    }
    else
    {
        return false; // or throw error?
    }
}

template <typename ChildClass>
std::unique_ptr<DynamicSeriesIterator>
AbstractSeriesIterator<ChildClass>::clone() const
{
    return std::unique_ptr<DynamicSeriesIterator>(
        new ChildClass(*static_cast<ChildClass const *>(this)));
}

#define OPENPMD_INSTANTIATE(type) template class AbstractSeriesIterator<type>;
OPENPMD_INSTANTIATE(SeriesIterator)
OPENPMD_INSTANTIATE(OpaqueSeriesIterator)
OPENPMD_INSTANTIATE(RandomAccessSnapshots)
#undef OPENPMD_INSTANTIATE
} // namespace openPMD
