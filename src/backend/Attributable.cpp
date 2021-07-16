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
AttributableData::AttributableData() : m_writable{ this }
{
}
}

AttributableImpl::AttributableImpl( internal::AttributableData * attri )
    : m_attri{ attri }
{
}

Attribute
AttributableImpl::getAttribute(std::string const& key) const
{
    auto & attri = get();
    auto it = attri.m_attributes.find(key);
    if( it != attri.m_attributes.cend() )
        return it->second;

    throw no_such_attribute_error(key);
}

bool
AttributableImpl::deleteAttribute(std::string const& key)
{
    auto & attri = get();
    if(Access::READ_ONLY == IOHandler()->m_frontendAccess )
        throw std::runtime_error("Can not delete an Attribute in a read-only Series.");

    auto it = attri.m_attributes.find(key);
    if( it != attri.m_attributes.end() )
    {
        Parameter< Operation::DELETE_ATT > aDelete;
        aDelete.name = key;
        IOHandler()->enqueue(IOTask(this, aDelete));
        IOHandler()->flush();
        attri.m_attributes.erase(it);
        return true;
    }
    return false;
}

std::vector< std::string >
AttributableImpl::attributes() const
{
    auto & attri = get();
    std::vector< std::string > ret;
    ret.reserve(attri.m_attributes.size());
    for( auto const& entry : attri.m_attributes )
        ret.emplace_back(entry.first);

    return ret;
}

size_t
AttributableImpl::numAttributes() const
{
    return get().m_attributes.size();
}

bool
AttributableImpl::containsAttribute(std::string const &key) const
{
    auto & attri = get();
    return attri.m_attributes.find(key) != attri.m_attributes.end();
}

std::string
AttributableImpl::comment() const
{
    return getAttribute("comment").get< std::string >();
}

AttributableImpl&
AttributableImpl::setComment(std::string const& c)
{
    setAttribute("comment", c);
    return *this;
}

void
AttributableImpl::seriesFlush()
{
    writable().seriesFlush();
}

internal::SeriesInternal const & AttributableImpl::retrieveSeries() const
{
    Writable const * findSeries = &writable();
    while( findSeries->parent )
    {
        findSeries = findSeries->parent;
    }
    return auxiliary::deref_dynamic_cast< internal::SeriesInternal >(
        findSeries->attributable );
}

internal::SeriesInternal & AttributableImpl::retrieveSeries()
{
    return const_cast< internal::SeriesInternal & >(
        static_cast< AttributableImpl const * >( this )->retrieveSeries() );
}

Iteration const & AttributableImpl::containingIteration() const
{
    std::vector< Writable const * > searchQueue;
    searchQueue.reserve( 7 );
    Writable const * findSeries = &writable();
    while( findSeries )
    {
        searchQueue.push_back( findSeries );
        // we don't need to push the last Writable since it's the Series anyway
        findSeries = findSeries->parent;
    }
    // End of the queue:
    // Iteration -> Series.iterations -> Series
    if( searchQueue.size() < 3 )
    {
        throw std::runtime_error(
            "containingIteration(): Must be called for an object contained in "
            "an iteration." );
    }
    auto end = searchQueue.rbegin();
    internal::AttributableData const * attr = ( *( end + 2 ) )->attributable;
    /*
     * We now know the unique instance of Attributable that corresponds with
     * the iteration.
     * Since the class Iteration itself still follows the old class design,
     * we will have to take a detour via Series.
     */
    auto & series = auxiliary::deref_dynamic_cast< internal::SeriesInternal >(
        ( *searchQueue.rbegin() )->attributable );
    for( auto const & pair : series.iterations )
    {
        if( &pair.second.get() == attr )
        {
            return pair.second;
        }
    }
    throw std::runtime_error(
        "Containing iteration not found in containing Series." );
}

Iteration & AttributableImpl::containingIteration()
{
    return const_cast< Iteration & >(
        static_cast< AttributableImpl const * >( this )
            ->containingIteration() );
}

std::string Attributable::MyPath::filePath() const
{
    return directory + seriesName + seriesExtension;
}

auto AttributableImpl::myPath() const -> MyPath
{
    MyPath res;
    Writable const * findSeries = &writable();
    while( findSeries->parent )
    {
        // we don't need to push_back the ownKeyWithinParent of the Series class
        // so it's alright that this loop doesn't ask the key of the last found
        // Writable

        // push these in reverse because we're building the list from the back
        for( auto it = findSeries->ownKeyWithinParent.rbegin();
             it != findSeries->ownKeyWithinParent.rend();
             ++it )
        {
            res.group.push_back(*it );
        }
        findSeries = findSeries->parent;
    }
    std::reverse(res.group.begin(), res.group.end() );
    auto const & series =
        auxiliary::deref_dynamic_cast< internal::SeriesInternal >(
            findSeries->attributable );
    res.seriesName = series.name();
    res.seriesExtension = suffix( series.m_format );
    res.directory = IOHandler()->directory;
    return res;
}

void
AttributableImpl::seriesFlush( FlushLevel level )
{
    writable().seriesFlush( level );
}

void
AttributableImpl::flushAttributes()
{
    if( IOHandler()->m_flushLevel == FlushLevel::SkeletonOnly )
    {
        return;
    }
    if( dirty() )
    {
        Parameter< Operation::WRITE_ATT > aWrite;
        for( std::string const & att_name : attributes() )
        {
            aWrite.name = att_name;
            aWrite.resource = getAttribute(att_name).getResource();
            aWrite.dtype = getAttribute(att_name).dtype;
            IOHandler()->enqueue(IOTask(this, aWrite));
        }

        dirty() = false;
    }
}

void
AttributableImpl::readAttributes( ReadMode mode )
{
    auto & attri = get();
    Parameter< Operation::LIST_ATTS > aList;
    IOHandler()->enqueue(IOTask(this, aList));
    IOHandler()->flush();
    std::vector< std::string > written_attributes = attributes();

    /* std::set_difference requires sorted ranges */
    std::sort(aList.attributes->begin(), aList.attributes->end());
    std::sort(written_attributes.begin(), written_attributes.end());

    std::set< std::string > tmpAttributes;
    switch( mode )
    {
    case ReadMode::IgnoreExisting:
        // reread: aList - written_attributes
        std::set_difference(
            aList.attributes->begin(), aList.attributes->end(),
            written_attributes.begin(), written_attributes.end(),
            std::inserter(tmpAttributes, tmpAttributes.begin()));
        break;
    case ReadMode::OverrideExisting:
        tmpAttributes = std::set< std::string >(
            aList.attributes->begin(),
            aList.attributes->end() );
        break;
    case ReadMode::FullyReread:
        attri.m_attributes.clear();
        tmpAttributes = std::set< std::string >(
            aList.attributes->begin(),
            aList.attributes->end() );
        break;
    }

    using DT = Datatype;
    Parameter< Operation::READ_ATT > aRead;

    for( auto const& att_name : tmpAttributes )
    {
        aRead.name = att_name;
        std::string att = auxiliary::strip(att_name, {'\0'});
        IOHandler()->enqueue(IOTask(this, aRead));
        try
        {
            IOHandler()->flush();
        } catch( unsupported_data_error const& e )
        {
            std::cerr << "Skipping non-standard attribute "
                      << att << " ("
                      << e.what()
                      << ")\n";
            continue;
        }
        Attribute a(*aRead.resource);

        auto guardUnitDimension =
            [ this ]( std::string const & key, auto vector )
        {
            if( key == "unitDimension" )
            {
                // Some backends may report the wrong type when reading
                if( vector.size() != 7 )
                {
                    throw std::runtime_error(
                        "[Attributable] "
                        "Unexpected datatype for unitDimension." );
                }
                std::array< double, 7 > arr;
                std::copy_n( vector.begin(), 7, arr.begin() );
                setAttribute( key, std::move( arr ) );
            }
            else
            {
                setAttribute( key, std::move( vector ) );
            }
        };

        switch( *aRead.dtype )
        {
            case DT::CHAR:
                setAttribute(att, a.get< char >());
                break;
            case DT::UCHAR:
                setAttribute(att, a.get< unsigned char >());
                break;
            case DT::SHORT:
                setAttribute(att, a.get< short >());
                break;
            case DT::INT:
                setAttribute(att, a.get< int >());
                break;
            case DT::LONG:
                setAttribute(att, a.get< long >());
                break;
            case DT::LONGLONG:
                setAttribute(att, a.get< long long >());
                break;
            case DT::USHORT:
                setAttribute(att, a.get< unsigned short >());
                break;
            case DT::UINT:
                setAttribute(att, a.get< unsigned int >());
                break;
            case DT::ULONG:
                setAttribute(att, a.get< unsigned long >());
                break;
            case DT::ULONGLONG:
                setAttribute(att, a.get< unsigned long long >());
                break;
            case DT::FLOAT:
                setAttribute(att, a.get< float >());
                break;
            case DT::DOUBLE:
                setAttribute(att, a.get< double >());
                break;
            case DT::LONG_DOUBLE:
                setAttribute(att, a.get< long double >());
                break;
            case DT::CFLOAT:
                setAttribute(att, a.get< std::complex< float > >());
                break;
            case DT::CDOUBLE:
                setAttribute(att, a.get< std::complex< double > >());
                break;
            case DT::CLONG_DOUBLE:
                setAttribute(att, a.get< std::complex< long double > >());
                break;
            case DT::STRING:
                setAttribute(att, a.get< std::string >());
                break;
            case DT::VEC_CHAR:
                setAttribute(att, a.get< std::vector< char > >());
                break;
            case DT::VEC_SHORT:
                setAttribute(att, a.get< std::vector< short > >());
                break;
            case DT::VEC_INT:
                setAttribute(att, a.get< std::vector< int > >());
                break;
            case DT::VEC_LONG:
                setAttribute(att, a.get< std::vector< long > >());
                break;
            case DT::VEC_LONGLONG:
                setAttribute(att, a.get< std::vector< long long > >());
                break;
            case DT::VEC_UCHAR:
                setAttribute(att, a.get< std::vector< unsigned char > >());
                break;
            case DT::VEC_USHORT:
                setAttribute(att, a.get< std::vector< unsigned short > >());
                break;
            case DT::VEC_UINT:
                setAttribute(att, a.get< std::vector< unsigned int > >());
                break;
            case DT::VEC_ULONG:
                setAttribute(att, a.get< std::vector< unsigned long > >());
                break;
            case DT::VEC_ULONGLONG:
                setAttribute(att, a.get< std::vector< unsigned long long > >());
                break;
            case DT::VEC_FLOAT:
                guardUnitDimension( att, a.get< std::vector< float > >() );
                break;
            case DT::VEC_DOUBLE:
                guardUnitDimension( att, a.get< std::vector< double > >() );
                break;
            case DT::VEC_LONG_DOUBLE:
                guardUnitDimension( att, a.get< std::vector< long double > >() );
                break;
            case DT::VEC_CFLOAT:
                setAttribute(att, a.get< std::vector< std::complex< float > > >());
                break;
            case DT::VEC_CDOUBLE:
                setAttribute(att, a.get< std::vector< std::complex< double > > >());
                break;
            case DT::VEC_CLONG_DOUBLE:
                setAttribute(att, a.get< std::vector< std::complex< long double > > >());
                break;
            case DT::VEC_STRING:
                setAttribute(att, a.get< std::vector< std::string > >());
                break;
            case DT::ARR_DBL_7:
                setAttribute(att, a.get< std::array< double, 7 > >());
                break;
            case DT::BOOL:
                setAttribute(att, a.get< bool >());
                break;
            case DT::DATATYPE:
            case DT::UNDEFINED:
                throw std::runtime_error("Invalid Attribute datatype during read");
        }
    }

    dirty() = false;
}

void
AttributableImpl::linkHierarchy(Writable& w)
{
    auto handler = w.IOHandler;
    writable().IOHandler = handler;
    writable().parent = &w;
}
} // openPMD
