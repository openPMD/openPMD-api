/* Copyright 2018-2021 Franz Poeschel
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

#include "openPMD/Dataset.hpp"
#include <memory>
#include <random>

namespace openPMD
{
/**
 * An abstract class to create one iteration of data per thread.
 * @tparam T The type of data to produce.
 */
template <typename T>
class DatasetFiller
{
protected:
    Extent::value_type m_numberOfItems;

public:
    using resultType = T;

    explicit DatasetFiller(Extent::value_type numberOfItems = 0);

    /** This class will be derived from
     */
    virtual ~DatasetFiller() = default;

    /**
     * Create a shared pointer of m_numberOfItems items of type T.
     * Should take roughly the same amount of time per call as long as
     * m_numberOfItems does not change.
     * @return
     */
    virtual std::shared_ptr<T> produceData() = 0;

    /**
     * Set number of items to be produced.
     * @param numberOfItems The number.
     */
    virtual void setNumberOfItems(Extent::value_type numberOfItems) = 0;
};

template <typename T>
DatasetFiller<T>::DatasetFiller(Extent::value_type numberOfItems)
    : m_numberOfItems(numberOfItems)
{}

template <typename DF>
class SimpleDatasetFillerProvider
{
public:
    using resultType = typename DF::resultType;

private:
    std::shared_ptr<DF> m_df;

    template <typename T, typename Dummy = void>
    struct Helper
    {
        std::shared_ptr<DatasetFiller<T>> operator()(std::shared_ptr<DF> &)
        {
            throw std::runtime_error(
                "Can only create data of type " +
                datatypeToString(determineDatatype<resultType>()));
        }
    };

    template <typename Dummy>
    struct Helper<resultType, Dummy>
    {
        std::shared_ptr<DatasetFiller<resultType>>
        operator()(std::shared_ptr<DF> &df)
        {
            return df;
        }
    };

public:
    explicit SimpleDatasetFillerProvider(DF df)
        : m_df{std::make_shared<DF>(std::move(df))}
    {}

    template <typename T>
    std::shared_ptr<DatasetFiller<T>> operator()()
    {
        Helper<T> h;
        return h(m_df);
    }
};

} // namespace openPMD
