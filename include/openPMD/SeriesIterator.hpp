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
#include <memory>

namespace openPMD
{
// Abstract class that can be used as an abstract interface to an opaque
// iterator implementation
class DynamicSeriesIterator
{
public:
    using difference_type = Iteration::IterationIndex_t;
    using value_type = Container<Iteration, difference_type>::value_type;

protected:
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

// Class template with default method implementations for iterators.
// No virtual classes since there is no use.
// Commented methods must be implemented by child classes.
// Implement as `class MyIterator : public AbstractSeriesIterator<MyIterator>`
// Use `using AbstractSeriesIterator<MyIterator>::operator-` to pull default
// implementations.
template <typename ChildClass>
class AbstractSeriesIterator : public DynamicSeriesIterator
{
public:
    using difference_type = Iteration::IterationIndex_t;
    using value_type = Container<Iteration, difference_type>::value_type;

    // dereference
    // value_type const &operator*() const = 0;
    value_type &operator*();
    value_type const *operator->() const;
    value_type *operator->();

    // increment/decrement
    // ChildClass &operator++();
    // ChildClass &operator--();
    ChildClass operator++(int);
    ChildClass operator--(int);

    // comparison
    // bool operator==(ChildClass const &) const = 0;
    bool operator!=(ChildClass const &) const;

    /*************
     * overrides *
     *************/

protected:
    using parent_t = DynamicSeriesIterator;
    // dereference
    using parent_t::dereference_operator;
    value_type const &dereference_operator() const override;

    // increment/decrement
    DynamicSeriesIterator &increment_operator() override;
    DynamicSeriesIterator &decrement_operator() override;

    // comparison
    bool equality_operator(DynamicSeriesIterator const &) const override;

    std::unique_ptr<DynamicSeriesIterator> clone() const override;

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
