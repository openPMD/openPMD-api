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
class DynamicSeriesIterator
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
    virtual value_type const &operator[](difference_type) const;
    virtual value_type &operator[](difference_type);

protected:
    friend class OpaqueSeriesIterator;
    // arithmetic random-access
    virtual std::unique_ptr<DynamicSeriesIterator>
        plus_operator(difference_type) const = 0;
    virtual std::unique_ptr<DynamicSeriesIterator>
        plus_operator(difference_type);
    virtual std::unique_ptr<DynamicSeriesIterator>
        minus_operator(difference_type) const;
    virtual std::unique_ptr<DynamicSeriesIterator>
        minus_operator(difference_type);

    // increment/decrement
    virtual DynamicSeriesIterator &increment_operator();
    virtual DynamicSeriesIterator &decrement_operator();

    // comparison
    virtual difference_type
    difference_operator(DynamicSeriesIterator const &) const = 0;
    virtual bool equality_operator(DynamicSeriesIterator const &) const = 0;
    virtual bool less_than_operator(DynamicSeriesIterator const &) const = 0;

    std::unique_ptr<DynamicSeriesIterator> clone() const;
};

template <typename ChildClass>
class AbstractSeriesIterator : public DynamicSeriesIterator
{
public:
    using difference_type = Iteration::IterationIndex_t;
    using value_type = Container<Iteration, difference_type>::value_type;

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
    virtual difference_type operator-(ChildClass const &) const = 0;
    virtual bool operator==(ChildClass const &) const = 0;
    virtual bool operator!=(ChildClass const &) const;
    virtual bool operator<(ChildClass const &) const = 0;
    virtual bool operator>(ChildClass const &) const;
    virtual bool operator<=(ChildClass const &) const;
    virtual bool operator>=(ChildClass const &) const;

    /*************
     * overrides *
     *************/

    // member access
    value_type const &operator[](difference_type) const override;
    value_type &operator[](difference_type) override;

protected:
    // arithmetic random-access
    std::unique_ptr<DynamicSeriesIterator>
        plus_operator(difference_type) const override;
    std::unique_ptr<DynamicSeriesIterator>
        plus_operator(difference_type) override;
    std::unique_ptr<DynamicSeriesIterator>
        minus_operator(difference_type) const override;
    std::unique_ptr<DynamicSeriesIterator>
        minus_operator(difference_type) override;

    // increment/decrement
    DynamicSeriesIterator &increment_operator() override;
    DynamicSeriesIterator &decrement_operator() override;

    // comparison
    difference_type
    difference_operator(DynamicSeriesIterator const &) const override;
    bool equality_operator(DynamicSeriesIterator const &) const override;
    bool less_than_operator(DynamicSeriesIterator const &) const override;

    std::unique_ptr<DynamicSeriesIterator> clone() const;

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
