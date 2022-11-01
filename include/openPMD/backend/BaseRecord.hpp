/* Copyright 2017-2021 Fabian Koller
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

#include "openPMD/RecordComponent.hpp"
#include "openPMD/UnitDimension.hpp"
#include "openPMD/backend/Container.hpp"

#include <array>
#include <stdexcept>
#include <string>

namespace openPMD
{
namespace internal
{
    template <typename T_elem>
    class BaseRecordData : public ContainerData<T_elem>
    {
    public:
        /**
         * True if this Record contains a scalar record component.
         * If so, then that record component is the only component contained,
         * and the last hierarchical layer is skipped (i.e. only one OPEN_PATH
         * task for Record and RecordComponent).
         */
        bool m_containsScalar = false;

        BaseRecordData();

        BaseRecordData(BaseRecordData const &) = delete;
        BaseRecordData(BaseRecordData &&) = delete;

        BaseRecordData &operator=(BaseRecordData const &) = delete;
        BaseRecordData &operator=(BaseRecordData &&) = delete;
    };
} // namespace internal

template <typename T_elem>
class BaseRecord : public Container<T_elem>
{
    friend class Iteration;
    friend class ParticleSpecies;
    friend class PatchRecord;
    friend class Record;
    friend class Mesh;

    std::shared_ptr<internal::BaseRecordData<T_elem> > m_baseRecordData{
        new internal::BaseRecordData<T_elem>()};

    inline internal::BaseRecordData<T_elem> &get()
    {
        return *m_baseRecordData;
    }

    inline internal::BaseRecordData<T_elem> const &get() const
    {
        return *m_baseRecordData;
    }

    BaseRecord();

protected:
    BaseRecord(std::shared_ptr<internal::BaseRecordData<T_elem> >);

    inline void setData(internal::BaseRecordData<T_elem> *data)
    {
        m_baseRecordData = std::move(data);
        Container<T_elem>::setData(m_baseRecordData);
    }

public:
    using key_type = typename Container<T_elem>::key_type;
    using mapped_type = typename Container<T_elem>::mapped_type;
    using value_type = typename Container<T_elem>::value_type;
    using size_type = typename Container<T_elem>::size_type;
    using difference_type = typename Container<T_elem>::difference_type;
    using allocator_type = typename Container<T_elem>::allocator_type;
    using reference = typename Container<T_elem>::reference;
    using const_reference = typename Container<T_elem>::const_reference;
    using pointer = typename Container<T_elem>::pointer;
    using const_pointer = typename Container<T_elem>::const_pointer;
    using iterator = typename Container<T_elem>::iterator;
    using const_iterator = typename Container<T_elem>::const_iterator;

    virtual ~BaseRecord() = default;

    mapped_type &operator[](key_type const &key) override;
    mapped_type &operator[](key_type &&key) override;
    size_type erase(key_type const &key) override;
    iterator erase(iterator res) override;
    //! @todo add also, as soon as added in Container:
    // iterator erase(const_iterator first, const_iterator last) override;

    /** Return the physical dimension (quantity) of a record
     *
     * Annotating the physical dimension of a record allows us to read data
     * sets with arbitrary names and understand their purpose simply by
     * dimensional analysis. The dimensional base quantities in openPMD are
     * in order: length (L), mass (M), time (T), electric current (I),
     * thermodynamic temperature (theta), amount of substance (N),
     * luminous intensity (J) after the international system of quantities
     * (ISQ).
     *
     * @see https://en.wikipedia.org/wiki/Dimensional_analysis
     * @see
     * https://en.wikipedia.org/wiki/International_System_of_Quantities#Base_quantities
     * @see
     * https://github.com/openPMD/openPMD-standard/blob/1.1.0/STANDARD.md#required-for-each-record
     *
     * @return powers of the 7 base measures in the order specified above
     */
    std::array<double, 7> unitDimension() const;

    /** Returns true if this record only contains a single component
     *
     * @return true if a record with only a single component
     */
    bool scalar() const;

protected:
    BaseRecord(internal::BaseRecordData<T_elem> *);
    void readBase();

private:
    void flush(std::string const &, internal::FlushParams const &) final;
    virtual void
    flush_impl(std::string const &, internal::FlushParams const &) = 0;
    virtual void read() = 0;

    /**
     * @brief Check recursively whether this BaseRecord is dirty.
     *        It is dirty if any attribute or dataset is read from or written to
     *        the backend.
     *
     * @return true If dirty.
     * @return false Otherwise.
     */
    bool dirtyRecursive() const;
}; // BaseRecord

// implementation

namespace internal
{
    template <typename T_elem>
    BaseRecordData<T_elem>::BaseRecordData()
    {
        Attributable impl{{this, [](auto const *) {}}};
        impl.setAttribute(
            "unitDimension",
            std::array<double, 7>{{0., 0., 0., 0., 0., 0., 0.}});
    }
} // namespace internal

template <typename T_elem>
BaseRecord<T_elem>::BaseRecord() : Container<T_elem>{nullptr}
{
    Container<T_elem>::setData(m_baseRecordData);
}

template <typename T_elem>
BaseRecord<T_elem>::BaseRecord(
    std::shared_ptr<internal::BaseRecordData<T_elem> > data)
    : Container<T_elem>{data}, m_baseRecordData{std::move(data)}
{}

template <typename T_elem>
inline typename BaseRecord<T_elem>::mapped_type &
BaseRecord<T_elem>::operator[](key_type const &key)
{
    auto it = this->find(key);
    if (it != this->end())
        return it->second;
    else
    {
        bool const keyScalar = (key == RecordComponent::SCALAR);
        if ((keyScalar && !Container<T_elem>::empty() && !scalar()) ||
            (scalar() && !keyScalar))
            throw std::runtime_error(
                "A scalar component can not be contained at "
                "the same time as one or more regular components.");

        mapped_type &ret = Container<T_elem>::operator[](key);
        if (keyScalar)
        {
            get().m_containsScalar = true;
            ret.parent() = this->parent();
        }
        return ret;
    }
}

template <typename T_elem>
inline typename BaseRecord<T_elem>::mapped_type &
BaseRecord<T_elem>::operator[](key_type &&key)
{
    auto it = this->find(key);
    if (it != this->end())
        return it->second;
    else
    {
        bool const keyScalar = (key == RecordComponent::SCALAR);
        if ((keyScalar && !Container<T_elem>::empty() && !scalar()) ||
            (scalar() && !keyScalar))
            throw std::runtime_error(
                "A scalar component can not be contained at "
                "the same time as one or more regular components.");

        mapped_type &ret = Container<T_elem>::operator[](std::move(key));
        if (keyScalar)
        {
            get().m_containsScalar = true;
            ret.parent() = this->parent();
        }
        return ret;
    }
}

template <typename T_elem>
inline typename BaseRecord<T_elem>::size_type
BaseRecord<T_elem>::erase(key_type const &key)
{
    bool const keyScalar = (key == RecordComponent::SCALAR);
    size_type res;
    if (!keyScalar || (keyScalar && this->at(key).constant()))
        res = Container<T_elem>::erase(key);
    else
    {
        mapped_type &rc = this->find(RecordComponent::SCALAR)->second;
        if (rc.written())
        {
            Parameter<Operation::DELETE_DATASET> dDelete;
            dDelete.name = ".";
            this->IOHandler()->enqueue(IOTask(&rc, dDelete));
            this->IOHandler()->flush(internal::defaultFlushParams);
        }
        res = Container<T_elem>::erase(key);
    }

    if (keyScalar)
    {
        this->written() = false;
        this->writable().abstractFilePosition.reset();
        this->get().m_containsScalar = false;
    }
    return res;
}

template <typename T_elem>
inline typename BaseRecord<T_elem>::iterator
BaseRecord<T_elem>::erase(iterator res)
{
    bool const keyScalar = (res->first == RecordComponent::SCALAR);
    iterator ret;
    if (!keyScalar || (keyScalar && this->at(res->first).constant()))
        ret = Container<T_elem>::erase(res);
    else
    {
        mapped_type &rc = this->find(RecordComponent::SCALAR)->second;
        if (rc.written())
        {
            Parameter<Operation::DELETE_DATASET> dDelete;
            dDelete.name = ".";
            this->IOHandler()->enqueue(IOTask(&rc, dDelete));
            this->IOHandler()->flush(internal::defaultFlushParams);
        }
        ret = Container<T_elem>::erase(res);
    }

    if (keyScalar)
    {
        this->written() = false;
        this->writable().abstractFilePosition.reset();
        this->get().m_containsScalar = false;
    }
    return ret;
}

template <typename T_elem>
inline std::array<double, 7> BaseRecord<T_elem>::unitDimension() const
{
    return this->getAttribute("unitDimension")
        .template get<std::array<double, 7> >();
}

template <typename T_elem>
inline bool BaseRecord<T_elem>::scalar() const
{
    return get().m_containsScalar;
}

template <typename T_elem>
inline void BaseRecord<T_elem>::readBase()
{
    using DT = Datatype;
    Parameter<Operation::READ_ATT> aRead;

    aRead.name = "unitDimension";
    this->IOHandler()->enqueue(IOTask(this, aRead));
    this->IOHandler()->flush(internal::defaultFlushParams);
    if (auto val =
            Attribute(*aRead.resource).getOptional<std::array<double, 7> >();
        val.has_value())
        this->setAttribute("unitDimension", val.value());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'unitDimension'");

    aRead.name = "timeOffset";
    this->IOHandler()->enqueue(IOTask(this, aRead));
    this->IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::FLOAT)
        this->setAttribute(
            "timeOffset", Attribute(*aRead.resource).get<float>());
    else if (*aRead.dtype == DT::DOUBLE)
        this->setAttribute(
            "timeOffset", Attribute(*aRead.resource).get<double>());
    // conversion cast if a backend reports an integer type
    else if (auto val = Attribute(*aRead.resource).getOptional<double>();
             val.has_value())
        this->setAttribute("timeOffset", val.value());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'timeOffset'");
}

template <typename T_elem>
inline void BaseRecord<T_elem>::flush(
    std::string const &name, internal::FlushParams const &flushParams)
{
    if (!this->written() && this->empty())
        throw std::runtime_error(
            "A Record can not be written without any contained "
            "RecordComponents: " +
            name);

    this->flush_impl(name, flushParams);
    // flush_impl must take care to correctly set the dirty() flag so this
    // method doesn't do it
}

template <typename T_elem>
inline bool BaseRecord<T_elem>::dirtyRecursive() const
{
    if (this->dirty())
    {
        return true;
    }
    for (auto const &pair : *this)
    {
        if (pair.second.dirtyRecursive())
        {
            return true;
        }
    }
    return false;
}
} // namespace openPMD
