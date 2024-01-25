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

namespace openPMD
{
template <typename ChildClass>
class AbstractSeriesIterator
{
public:
    using difference_type = Iteration::IterationIndex_t;
    using value_type = Container<Iteration, difference_type>::value_type;

    // dereference
    virtual value_type const &operator*() const = 0;
    virtual value_type &operator*();
    virtual value_type const *operator->() const;
    virtual value_type *operator->();

    // member access
    virtual const value_type &operator[](difference_type) const;
    virtual value_type &operator[](difference_type);

    // arithmetic random-access
    virtual ChildClass operator+(difference_type) const = 0;
    virtual ChildClass operator+(difference_type);
    virtual ChildClass operator-(difference_type) const;
    virtual ChildClass operator-(difference_type);

    // increment/decrement
    virtual ChildClass &operator++();
    virtual ChildClass &operator--();
    virtual ChildClass operator++(int);
    virtual ChildClass operator--(int);

    // comparison
    virtual difference_type operator-(AbstractSeriesIterator const &) const = 0;
    virtual bool operator==(ChildClass const &) const = 0;
    virtual bool operator!=(ChildClass const &) const;
    virtual bool operator<(ChildClass const &) const = 0;
    virtual bool operator>(ChildClass const &) const;
    virtual bool operator<=(ChildClass const &) const;
    virtual bool operator>=(ChildClass const &) const;

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
