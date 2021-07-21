/* Copyright 2017-2021 Fabian Koller, Axel Huebl
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

#include "openPMD/config.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/IO/Format.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/Streaming.hpp"
#include "openPMD/WriteIterations.hpp"
#include "openPMD/auxiliary/Option.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/config.hpp"
#include "openPMD/version.hpp"

#if openPMD_HAVE_MPI
#   include <mpi.h>
#endif

#include <map>
#include <string>

// expose private and protected members for invasive testing
#ifndef OPENPMD_private
#   define OPENPMD_private private
#endif


namespace openPMD
{
class ReadIterations;
class Series;
class SeriesImpl;

namespace internal
{
/**
 * @brief Data members for Series. Pinned at one memory location.
 *
 * (Not movable or copyable)
 *
 */
class SeriesData : public AttributableData
{
public:
    explicit SeriesData() = default;

    SeriesData( SeriesData const & ) = delete;
    SeriesData( SeriesData && ) = delete;

    SeriesData & operator=( SeriesData const & ) = delete;
    SeriesData & operator=( SeriesData && ) = delete;

    virtual ~SeriesData() = default;

    Container< Iteration, uint64_t > iterations{};

    auxiliary::Option< WriteIterations > m_writeIterations;
    auxiliary::Option< std::string > m_overrideFilebasedFilename;
    std::string m_name;
    std::string m_filenamePrefix;
    std::string m_filenamePostfix;
    int m_filenamePadding;
    IterationEncoding m_iterationEncoding{};
    Format m_format;
    /**
     *  Whether a step is currently active for this iteration.
     * Used for group-based iteration layout, see SeriesData.hpp for
     * iteration-based layout.
     * Access via stepStatus() method to automatically select the correct
     * one among both flags.
     */
    StepStatus m_stepStatus = StepStatus::NoStep;
    bool m_parseLazily = false;
    bool m_lastFlushSuccessful = true;
}; // SeriesData

class SeriesInternal;
} // namespace internal

/** @brief  Implementation for the root level of the openPMD hierarchy.
 *
 * Entry point and common link between all iterations of particle and mesh data.
 *
 * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file
 * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series
 */
class SeriesImpl : public AttributableImpl
{
    friend class AttributableImpl;
    friend class Iteration;
    friend class Writable;
    friend class SeriesIterator;
    friend class internal::SeriesInternal;
    friend class Series;
    friend class WriteIterations;

protected:
    // Should not be called publicly, only by implementing classes
    SeriesImpl( internal::SeriesData *, internal::AttributableData * );

public:
    /**
     * @return  String representing the current enforced version of the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     */
    std::string openPMD() const;
    /** Set the version of the enforced <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     *
     * @param   openPMD   String <CODE>MAJOR.MINOR.REVISION</CODE> of the desired version of the openPMD standard.
     * @return  Reference to modified series.
     */
    SeriesImpl& setOpenPMD(std::string const& openPMD);

    /**
     * @return  32-bit mask of applied extensions to the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     */
    uint32_t openPMDextension() const;
    /** Set a 32-bit mask of applied extensions to the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     *
     * @param   openPMDextension  Unsigned 32-bit integer used as a bit-mask of applied extensions.
     * @return  Reference to modified series.
     */
    SeriesImpl& setOpenPMDextension(uint32_t openPMDextension);

    /**
     * @return  String representing the common prefix for all data sets and sub-groups of a specific iteration.
     */
    std::string basePath() const;
    /** Set the common prefix for all data sets and sub-groups of a specific iteration.
     *
     * @param   basePath    String of the common prefix for all data sets and sub-groups of a specific iteration.
     * @return  Reference to modified series.
     */
    SeriesImpl& setBasePath(std::string const& basePath);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String representing the path to mesh records, relative(!) to <CODE>basePath</CODE>.
     */
    std::string meshesPath() const;
    /** Set the path to <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#mesh-based-records">mesh records</A>, relative(!) to <CODE>basePath</CODE>.
     *
     * @param   meshesPath  String of the path to <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#mesh-based-records">mesh records</A>, relative(!) to <CODE>basePath</CODE>.
     * @return  Reference to modified series.
     */
    SeriesImpl& setMeshesPath(std::string const& meshesPath);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String representing the path to particle species, relative(!) to <CODE>basePath</CODE>.
     */
    std::string particlesPath() const;
    /** Set the path to groups for each <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#particle-records">particle species</A>, relative(!) to <CODE>basePath</CODE>.
     *
     * @param   particlesPath   String of the path to groups for each <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#particle-records">particle species</A>, relative(!) to <CODE>basePath</CODE>.
     * @return  Reference to modified series.
     */
    SeriesImpl& setParticlesPath(std::string const& particlesPath);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String indicating author and contact for the information in the file.
     */
    std::string author() const;
    /** Indicate the author and contact for the information in the file.
     *
     * @param   author  String indicating author and contact for the information in the file.
     * @return  Reference to modified series.
     */
    SeriesImpl& setAuthor(std::string const& author);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String indicating the software/code/simulation that created the file;
     */
    std::string software() const;
    /** Indicate the software/code/simulation that created the file.
     *
     * @param   newName    String indicating the software/code/simulation that created the file.
     * @param   newVersion String indicating the version of the software/code/simulation that created the file.
     * @return  Reference to modified series.
     */
    SeriesImpl& setSoftware(std::string const& newName, std::string const& newVersion = std::string("unspecified"));

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String indicating the version of the software/code/simulation that created the file.
     */
    std::string softwareVersion() const;
    /** Indicate the version of the software/code/simulation that created the file.
     *
     * @deprecated Set the version with the second argument of setSoftware()
     *
     * @param   softwareVersion String indicating the version of the software/code/simulation that created the file.
     * @return  Reference to modified series.
     */
    [[deprecated("Set the version with the second argument of setSoftware()")]]
    SeriesImpl& setSoftwareVersion(std::string const& softwareVersion);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String indicating date of creation.
     */
    std::string date() const;
    /** Indicate the date of creation.
     *
     * @param   date    String indicating the date of creation.
     * @return  Reference to modified series.
     */
    SeriesImpl& setDate(std::string const& date);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String indicating dependencies of software that were used to create the file.
     */
    std::string softwareDependencies() const;
    /** Indicate dependencies of software that were used to create the file.
     *
     * @param   newSoftwareDependencies String indicating dependencies of software that were used to create the file (semicolon-separated list if needed).
     * @return  Reference to modified series.
     */
    SeriesImpl& setSoftwareDependencies(std::string const& newSoftwareDependencies);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String indicating the machine or relevant hardware that created the file.
     */
    std::string machine() const;
    /** Indicate the machine or relevant hardware that created the file.
     *
     * @param   newMachine String indicating the machine or relevant hardware that created the file (semicolon-separated list if needed)..
     * @return  Reference to modified series.
     */
    SeriesImpl& setMachine(std::string const& newMachine);

    /**
     * @return  Current encoding style for multiple iterations in this series.
     */
    IterationEncoding iterationEncoding() const;
    /** Set the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">encoding style</A> for multiple iterations in this series.
     * A preview on the <A HREF="https://github.com/openPMD/openPMD-standard/pull/250">openPMD 2.0 variable-based iteration encoding</A> can be activated with this call.
     * Making full use of the variable-based iteration encoding requires (1) explicit support by the backend (available only in ADIOS2) and (2) use of the openPMD streaming API.
     * In other backends and without the streaming API, only one iteration/snapshot may be written in the variable-based encoding, making this encoding a good choice for single-snapshot data dumps.
     *
     * @param   iterationEncoding   Desired <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">encoding style</A> for multiple iterations in this series.
     * @return  Reference to modified series.
     */
    SeriesImpl& setIterationEncoding(IterationEncoding iterationEncoding);

    /**
     * @return  String describing a <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">pattern</A> describing how to access single iterations in the raw file.
     */
    std::string iterationFormat() const;
    /** Set a <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">pattern</A> describing how to access single iterations in the raw file.
     *
     * @param   iterationFormat String with the iteration regex <CODE>\%T</CODE> defining either
     *                          the series of files (fileBased)
     *                          or the series of groups within a single file (groupBased)
     *                          that allows to extract the iteration from it.
     *                          For fileBased formats the iteration must be included in the file name.
     *                          The format depends on the selected iterationEncoding method.
     * @return  Reference to modified series.
     */
    SeriesImpl& setIterationFormat(std::string const& iterationFormat);

    /**
     * @return String of a pattern for file names.
     */
    std::string name() const;

    /** Set the pattern for file names.
     *
     * @param   name    String of the pattern for file names. Must include iteration regex <CODE>\%T</CODE> for fileBased data.
     * @return  Reference to modified series.
     */
    SeriesImpl& setName(std::string const& name);

    /** The currently used backend
     *
     * @see AbstractIOHandler::backendName()
     *
     * @return String of a pattern for data backend.
     */
    std::string backend() const;

    /** Execute all required remaining IO operations to write or read data.
     */
    void flush();

OPENPMD_private:
    static constexpr char const * const BASEPATH = "/data/%T/";

    struct ParsedInput;
    using iterations_t = decltype(internal::SeriesData::iterations);
    using iterations_iterator = iterations_t::iterator;

    internal::SeriesData * m_series = nullptr;

    inline internal::SeriesData & get()
    {
        if( m_series )
        {
            return *m_series;
        }
        else
        {
            throw std::runtime_error(
                "[Series] Cannot use default-constructed Series." );
        }
    }

    inline internal::SeriesData const & get() const
    {
        if( m_series )
        {
            return *m_series;
        }
        else
        {
            throw std::runtime_error(
                "[Series] Cannot use default-constructed Series." );
        }    }

    std::unique_ptr< ParsedInput > parseInput(std::string);
    void init(std::shared_ptr< AbstractIOHandler >, std::unique_ptr< ParsedInput >);
    void initDefaults( IterationEncoding );
    /**
     * @brief Internal call for flushing a Series.
     *
     * Any flushing of the Series will pass through this call.
     * 
     * @param begin Start of the range of iterations to flush.
     * @param end End of the range of iterations to flush.
     * @param level Flush level, as documented in AbstractIOHandler.hpp.
     * @param flushIOHandler Tasks will always be enqueued to the backend.
     *     If this flag is true, tasks will be flushed to the backend.
     */
    std::future< void > flush_impl(
        iterations_iterator begin,
        iterations_iterator end,
        FlushLevel level,
        bool flushIOHandler = true );
    void flushFileBased( iterations_iterator begin, iterations_iterator end );
    /*
     * Group-based and variable-based iteration layouts share a lot of logic
     * (realistically, the variable-based iteration layout only throws out
     *  one layer in the hierarchy).
     * As a convention, methods that deal with both layouts are called
     * .*GorVBased, short for .*GroupOrVariableBased
     */
    void flushGorVBased( iterations_iterator begin, iterations_iterator end );
    void flushMeshesPath();
    void flushParticlesPath();
    void readFileBased( );
    void readOneIterationFileBased( std::string const & filePath );
    /**
     * Note on re-parsing of a Series:
     * If init == false, the parsing process will seek for new
     * Iterations/Records/Record Components etc.
     */
    void readGorVBased( bool init = true );
    void readBase();
    std::string iterationFilename( uint64_t i );

    enum class IterationOpened : bool
    {
        HasBeenOpened,
        RemainsClosed
    };
    /*
     * For use by flushFileBased, flushGorVBased
     * Open an iteration, but only if necessary.
     * Only open if the iteration is dirty and if it is not in deferred
     * parse state.
     */
    IterationOpened openIterationIfDirty( uint64_t index, Iteration iteration );
    /*
     * Open an iteration. Ensures that the iteration's m_closed status
     * is set properly and that any files pertaining to the iteration
     * is opened.
     * Does not create files when called in CREATE mode.
     */
    void openIteration( uint64_t index, Iteration iteration );

    /**
     * Find the given iteration in Series::iterations and return an iterator
     * into Series::iterations at that place.
     */
    iterations_iterator
    indexOf( Iteration const & );

    /**
     * @brief In step-based IO mode, begin or end an IO step for the given
     *        iteration.
     *
     * Called internally by Iteration::beginStep and Iteration::endStep.
     *
     * @param mode Whether to begin or end a step.
     * @param file The Attributable representing the iteration. In file-based
     *             iteration layout, this is an Iteration object, in group-
     *             based layout, it's the Series object.
     * @param it The iterator within Series::iterations pointing to that
     *           iteration.
     * @param iteration The actual Iteration object.
     * @return AdvanceStatus
     */
    AdvanceStatus
    advance(
        AdvanceMode mode,
        internal::AttributableData & file,
        iterations_iterator it,
        Iteration & iteration );
}; // SeriesImpl

namespace internal
{
class SeriesInternal : public SeriesData, public SeriesImpl
{
    friend struct SeriesShared;
    friend class openPMD::Iteration;
    friend class openPMD::Series;
    friend class openPMD::Writable;

public:
#if openPMD_HAVE_MPI
    SeriesInternal(
        std::string const & filepath,
        Access at,
        MPI_Comm comm,
        std::string const & options = "{}" );
#endif

    SeriesInternal(
        std::string const & filepath,
        Access at,
        std::string const & options = "{}" );
    // @todo make AttributableImpl<>::linkHierarchy non-virtual
    virtual ~SeriesInternal();
};
} // namespace internal

/** @brief  Root level of the openPMD hierarchy.
 *
 * Entry point and common link between all iterations of particle and mesh data.
 *
 * An instance can be created either directly via the given constructors or via
 * the SeriesBuilder class.
 *
 * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file
 * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series
 */
class Series : public SeriesImpl
{
private:
    std::shared_ptr< internal::SeriesInternal > m_series;

public:
    explicit Series();

#if openPMD_HAVE_MPI
    Series(
        std::string const & filepath,
        Access at,
        MPI_Comm comm,
        std::string const & options = "{}" );
#endif

    /**
     * @brief Construct a new Series
     *
     * @param filepath The backend will be determined by the filepath extension.
     * @param at Access mode.
     * @param options Advanced backend configuration via JSON.
     *      May be specified as a JSON-formatted string directly, or as a path
     *      to a JSON textfile, prepended by an at sign '@'.
     */
    Series(
        std::string const & filepath,
        Access at,
        std::string const & options = "{}" );

    virtual ~Series() = default;

    Container< Iteration, uint64_t > iterations;

    /**
     * @brief Is this a usable Series object?
     *
     * @return true If a Series has been opened for reading and/or writing.
     * @return false If the object has been default-constructed.
     */
    operator bool() const;

    /**
     * @brief Entry point to the reading end of the streaming API.
     *
     * Creates and returns an instance of the ReadIterations class which can
     * be used for iterating over the openPMD iterations in a C++11-style for
     * loop.
     * Look for the ReadIterations class for further documentation.
     *
     * @return ReadIterations
     */
    ReadIterations readIterations();

    /**
     * @brief Entry point to the writing end of the streaming API.
     *
     * Creates and returns an instance of the WriteIterations class which is a
     * restricted container of iterations which takes care of
     * streaming semantics.
     * The created object is stored as member of the Series object, hence this
     * method may be called as many times as a user wishes.
     * Look for the WriteIterations class for further documentation.
     *
     * @return WriteIterations
     */
    WriteIterations writeIterations();
};
} // namespace openPMD

// Make sure that this one is always included if Series.hpp is included,
// otherwise SeriesImpl::readIterations() cannot be used
#include "openPMD/ReadIterations.hpp"
