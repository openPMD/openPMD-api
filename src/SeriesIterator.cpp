#include "openPMD/SeriesIterator.hpp"
#include "openPMD/ReadIterations.hpp"

namespace openPMD
{
// dereference
template <typename ChildClass>
auto AbstractSeriesIterator<ChildClass>::operator*() -> value_type &
{
    return const_cast<value_type &>(
        static_cast<AbstractSeriesIterator const *>(this)->operator*());
}
template <typename ChildClass>
auto AbstractSeriesIterator<ChildClass>::operator->() const
    -> value_type const *
{
    return &operator*();
}
template <typename ChildClass>
auto AbstractSeriesIterator<ChildClass>::operator->() -> value_type *
{
    return &operator*();
}

// member access
template <typename ChildClass>
auto AbstractSeriesIterator<ChildClass>::operator[](difference_type diff) const
    -> value_type const &
{
    return *(*this + diff);
}
template <typename ChildClass>
auto AbstractSeriesIterator<ChildClass>::operator[](difference_type diff)
    -> value_type &
{
    return *(*this + diff);
}

// arithmetic random-access
template <typename ChildClass>
ChildClass AbstractSeriesIterator<ChildClass>::operator+(difference_type diff)
{
    return static_cast<AbstractSeriesIterator const *>(this)->operator+(diff);
}
template <typename ChildClass>
ChildClass
AbstractSeriesIterator<ChildClass>::operator-(difference_type diff) const
{
    return this->operator+(-diff);
}
template <typename ChildClass>
ChildClass AbstractSeriesIterator<ChildClass>::operator-(difference_type diff)
{
    return this->operator+(-diff);
}

// increment/decrement
template <typename ChildClass>
ChildClass &AbstractSeriesIterator<ChildClass>::operator++()
{
    *this_child() = *this + 1;
    return *this_child();
}
template <typename ChildClass>
ChildClass &AbstractSeriesIterator<ChildClass>::operator--()
{
    *this_child() = *this - 1;
    return *this_child();
}
template <typename ChildClass>
ChildClass AbstractSeriesIterator<ChildClass>::operator++(int)
{
    auto prev = *this_child();
    operator++();
    return prev;
}
template <typename ChildClass>
ChildClass AbstractSeriesIterator<ChildClass>::operator--(int)
{
    auto prev = *this_child();
    operator--();
    return prev;
}

// comparison
template <typename ChildClass>
bool AbstractSeriesIterator<ChildClass>::operator!=(
    ChildClass const &other) const
{
    return !operator==(other);
}
template <typename ChildClass>
bool AbstractSeriesIterator<ChildClass>::operator>(
    ChildClass const &other) const
{
    return other.operator<(*this_child());
}
template <typename ChildClass>
bool AbstractSeriesIterator<ChildClass>::operator<=(
    ChildClass const &other) const
{
    return !operator>(other);
}
template <typename ChildClass>
bool AbstractSeriesIterator<ChildClass>::operator>=(
    ChildClass const &other) const
{
    return !operator<(other);
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
    return iterator.operator+(index);
}
template <typename ChildClass>
ChildClass operator+(
    Iteration::IterationIndex_t index,
    AbstractSeriesIterator<ChildClass> &iterator)
{
    return iterator.operator+(index);
}
template <typename ChildClass>
ChildClass operator-(
    Iteration::IterationIndex_t index,
    AbstractSeriesIterator<ChildClass> const &iterator)
{
    return iterator.operator-(index);
}
template <typename ChildClass>
ChildClass operator-(
    Iteration::IterationIndex_t index,
    AbstractSeriesIterator<ChildClass> &iterator)
{
    return iterator.operator-(index);
}

#define OPENPMD_INSTANTIATE(type)                                              \
    template class AbstractSeriesIterator<type>;                               \
    template auto operator+(                                                   \
        Iteration::IterationIndex_t, AbstractSeriesIterator<type> const &)     \
        -> type;                                                               \
    template auto operator+(                                                   \
        Iteration::IterationIndex_t, AbstractSeriesIterator<type> &) -> type;  \
    template auto operator-(                                                   \
        Iteration::IterationIndex_t, AbstractSeriesIterator<type> const &)     \
        -> type;                                                               \
    template auto operator-(                                                   \
        Iteration::IterationIndex_t, AbstractSeriesIterator<type> &) -> type;
OPENPMD_INSTANTIATE(SeriesIterator)
#undef OPENPMD_INSTANTIATE
} // namespace openPMD
