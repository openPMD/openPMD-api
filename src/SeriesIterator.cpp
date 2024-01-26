#include "openPMD/SeriesIterator.hpp"
#include "openPMD/ReadIterations.hpp"
#include <memory>

namespace openPMD
{
// dereference
auto DynamicSeriesIterator::operator*() -> value_type &
{
    return const_cast<value_type &>(
        static_cast<DynamicSeriesIterator const *>(this)->operator*());
}
auto DynamicSeriesIterator::operator->() const -> value_type const *
{
    return &operator*();
}
auto DynamicSeriesIterator::operator->() -> value_type *
{
    return &operator*();
}

// member access
auto DynamicSeriesIterator::operator[](difference_type diff) const
    -> value_type const &
{
    return **this->plus_operator(diff);
}
auto DynamicSeriesIterator::operator[](difference_type diff) -> value_type &
{
    return **this->plus_operator(diff);
}

// arithmetic random-access
std::unique_ptr<DynamicSeriesIterator>
DynamicSeriesIterator::plus_operator(difference_type diff)
{
    return static_cast<DynamicSeriesIterator const *>(this)->plus_operator(
        diff);
}
std::unique_ptr<DynamicSeriesIterator>
DynamicSeriesIterator::minus_operator(difference_type diff) const
{
    return this->plus_operator(-diff);
}
std::unique_ptr<DynamicSeriesIterator>
DynamicSeriesIterator::minus_operator(difference_type diff)
{
    return this->plus_operator(-diff);
}

// increment/decrement
DynamicSeriesIterator &DynamicSeriesIterator::increment_operator()
{
    *this = *plus_operator(1);
    return *this;
}
DynamicSeriesIterator &DynamicSeriesIterator::decrement_operator()
{
    *this = *minus_operator(1);
    return *this;
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

/*************
 * overrides *
 *************/

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
std::unique_ptr<DynamicSeriesIterator>
AbstractSeriesIterator<ChildClass>::plus_operator(difference_type diff) const
{
    return std::unique_ptr<DynamicSeriesIterator>{
        new ChildClass(operator+(diff))};
}
template <typename ChildClass>
std::unique_ptr<DynamicSeriesIterator>
AbstractSeriesIterator<ChildClass>::plus_operator(difference_type diff)
{
    return std::unique_ptr<DynamicSeriesIterator>{
        new ChildClass(operator+(diff))};
}
template <typename ChildClass>
std::unique_ptr<DynamicSeriesIterator>
AbstractSeriesIterator<ChildClass>::minus_operator(difference_type diff) const
{
    return std::unique_ptr<DynamicSeriesIterator>{
        new ChildClass(operator-(diff))};
}
template <typename ChildClass>
std::unique_ptr<DynamicSeriesIterator>
AbstractSeriesIterator<ChildClass>::minus_operator(difference_type diff)
{
    return std::unique_ptr<DynamicSeriesIterator>{
        new ChildClass(operator-(diff))};
}

// increment/decrement
template <typename ChildClass>
DynamicSeriesIterator &AbstractSeriesIterator<ChildClass>::increment_operator()
{
    return ++*this;
}
template <typename ChildClass>
DynamicSeriesIterator &AbstractSeriesIterator<ChildClass>::decrement_operator()
{
    return --*this;
}

// comparison
template <typename ChildClass>
auto AbstractSeriesIterator<ChildClass>::difference_operator(
    DynamicSeriesIterator const &other) const -> difference_type
{
    return operator-(dynamic_cast<ChildClass const &>(other));
}
template <typename ChildClass>
bool AbstractSeriesIterator<ChildClass>::equality_operator(
    DynamicSeriesIterator const &other) const
{
    if (auto child = dynamic_cast<ChildClass const *>(&other); child)
    {
        return operator==(*child);
    }
    else
    {
        return false; // or throw error?
    }
}
template <typename ChildClass>
bool AbstractSeriesIterator<ChildClass>::less_than_operator(
    DynamicSeriesIterator const &other) const
{
    if (auto child = dynamic_cast<ChildClass const *>(&other); child)
    {
        return operator<(*child);
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
