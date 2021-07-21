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
#include "openPMD/Iteration.hpp"
#include "openPMD/Dataset.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Writable.hpp"

#include <exception>
#include <tuple>


namespace openPMD
{
Iteration::Iteration()
        : meshes{Container< Mesh >()},
          particles{Container< ParticleSpecies >()}
{
    setTime(static_cast< double >(0));
    setDt(static_cast< double >(1));
    setTimeUnitSI(1);
    meshes.writable().ownKeyWithinParent = { "meshes" };
    particles.writable().ownKeyWithinParent = { "particles" };
}

template< typename T >
Iteration&
Iteration::setTime(T newTime)
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    setAttribute("time", newTime);
    return *this;
}

template< typename T >
Iteration&
Iteration::setDt(T newDt)
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    setAttribute("dt", newDt);
    return *this;
}

double
Iteration::timeUnitSI() const
{
    return getAttribute("timeUnitSI").get< double >();
}

Iteration&
Iteration::setTimeUnitSI(double newTimeUnitSI)
{
    setAttribute("timeUnitSI", newTimeUnitSI);
    return *this;
}

using iterator_t = Container< Iteration, uint64_t >::iterator;

Iteration &
Iteration::close( bool _flush )
{
    using bool_type = unsigned char;
    if( this->IOHandler()->m_frontendAccess != Access::READ_ONLY )
    {
        setAttribute< bool_type >( "closed", 1u );
    }
    StepStatus flag = getStepStatus();
    // update close status
    switch( *m_closed )
    {
        case CloseStatus::Open:
        case CloseStatus::ClosedInFrontend:
            *m_closed = CloseStatus::ClosedInFrontend;
            break;
        case CloseStatus::ClosedTemporarily:
            // should we bother to reopen?
            if( dirtyRecursive() )
            {
                // let's reopen
                *m_closed = CloseStatus::ClosedInFrontend;
            }
            else
            {
                // don't reopen
                *m_closed = CloseStatus::ClosedInBackend;
            }
            break;
        case CloseStatus::ParseAccessDeferred:
        case CloseStatus::ClosedInBackend:
            // just keep it like it is
            // (this means that closing an iteration that has not been parsed
            // yet keeps it re-openable)
            break;
    }
    if( _flush )
    {
        if( flag == StepStatus::DuringStep )
        {
            endStep();
            setStepStatus( StepStatus::NoStep );
        }
        else
        {
            // flush things manually
            internal::SeriesInternal * s = &retrieveSeries();
            // figure out my iteration number
            auto begin = s->indexOf( *this );
            auto end = begin;
            ++end;

            s->flush_impl( begin, end, FlushLevel::UserFlush );
        }
    }
    else
    {
        if( flag == StepStatus::DuringStep )
        {
            throw std::runtime_error( "Using deferred Iteration::close "
                                      "unimplemented in auto-stepping mode." );
        }
    }
    return *this;
}

Iteration &
Iteration::open()
{
    if( *m_closed == CloseStatus::ParseAccessDeferred )
    {
        *m_closed = CloseStatus::Open;
    }
    runDeferredParseAccess();
    internal::SeriesInternal * s = &retrieveSeries();
    // figure out my iteration number
    auto begin = s->indexOf( *this );
    s->openIteration( begin->first, *this );
    IOHandler()->flush();
    return *this;
}

bool
Iteration::closed() const
{
    switch( *m_closed )
    {
        case CloseStatus::ParseAccessDeferred:
        case CloseStatus::Open:
        /*
         * Temporarily closing a file is something that the openPMD API
         * does for optimization purposes.
         * Logically to the user, it is still open.
         */
        case CloseStatus::ClosedTemporarily:
            return false;
        case CloseStatus::ClosedInFrontend:
        case CloseStatus::ClosedInBackend:
            return true;
    }
    throw std::runtime_error( "Unreachable!" );
}

bool
Iteration::closedByWriter() const
{
    using bool_type = unsigned char;
    if( containsAttribute( "closed" ) )
    {
        return getAttribute( "closed" ).get< bool_type >() == 0u ? false : true;
    }
    else
    {
        return false;
    }
}

void
Iteration::flushFileBased(std::string const& filename, uint64_t i)
{
    /* Find the root point [Series] of this file,
     * meshesPath and particlesPath are stored there */
    internal::SeriesInternal * s = &retrieveSeries();
    if( s == nullptr )
        throw std::runtime_error("[Iteration::flushFileBased] Series* is a nullptr");

    if( !written() )
    {
        /* create file */
        Parameter< Operation::CREATE_FILE > fCreate;
        fCreate.name = filename;
        IOHandler()->enqueue(IOTask(s, fCreate));

        /* create basePath */
        Parameter< Operation::CREATE_PATH > pCreate;
        pCreate.path = auxiliary::replace_first(s->basePath(), "%T/", "");
        IOHandler()->enqueue(IOTask(&s->iterations, pCreate));

        /* create iteration path */
        pCreate.path = std::to_string(i);
        IOHandler()->enqueue(IOTask(this, pCreate));
    } else
    {
        // operations for create mode
        if((IOHandler()->m_frontendAccess == Access::CREATE ) &&
           ( (IOHandler()->backendName() == "MPI_ADIOS1") || (IOHandler()->backendName() == "ADIOS1") ) )
        {
            Parameter< Operation::OPEN_FILE > fOpen;
            fOpen.name = filename;
            fOpen.encoding = IterationEncoding::fileBased;
            IOHandler()->enqueue(IOTask(s, fOpen));
            flush();

            return;
        }

        // operations for read/read-write mode
        /* open file */
        s->openIteration( i, *this );
    }

    flush();
}

void
Iteration::flushGroupBased(uint64_t i)
{
    if( !written() )
    {
        /* create iteration path */
        Parameter< Operation::CREATE_PATH > pCreate;
        pCreate.path = std::to_string(i);
        IOHandler()->enqueue(IOTask(this, pCreate));
    }

    flush();
}

void
Iteration::flushVariableBased( uint64_t i )
{
    if( !written() )
    {
        /* create iteration path */
        Parameter< Operation::OPEN_PATH > pOpen;
        pOpen.path = "";
        IOHandler()->enqueue( IOTask( this, pOpen ) );
        this->setAttribute( "snapshot", i );
    }

    flush();
}

void
Iteration::flush()
{
    if(IOHandler()->m_frontendAccess == Access::READ_ONLY )
    {
        for( auto& m : meshes )
            m.second.flush(m.first);
        for( auto& species : particles )
            species.second.flush(species.first);
    } else
    {
        /* Find the root point [Series] of this file,
         * meshesPath and particlesPath are stored there */
        internal::SeriesInternal * s = &retrieveSeries();

        if( !meshes.empty() || s->containsAttribute("meshesPath") )
        {
            if( !s->containsAttribute("meshesPath") )
            {
                s->setMeshesPath("meshes/");
                s->flushMeshesPath();
            }
            meshes.flush(s->meshesPath());
            for( auto& m : meshes )
                m.second.flush(m.first);
        }
        else
        {
            meshes.dirty() = false;
        }

        if( !particles.empty() || s->containsAttribute("particlesPath") )
        {
            if( !s->containsAttribute("particlesPath") )
            {
                s->setParticlesPath("particles/");
                s->flushParticlesPath();
            }
            particles.flush(s->particlesPath());
            for( auto& species : particles )
                species.second.flush(species.first);
        }
        else
        {
            particles.dirty() = false;
        }

        flushAttributes();
    }
}

void Iteration::deferParseAccess( DeferredParseAccess dr )
{
    *m_deferredParseAccess =
        auxiliary::makeOption< DeferredParseAccess >( std::move( dr ) );
}

void Iteration::read()
{
    if( !m_deferredParseAccess->has_value() )
    {
        return;
    }
    auto const & deferred = m_deferredParseAccess->get();
    if( deferred.fileBased )
    {
        readFileBased( deferred.filename, deferred.path );
    }
    else
    {
        readGorVBased( deferred.path );
    }
    // reset this thing
    *m_deferredParseAccess = auxiliary::Option< DeferredParseAccess >();
}

void Iteration::reread( std::string const & path )
{
    if( m_deferredParseAccess->has_value() )
    {
        throw std::runtime_error(
            "[Iteration] Internal control flow error: Trying to reread an "
            "iteration that has not yet been read for its first time." );
    }
    read_impl( path );
}

void Iteration::readFileBased(
    std::string filePath, std::string const & groupPath )
{
    auto & series = retrieveSeries();

    series.readOneIterationFileBased( filePath );

    read_impl( groupPath );
}

void Iteration::readGorVBased( std::string const & groupPath )
{

    read_impl(groupPath );
}

void Iteration::read_impl( std::string const & groupPath )
{
    Parameter< Operation::OPEN_PATH > pOpen;
    pOpen.path = groupPath;
    IOHandler()->enqueue( IOTask( this, pOpen ) );

    using DT = Datatype;
    Parameter< Operation::READ_ATT > aRead;

    aRead.name = "dt";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush();
    if( *aRead.dtype == DT::FLOAT )
        setDt(Attribute(*aRead.resource).get< float >());
    else if( *aRead.dtype == DT::DOUBLE )
        setDt(Attribute(*aRead.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'dt'");

    aRead.name = "time";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush();
    if( *aRead.dtype == DT::FLOAT )
        setTime(Attribute(*aRead.resource).get< float >());
    else if( *aRead.dtype == DT::DOUBLE )
        setTime(Attribute(*aRead.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'time'");

    aRead.name = "timeUnitSI";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush();
    if( *aRead.dtype == DT::DOUBLE )
        setTimeUnitSI(Attribute(*aRead.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'timeUnitSI'");

    /* Find the root point [Series] of this file,
     * meshesPath and particlesPath are stored there */
    internal::SeriesInternal * s = &retrieveSeries();

    Parameter< Operation::LIST_PATHS > pList;
    std::string version = s->openPMD();
    bool hasMeshes = false;
    bool hasParticles = false;
    if( version == "1.0.0" || version == "1.0.1" )
    {
        IOHandler()->enqueue(IOTask(this, pList));
        IOHandler()->flush();
        hasMeshes = std::count(
            pList.paths->begin(),
            pList.paths->end(),
            auxiliary::replace_last(s->meshesPath(), "/", "")
        ) == 1;
        hasParticles = std::count(
            pList.paths->begin(),
            pList.paths->end(),
            auxiliary::replace_last(s->particlesPath(), "/", "")
        ) == 1;
        pList.paths->clear();
    } else
    {
        hasMeshes = s->containsAttribute("meshesPath");
        hasParticles = s->containsAttribute("particlesPath");
    }

    if( hasMeshes )
    {
        pOpen.path = s->meshesPath();
        IOHandler()->enqueue(IOTask(&meshes, pOpen));

        meshes.readAttributes( ReadMode::FullyReread );

        auto map = meshes.eraseStaleEntries();

        /* obtain all non-scalar meshes */
        IOHandler()->enqueue(IOTask(&meshes, pList));
        IOHandler()->flush();

        Parameter< Operation::LIST_ATTS > aList;
        for( auto const& mesh_name : *pList.paths )
        {
            Mesh& m = map[mesh_name];
            pOpen.path = mesh_name;
            aList.attributes->clear();
            IOHandler()->enqueue(IOTask(&m, pOpen));
            IOHandler()->enqueue(IOTask(&m, aList));
            IOHandler()->flush();

            auto att_begin = aList.attributes->begin();
            auto att_end = aList.attributes->end();
            auto value = std::find(att_begin, att_end, "value");
            auto shape = std::find(att_begin, att_end, "shape");
            if( value != att_end && shape != att_end )
            {
                MeshRecordComponent& mrc = m[MeshRecordComponent::SCALAR];
                mrc.parent() = m.parent();
                IOHandler()->enqueue(IOTask(&mrc, pOpen));
                IOHandler()->flush();
                *mrc.m_isConstant = true;
            }
            m.read();
        }

        /* obtain all scalar meshes */
        Parameter< Operation::LIST_DATASETS > dList;
        IOHandler()->enqueue(IOTask(&meshes, dList));
        IOHandler()->flush();

        Parameter< Operation::OPEN_DATASET > dOpen;
        for( auto const& mesh_name : *dList.datasets )
        {
            Mesh& m = map[mesh_name];
            dOpen.name = mesh_name;
            IOHandler()->enqueue(IOTask(&m, dOpen));
            IOHandler()->flush();
            MeshRecordComponent& mrc = m[MeshRecordComponent::SCALAR];
            mrc.parent() = m.parent();
            IOHandler()->enqueue(IOTask(&mrc, dOpen));
            IOHandler()->flush();
            mrc.written() = false;
            mrc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
            mrc.written() = true;
            m.read();
        }
    }
    else
    {
        meshes.dirty() = false;
    }

    if( hasParticles )
    {
        pOpen.path = s->particlesPath();
        IOHandler()->enqueue(IOTask(&particles, pOpen));

        particles.readAttributes( ReadMode::FullyReread );

        /* obtain all particle species */
        pList.paths->clear();
        IOHandler()->enqueue(IOTask(&particles, pList));
        IOHandler()->flush();

        auto map = particles.eraseStaleEntries();
        for( auto const& species_name : *pList.paths )
        {
            ParticleSpecies& p = map[species_name];
            pOpen.path = species_name;
            IOHandler()->enqueue(IOTask(&p, pOpen));
            IOHandler()->flush();
            p.read();
        }
    }
    else
    {
        particles.dirty() = false;
    }

    readAttributes( ReadMode::FullyReread );
}

AdvanceStatus
Iteration::beginStep()
{
    using IE = IterationEncoding;
    auto & series = retrieveSeries();
    // Initialize file with this to quiet warnings
    // The following switch is comprehensive
    internal::AttributableData * file = nullptr;
    switch( series.iterationEncoding() )
    {
        case IE::fileBased:
            file = m_attributableData.get();
            break;
        case IE::groupBased:
        case IE::variableBased:
            file = &series;
            break;
    }
    AdvanceStatus status = series.advance(
        AdvanceMode::BEGINSTEP, *file, series.indexOf( *this ), *this );
    if( status != AdvanceStatus::OK )
    {
        return status;
    }

    // re-read -> new datasets might be available
    if( ( series.iterationEncoding() == IE::groupBased ||
          series.iterationEncoding() == IE::variableBased ) &&
        ( this->IOHandler()->m_frontendAccess == Access::READ_ONLY ||
          this->IOHandler()->m_frontendAccess == Access::READ_WRITE ) )
    {
        bool previous = series.iterations.written();
        series.iterations.written() = false;
        auto oldType = this->IOHandler()->m_frontendAccess;
        auto newType =
            const_cast< Access * >( &this->IOHandler()->m_frontendAccess );
        *newType = Access::READ_WRITE;
        series.readGorVBased( false );
        *newType = oldType;
        series.iterations.written() = previous;
    }

    return status;
}

void
Iteration::endStep()
{
    using IE = IterationEncoding;
    auto & series = retrieveSeries();
    // Initialize file with this to quiet warnings
    // The following switch is comprehensive
    internal::AttributableData * file = nullptr;
    switch( series.iterationEncoding() )
    {
        case IE::fileBased:
            file = m_attributableData.get();
            break;
        case IE::groupBased:
        case IE::variableBased:
            file = &series;
            break;
    }
    // @todo filebased check
    series.advance(
        AdvanceMode::ENDSTEP, *file, series.indexOf( *this ), *this );
}

StepStatus
Iteration::getStepStatus()
{
    internal::SeriesInternal * s = &retrieveSeries();
    switch( s->iterationEncoding() )
    {
        using IE = IterationEncoding;
        case IE::fileBased:
            return *this->m_stepStatus;
        case IE::groupBased:
        case IE::variableBased:
            return s->m_stepStatus;
        default:
            throw std::runtime_error( "[Iteration] unreachable" );
    }
}

void
Iteration::setStepStatus( StepStatus status )
{
    internal::SeriesInternal * s = &retrieveSeries();
    switch( s->iterationEncoding() )
    {
        using IE = IterationEncoding;
        case IE::fileBased:
            *this->m_stepStatus = status;
            break;
        case IE::groupBased:
        case IE::variableBased:
            s->m_stepStatus = status;
            break;
        default:
            throw std::runtime_error( "[Iteration] unreachable" );
    }
}

bool
Iteration::dirtyRecursive() const
{
    if( dirty() )
    {
        return true;
    }
    if( particles.dirty() || meshes.dirty() )
    {
        return true;
    }
    for( auto const & pair : particles )
    {
        if( pair.second.dirtyRecursive() )
        {
            return true;
        }
    }
    for( auto const & pair : meshes )
    {
        if( pair.second.dirtyRecursive() )
        {
            return true;
        }
    }
    return false;
}

void
Iteration::linkHierarchy(Writable& w)
{
    AttributableImpl::linkHierarchy(w);
    meshes.linkHierarchy(this->writable());
    particles.linkHierarchy(this->writable());
}

void Iteration::runDeferredParseAccess()
{
    if( IOHandler()->m_frontendAccess == Access::CREATE )
    {
        return;
    }
    auto oldAccess = IOHandler()->m_frontendAccess;
    auto newAccess =
        const_cast< Access * >( &IOHandler()->m_frontendAccess );
    *newAccess = Access::READ_WRITE;
    try
    {
        read();
    }
    catch( ... )
    {
        *newAccess = oldAccess;
        throw;
    }
    *newAccess = oldAccess;
}

template float
Iteration::time< float >() const;
template double
Iteration::time< double >() const;
template long double
Iteration::time< long double >() const;

template float
Iteration::dt< float >() const;
template double
Iteration::dt< double >() const;
template long double
Iteration::dt< long double >() const;

template
Iteration& Iteration::setTime< float >(float time);
template
Iteration& Iteration::setTime< double >(double time);
template
Iteration& Iteration::setTime< long double >(long double time);

template
Iteration& Iteration::setDt< float >(float dt);
template
Iteration& Iteration::setDt< double >(double dt);
template
Iteration& Iteration::setDt< long double >(long double dt);
} // openPMD
