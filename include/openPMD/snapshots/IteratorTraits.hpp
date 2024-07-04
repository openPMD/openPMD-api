/* Copyright 2024 Franz Poeschel
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "openPMD/Iteration.hpp"
#include "openPMD/backend/Writable.hpp"

#include <memory>

/*
 * Public header due to use in OpaqueSeriesIterator type which is the public
 * iterator type of Snapshots class.
 */

namespace openPMD
{
/*
 * Abstract class that can be used as an abstract interface to an opaque
 * iterator implementation
 *
 * This has renamed operator names for two reasons:
 *
 * 1. Name shadowing of default implementations is too finnicky.
 * 2. The return type for the actual operators should be a reference to the
 *    child class, but for an Interface we need a common return type.
 *
 * For returning a reference to the child class, we need a CRT-style template to
 * know the type. That does not work for an interface. As a result, this generic
 * Iterator interface is split in two parts, class DynamicSeriesIterator which
 * can be used generically without specifying the child class. All methods that
 * need to know the child class type are put into the CRT-style class template
 * AbstractSeriesIterator below.
 */
template <
    typename value_type =
        Container<Iteration, Iteration::IterationIndex_t>::value_type>
class DynamicSeriesIterator
{
public:
    using difference_type = Iteration::IterationIndex_t;
    virtual ~DynamicSeriesIterator() = 0;

protected:
    template <typename>
    friend class OpaqueSeriesIterator;

    // dereference
    virtual value_type const &dereference_operator() const = 0;
    virtual value_type &dereference_operator();

    // increment/decrement
    virtual DynamicSeriesIterator &increment_operator() = 0;
    virtual DynamicSeriesIterator &decrement_operator() = 0;

    // comparison
    virtual bool equality_operator(DynamicSeriesIterator const &) const = 0;

    virtual std::unique_ptr<DynamicSeriesIterator> clone() const = 0;
};
/*
 * Class template with default method implementations for iterators.
 * Complementary class to above class DynamicSeriesIterator, containing those
 * methods that have the specific Iterator type in their type specification. See
 * the documentation for DynamicSeriesIterator for more details.
 * No virtual classes since there is no use.
 * Commented methods must be implemented by child classes.
 * Implement as `class MyIterator : public AbstractSeriesIterator<MyIterator>`
 * Use `using AbstractSeriesIterator<MyIterator>::operator-` to pull default
 * implementations.
 */
template <
    typename ChildClass,
    typename value_type_in = typename ChildClass::value_type>
class AbstractSeriesIterator : public DynamicSeriesIterator<value_type_in>
{
public:
    using difference_type = Iteration::IterationIndex_t;
    using value_type = value_type_in;

    ~AbstractSeriesIterator() override;

    /*
     * Default definitions that can be pulled in implementing child classes with
     * `using` declarations. Does not work for overloaded methods due to
     * compiler bugs in ICPC, hence e.g. `default_increment_operator` instead of
     * `operator++`.
     */

    // dereference
    // value_type const &operator*() const = 0;
    // value_type &operator*() = 0;
    auto operator->() const -> value_type const *;
    auto operator->() -> value_type *;

    // increment/decrement
    // ChildClass &operator++();
    // ChildClass &operator--();
    // ChildClass &operator++(int);
    // ChildClass &operator--(int);
    auto default_increment_operator(int) -> ChildClass;
    auto default_decrement_operator(int) -> ChildClass;

    // comparison
    // bool operator==(ChildClass const &) const = 0;
    bool operator!=(ChildClass const &) const;

    /*************
     * overrides *
     *************/

protected:
    using parent_t = DynamicSeriesIterator<value_type_in>;
    // dereference
    using parent_t::dereference_operator;
    auto dereference_operator() const -> value_type const & override;

    // increment/decrement
    auto increment_operator() -> parent_t & override;
    auto decrement_operator() -> parent_t & override;

    // comparison
    auto equality_operator(parent_t const &) const -> bool override;

    auto clone() const -> std::unique_ptr<parent_t> override;

private:
    ChildClass *this_child();
    ChildClass const *this_child() const;
};

template <typename ChildClass>
ChildClass operator+(
    Iteration::IterationIndex_t, AbstractSeriesIterator<ChildClass> const &);
template <typename ChildClass>
ChildClass
operator+(Iteration::IterationIndex_t, AbstractSeriesIterator<ChildClass> &);
template <typename ChildClass>
ChildClass operator-(
    Iteration::IterationIndex_t, AbstractSeriesIterator<ChildClass> const &);
template <typename ChildClass>
ChildClass
operator-(Iteration::IterationIndex_t, AbstractSeriesIterator<ChildClass> &);
} // namespace openPMD
