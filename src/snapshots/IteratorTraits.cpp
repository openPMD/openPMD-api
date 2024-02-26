#include "openPMD/snapshots/IteratorTraits.hpp"
#include "openPMD/snapshots/RandomAccessIterator.hpp"
#include "openPMD/snapshots/Snapshots.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"
#include <memory>

namespace openPMD
{
template <typename value_type>
DynamicSeriesIterator<value_type>::~DynamicSeriesIterator() = default;

template <typename value_type>
auto DynamicSeriesIterator<value_type>::dereference_operator() -> value_type &
{
    return const_cast<value_type &>(
        static_cast<DynamicSeriesIterator const *>(this)
            ->dereference_operator());
}

template <typename ChildClass, typename value_type>
AbstractSeriesIterator<ChildClass, value_type>::~AbstractSeriesIterator() =
    default;

// dereference
template <typename ChildClass, typename value_type>
auto AbstractSeriesIterator<ChildClass, value_type>::operator*() -> value_type &
{
    return const_cast<value_type &>(
        static_cast<ChildClass const *>(this)->operator*());
}
template <typename ChildClass, typename value_type>
auto AbstractSeriesIterator<ChildClass, value_type>::operator->() const
    -> value_type const *
{
    return &this_child()->operator*();
}
template <typename ChildClass, typename value_type>
auto AbstractSeriesIterator<ChildClass, value_type>::operator->()
    -> value_type *
{
    return &this_child()->operator*();
}

// increment/decrement
template <typename ChildClass, typename value_type>
ChildClass AbstractSeriesIterator<ChildClass, value_type>::operator++(int)
{
    auto prev = *this_child();
    this_child()->operator++();
    return prev;
}
template <typename ChildClass, typename value_type>
ChildClass AbstractSeriesIterator<ChildClass, value_type>::operator--(int)
{
    auto prev = *this_child();
    this_child()->operator--();
    return prev;
}

// comparison
template <typename ChildClass, typename value_type>
bool AbstractSeriesIterator<ChildClass, value_type>::operator!=(
    ChildClass const &other) const
{
    return !this_child()->operator==(other);
}

// helpers
template <typename ChildClass, typename value_type>
ChildClass *AbstractSeriesIterator<ChildClass, value_type>::this_child()
{
    return static_cast<ChildClass *>(this);
}
template <typename ChildClass, typename value_type>
ChildClass const *
AbstractSeriesIterator<ChildClass, value_type>::this_child() const
{
    return static_cast<ChildClass const *>(this);
}

// out-of-line arithmetic operators
template <typename ChildClass, typename value_type>
ChildClass operator+(
    Iteration::IterationIndex_t index,
    AbstractSeriesIterator<ChildClass, value_type> const &iterator)
{
    return static_cast<ChildClass const &>(iterator).operator+(index);
}

/*************
 * overrides *
 *************/

// dereference
template <typename ChildClass, typename value_type>
auto AbstractSeriesIterator<ChildClass, value_type>::dereference_operator()
    const -> value_type const &
{
    return this_child()->operator*();
}

// increment/decrement
template <typename ChildClass, typename value_type>
auto AbstractSeriesIterator<ChildClass, value_type>::increment_operator()
    -> parent_t &
{
    return ++*this_child();
}
template <typename ChildClass, typename value_type>
auto AbstractSeriesIterator<ChildClass, value_type>::decrement_operator()
    -> parent_t &
{
    return --*this_child();
}

// comparison
template <typename ChildClass, typename value_type>
bool AbstractSeriesIterator<ChildClass, value_type>::equality_operator(
    parent_t const &other) const
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

template <typename ChildClass, typename value_type>
auto AbstractSeriesIterator<ChildClass, value_type>::clone() const
    -> std::unique_ptr<parent_t>
{
    return std::unique_ptr<parent_t>(
        new ChildClass(*static_cast<ChildClass const *>(this)));
}

using iterations_container_t =
    Container<Iteration, Iteration::IterationIndex_t>;

#define OPENPMD_INSTANTIATE(type)                                              \
    template class AbstractSeriesIterator<type, typename type::value_type>;
OPENPMD_INSTANTIATE(StatefulIterator)
OPENPMD_INSTANTIATE(OpaqueSeriesIterator<iterations_container_t::value_type>)
OPENPMD_INSTANTIATE(
    OpaqueSeriesIterator<iterations_container_t::value_type const>)
OPENPMD_INSTANTIATE(RandomAccessIterator<iterations_container_t::iterator>)
OPENPMD_INSTANTIATE(
    RandomAccessIterator<iterations_container_t::const_iterator>)
OPENPMD_INSTANTIATE(
    RandomAccessIterator<iterations_container_t::reverse_iterator>)
OPENPMD_INSTANTIATE(
    RandomAccessIterator<iterations_container_t::const_reverse_iterator>)
#undef OPENPMD_INSTANTIATE
} // namespace openPMD
