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
#include "openPMD/benchmark/mpi/DatasetFiller.hpp"
#include <memory>
#include <random>

namespace openPMD
{
template <typename Distr, typename T = typename Distr::result_type>
class RandomDatasetFiller : public DatasetFiller<T>
{

private:
    Distr distr;
    std::default_random_engine engine;
    std::shared_ptr<T> buffered;

public:
    using resultType = T;

    explicit RandomDatasetFiller(
        Distr distribution, Extent::value_type numOfItems = 0)
        : DatasetFiller<T>(numOfItems), distr(distribution)
    {}

    std::shared_ptr<T> produceData() override
    {
        if (this->buffered)
        {
            return buffered;
        }
        auto res = std::shared_ptr<T>{
            new T[this->m_numberOfItems], [](T *d) { delete[] d; }};
        auto ptr = res.get();
        for (typename Extent::value_type i = 0; i < this->m_numberOfItems; i++)
        {
            ptr[i] = this->distr(this->engine);
        }
        return res;
    }

    /**
     *
     * @tparam X Dummy template parameter such that the RandomDatasetFiller is
     * usable also when this function's implementation does not work on the
     * distribution's concrete type.
     * @param numberOfItems Number of items to be produced per call of
     *        produceData.
     * @param lower Lower bound for the random values to be generated.
     * @param upper Upper bound for the random values to be generated.
     * @return An instance of RandomDatasetFiller matching the given parameters.
     */
    template <typename X = Distr>
    static RandomDatasetFiller<X, T> makeRandomDatasetFiller(
        Extent::value_type numberOfItems,
        typename X::result_type lower,
        typename X::result_type upper)
    {
        return RandomDatasetFiller<X>(X(lower, upper), numberOfItems);
    }

    void setSeed(std::default_random_engine::result_type seed)
    {
        this->engine = std::default_random_engine(seed);
    }

    void randomSeed()
    {
        std::random_device rd;
        this->engine = std::default_random_engine(rd());
    }

    /**
     * Activate buffer mode. Create a bunch of data to write (instantly)
     * and return that upon calling <operator()>().
     */
    void bufferMode()
    {
        if (!this->buffered)
        {
            this->buffered = this->produceData();
        }
    }

    void setNumberOfItems(Extent::value_type numItems) override
    {
        this->m_numberOfItems = numItems;
        if (this->buffered)
        {
            this->buffered.reset();
            this->buffered = this->produceData();
        }
    }
};

} // namespace openPMD
