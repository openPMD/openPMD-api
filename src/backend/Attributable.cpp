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
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include "openPMD/auxiliary/StringManip.hpp"

#include <algorithm>
#include <complex>
#include <iostream>
#include <set>

namespace openPMD
{
namespace internal
{
    AttributableData::AttributableData() : m_writable{this}
    {}
} // namespace internal

Attributable::Attributable() = default;

Attributable::Attributable(std::shared_ptr<internal::AttributableData> attri)
    : m_attri{std::move(attri)}
{}

Attribute Attributable::getAttribute(std::string const &key) const
{
    auto &attri = get();
    auto it = attri.m_attributes.find(key);
    if (it != attri.m_attributes.cend())
        return it->second;

    throw no_such_attribute_error(key);
}

bool Attributable::deleteAttribute(std::string const &key)
{
    auto &attri = get();
    if (Access::READ_ONLY == IOHandler()->m_frontendAccess)
        throw std::runtime_error(
            "Can not delete an Attribute in a read-only Series.");

    auto it = attri.m_attributes.find(key);
    if (it != attri.m_attributes.end())
    {
        Parameter<Operation::DELETE_ATT> aDelete;
        aDelete.name = key;
        IOHandler()->enqueue(IOTask(this, aDelete));
        IOHandler()->flush(internal::defaultFlushParams);
        attri.m_attributes.erase(it);
        return true;
    }
    return false;
}

std::vector<std::string> Attributable::attributes() const
{
    auto &attri = get();
    std::vector<std::string> ret;
    ret.reserve(attri.m_attributes.size());
    for (auto const &entry : attri.m_attributes)
        ret.emplace_back(entry.first);

    return ret;
}

size_t Attributable::numAttributes() const
{
    return get().m_attributes.size();
}

bool Attributable::containsAttribute(std::string const &key) const
{
    auto &attri = get();
    return attri.m_attributes.find(key) != attri.m_attributes.end();
}

std::string Attributable::comment() const
{
    return getAttribute("comment").get<std::string>();
}

Attributable &Attributable::setComment(std::string const &c)
{
    setAttribute("comment", c);
    return *this;
}

void Attributable::seriesFlush(std::string backendConfig)
{
    writable().seriesFlush(std::move(backendConfig));
}

Series Attributable::retrieveSeries() const
{
    Writable const *findSeries = &writable();
    while (findSeries->parent)
    {
        findSeries = findSeries->parent;
    }
    auto seriesData = &auxiliary::deref_dynamic_cast<internal::SeriesData>(
        findSeries->attributable);
    return Series{{seriesData, [](auto const *) {}}};
}

Iteration const &Attributable::containingIteration() const
{
    std::vector<Writable const *> searchQueue;
    searchQueue.reserve(7);
    Writable const *findSeries = &writable();
    while (findSeries)
    {
        searchQueue.push_back(findSeries);
        // we don't need to push the last Writable since it's the Series anyway
        findSeries = findSeries->parent;
    }
    // End of the queue:
    // Iteration -> Series.iterations -> Series
    if (searchQueue.size() < 3)
    {
        throw std::runtime_error(
            "containingIteration(): Must be called for an object contained in "
            "an iteration.");
    }
    auto end = searchQueue.rbegin();
    internal::AttributableData const *attr = (*(end + 2))->attributable;
    if (attr == nullptr)
        throw std::runtime_error(
            "containingIteration(): attributable must not be a nullptr.");
    /*
     * We now know the unique instance of Attributable that corresponds with
     * the iteration.
     * Since the class Iteration itself still follows the old class design,
     * we will have to take a detour via Series.
     */
    auto &series = auxiliary::deref_dynamic_cast<internal::SeriesData>(
        (*searchQueue.rbegin())->attributable);
    for (auto const &pair : series.iterations)
    {
        if (&static_cast<Attributable const &>(pair.second).get() == attr)
        {
            return pair.second;
        }
    }
    throw std::runtime_error(
        "Containing iteration not found in containing Series.");
}

Iteration &Attributable::containingIteration()
{
    return const_cast<Iteration &>(
        static_cast<Attributable const *>(this)->containingIteration());
}

std::string Attributable::MyPath::filePath() const
{
    return directory + seriesName + seriesExtension;
}

auto Attributable::myPath() const -> MyPath
{
    MyPath res;
    Writable const *findSeries = &writable();
    while (findSeries->parent)
    {
        // we don't need to push_back the ownKeyWithinParent of the Series class
        // so it's alright that this loop doesn't ask the key of the last found
        // Writable

        // push these in reverse because we're building the list from the back
        for (auto it = findSeries->ownKeyWithinParent.rbegin();
             it != findSeries->ownKeyWithinParent.rend();
             ++it)
        {
            res.group.push_back(*it);
        }
        findSeries = findSeries->parent;
    }
    std::reverse(res.group.begin(), res.group.end());
    auto &seriesData = auxiliary::deref_dynamic_cast<internal::SeriesData>(
        findSeries->attributable);
    Series series{{&seriesData, [](auto const *) {}}};
    res.seriesName = series.name();
    res.seriesExtension = suffix(seriesData.m_format);
    res.directory = IOHandler()->directory;
    return res;
}

void Attributable::seriesFlush(internal::FlushParams flushParams)
{
    writable().seriesFlush(flushParams);
}

void Attributable::flushAttributes(internal::FlushParams const &flushParams)
{
    switch (flushParams.flushLevel)
    {
    case FlushLevel::SkeletonOnly:
    case FlushLevel::CreateOrOpenFiles:
        return;
    case FlushLevel::InternalFlush:
    case FlushLevel::UserFlush:
        // pass
        break;
    }
    if (dirty())
    {
        Parameter<Operation::WRITE_ATT> aWrite;
        for (std::string const &att_name : attributes())
        {
            aWrite.name = att_name;
            aWrite.resource = getAttribute(att_name).getResource();
            aWrite.dtype = getAttribute(att_name).dtype;
            IOHandler()->enqueue(IOTask(this, aWrite));
        }

        dirty() = false;
    }
}

void Attributable::readAttributes(ReadMode mode)
{
    auto &attri = get();
    Parameter<Operation::LIST_ATTS> aList;
    IOHandler()->enqueue(IOTask(this, aList));
    IOHandler()->flush(internal::defaultFlushParams);
    std::vector<std::string> written_attributes = attributes();

    /* std::set_difference requires sorted ranges */
    std::sort(aList.attributes->begin(), aList.attributes->end());
    std::sort(written_attributes.begin(), written_attributes.end());

    std::set<std::string> tmpAttributes;
    switch (mode)
    {
    case ReadMode::IgnoreExisting:
        // reread: aList - written_attributes
        std::set_difference(
            aList.attributes->begin(),
            aList.attributes->end(),
            written_attributes.begin(),
            written_attributes.end(),
            std::inserter(tmpAttributes, tmpAttributes.begin()));
        break;
    case ReadMode::OverrideExisting:
        tmpAttributes = std::set<std::string>(
            aList.attributes->begin(), aList.attributes->end());
        break;
    case ReadMode::FullyReread:
        attri.m_attributes.clear();
        tmpAttributes = std::set<std::string>(
            aList.attributes->begin(), aList.attributes->end());
        break;
    }

    using DT = Datatype;
    Parameter<Operation::READ_ATT> aRead;

    for (auto const &att_name : tmpAttributes)
    {
        aRead.name = att_name;
        std::string att = auxiliary::strip(att_name, {'\0'});
        IOHandler()->enqueue(IOTask(this, aRead));
        try
        {
            IOHandler()->flush(internal::defaultFlushParams);
        }
        catch (unsupported_data_error const &e)
        {
            std::cerr << "Skipping non-standard attribute " << att << " ("
                      << e.what() << ")\n";
            continue;
        }
        Attribute a(*aRead.resource);

        auto guardUnitDimension = [this](std::string const &key, auto vector) {
            if (key == "unitDimension")
            {
                // Some backends may report the wrong type when reading
                if (vector.size() != 7)
                {
                    throw error::ReadError(
                        error::AffectedObject::Attribute,
                        error::Reason::UnexpectedContent,
                        {},
                        "[Attributable] Unexpected datatype for unitDimension "
                        "(supplied vector has " +
                            std::to_string(vector.size()) +
                            " entries, but 7 are expected).");
                }
                std::array<double, 7> arr;
                std::copy_n(vector.begin(), 7, arr.begin());
                setAttribute(key, std::move(arr));
            }
            else
            {
                setAttribute(key, std::move(vector));
            }
        };

        switch (*aRead.dtype)
        {
        case DT::CHAR:
            setAttribute(att, a.get<char>());
            break;
        case DT::UCHAR:
            setAttribute(att, a.get<unsigned char>());
            break;
        case DT::SCHAR:
            setAttribute(att, a.get<signed char>());
            break;
        case DT::SHORT:
            setAttribute(att, a.get<short>());
            break;
        case DT::INT:
            setAttribute(att, a.get<int>());
            break;
        case DT::LONG:
            setAttribute(att, a.get<long>());
            break;
        case DT::LONGLONG:
            setAttribute(att, a.get<long long>());
            break;
        case DT::USHORT:
            setAttribute(att, a.get<unsigned short>());
            break;
        case DT::UINT:
            setAttribute(att, a.get<unsigned int>());
            break;
        case DT::ULONG:
            setAttribute(att, a.get<unsigned long>());
            break;
        case DT::ULONGLONG:
            setAttribute(att, a.get<unsigned long long>());
            break;
        case DT::FLOAT:
            setAttribute(att, a.get<float>());
            break;
        case DT::DOUBLE:
            setAttribute(att, a.get<double>());
            break;
        case DT::LONG_DOUBLE:
            setAttribute(att, a.get<long double>());
            break;
        case DT::CFLOAT:
            setAttribute(att, a.get<std::complex<float> >());
            break;
        case DT::CDOUBLE:
            setAttribute(att, a.get<std::complex<double> >());
            break;
        case DT::CLONG_DOUBLE:
            setAttribute(att, a.get<std::complex<long double> >());
            break;
        case DT::STRING:
            setAttribute(att, a.get<std::string>());
            break;
        case DT::VEC_CHAR:
            setAttribute(att, a.get<std::vector<char> >());
            break;
        case DT::VEC_SHORT:
            setAttribute(att, a.get<std::vector<short> >());
            break;
        case DT::VEC_INT:
            setAttribute(att, a.get<std::vector<int> >());
            break;
        case DT::VEC_LONG:
            setAttribute(att, a.get<std::vector<long> >());
            break;
        case DT::VEC_LONGLONG:
            setAttribute(att, a.get<std::vector<long long> >());
            break;
        case DT::VEC_UCHAR:
            setAttribute(att, a.get<std::vector<unsigned char> >());
            break;
        case DT::VEC_USHORT:
            setAttribute(att, a.get<std::vector<unsigned short> >());
            break;
        case DT::VEC_UINT:
            setAttribute(att, a.get<std::vector<unsigned int> >());
            break;
        case DT::VEC_ULONG:
            setAttribute(att, a.get<std::vector<unsigned long> >());
            break;
        case DT::VEC_ULONGLONG:
            setAttribute(att, a.get<std::vector<unsigned long long> >());
            break;
        case DT::VEC_FLOAT:
            guardUnitDimension(att, a.get<std::vector<float> >());
            break;
        case DT::VEC_DOUBLE:
            guardUnitDimension(att, a.get<std::vector<double> >());
            break;
        case DT::VEC_LONG_DOUBLE:
            guardUnitDimension(att, a.get<std::vector<long double> >());
            break;
        case DT::VEC_CFLOAT:
            setAttribute(att, a.get<std::vector<std::complex<float> > >());
            break;
        case DT::VEC_CDOUBLE:
            setAttribute(att, a.get<std::vector<std::complex<double> > >());
            break;
        case DT::VEC_CLONG_DOUBLE:
            setAttribute(
                att, a.get<std::vector<std::complex<long double> > >());
            break;
        case DT::VEC_SCHAR:
            setAttribute(att, a.get<std::vector<signed char> >());
            break;
        case DT::VEC_STRING:
            setAttribute(att, a.get<std::vector<std::string> >());
            break;
        case DT::ARR_DBL_7:
            setAttribute(att, a.get<std::array<double, 7> >());
            break;
        case DT::BOOL:
            setAttribute(att, a.get<bool>());
            break;
        case DT::UNDEFINED:
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Undefined Attribute datatype during read");
        }
    }

    dirty() = false;
}

void Attributable::linkHierarchy(Writable &w)
{
    auto handler = w.IOHandler;
    writable().IOHandler = handler;
    writable().parent = &w;
}
} // namespace openPMD
