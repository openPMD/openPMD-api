/* Copyright 2017-2020 Fabian Koller, Axel Huebl
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
class WriteIterations;

/** @brief  Root level of the openPMD hierarchy.
 *
 * Entry point and common link between all iterations of particle and mesh data.
 *
 * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file
 * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series
 */
class Series : public Attributable
{
    friend class Iteration;
    friend class SeriesIterator;

public:
#if openPMD_HAVE_MPI
    Series(
        std::string const & filepath,
        Access at,
        MPI_Comm comm,
        std::string const & options = "{}" );
#endif

    Series(
        std::string const & filepath,
        Access at,
        std::string const & options = "{}" );
    ~Series();

    /**
     * @return  String representing the current enforced version of the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     */
    std::string openPMD() const;
    /** Set the version of the enforced <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     *
     * @param   openPMD   String <CODE>MAJOR.MINOR.REVISION</CODE> of the desired version of the openPMD standard.
     * @return  Reference to modified series.
     */
    Series& setOpenPMD(std::string const& openPMD);

    /**
     * @return  32-bit mask of applied extensions to the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     */
    uint32_t openPMDextension() const;
    /** Set a 32-bit mask of applied extensions to the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     *
     * @param   openPMDextension  Unsigned 32-bit integer used as a bit-mask of applied extensions.
     * @return  Reference to modified series.
     */
    Series& setOpenPMDextension(uint32_t openPMDextension);

    /**
     * @return  String representing the common prefix for all data sets and sub-groups of a specific iteration.
     */
    std::string basePath() const;
    /** Set the common prefix for all data sets and sub-groups of a specific iteration.
     *
     * @param   basePath    String of the common prefix for all data sets and sub-groups of a specific iteration.
     * @return  Reference to modified series.
     */
    Series& setBasePath(std::string const& basePath);

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
    Series& setMeshesPath(std::string const& meshesPath);

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
    Series& setParticlesPath(std::string const& particlesPath);

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
    Series& setAuthor(std::string const& author);

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
    Series& setSoftware(std::string const& newName, std::string const& newVersion = std::string("unspecified"));

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
    Series& setSoftwareVersion(std::string const& softwareVersion);

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
    Series& setDate(std::string const& date);

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
    Series& setSoftwareDependencies(std::string const& newSoftwareDependencies);

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
    Series& setMachine(std::string const& newMachine);

    /**
     * @return  Current encoding style for multiple iterations in this series.
     */
    IterationEncoding iterationEncoding() const;
    /** Set the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">encoding style</A> for multiple iterations in this series.
     *
     * @param   iterationEncoding   Desired <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">encoding style</A> for multiple iterations in this series.
     * @return  Reference to modified series.
     */
    Series& setIterationEncoding(IterationEncoding iterationEncoding);

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
    Series& setIterationFormat(std::string const& iterationFormat);

    /**
     * @return String of a pattern for file names.
     */
    std::string name() const;

    /** Set the pattern for file names.
     *
     * @param   name    String of the pattern for file names. Must include iteration regex <CODE>\%T</CODE> for fileBased data.
     * @return  Reference to modified series.
     */
    Series& setName(std::string const& name);

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
    ReadIterations
    readIterations();

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
    WriteIterations
    writeIterations();

    Container< Iteration, uint64_t > iterations;

OPENPMD_private:
    struct ParsedInput;
    using iterations_iterator = decltype( iterations )::iterator;

    std::unique_ptr< ParsedInput > parseInput(std::string);
    void init(std::shared_ptr< AbstractIOHandler >, std::unique_ptr< ParsedInput >);
    void initDefaults();
    std::future< void > flush_impl(
        iterations_iterator begin, iterations_iterator end );
    void flushFileBased( iterations_iterator begin, iterations_iterator end );
    void flushGroupBased( iterations_iterator begin, iterations_iterator end );
    void flushMeshesPath();
    void flushParticlesPath();
    void readFileBased( );
    /**
     * Note on re-parsing of a Series:
     * If init == false, the parsing process will seek for new
     * Iterations/Records/Record Components etc.
     * Re-parsing of objects that have already been parsed is not implemented
     * as of yet. Such a facility will be required upon implementing things such
     * as resizable datasets.
     */
    void readGroupBased( bool init = true );
    void
    readBase();
    void read();
    std::string iterationFilename( uint64_t i );
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
        Attributable & file,
        iterations_iterator it,
        Iteration & iteration );

    /**
     *  Whether a step is currently active for this iteration.
     * Used for group-based iteration layout, see Series.hpp for
     * iteration-based layout.
     * Access via stepStatus() method to automatically select the correct
     * one among both flags.
     */
    std::shared_ptr< StepStatus > m_stepStatus =
        std::make_shared< StepStatus >( StepStatus::NoStep );

    static constexpr char const * const BASEPATH = "/data/%T/";

    std::shared_ptr< IterationEncoding > m_iterationEncoding;
    std::shared_ptr< std::string > m_name;
    std::shared_ptr< Format > m_format;

    std::shared_ptr< std::string > m_filenamePrefix;
    std::shared_ptr< std::string > m_filenamePostfix;
    std::shared_ptr< int > m_filenamePadding;

    std::shared_ptr< auxiliary::Option< WriteIterations > > m_writeIterations =
        std::make_shared< auxiliary::Option< WriteIterations > >();
}; // Series

/**
 * @brief Subclass of Iteration that knows its own index withing the containing
 *        Series.
 */
class IndexedIteration : public Iteration
{
    friend class SeriesIterator;

public:
    using iterations_t = decltype( Series::iterations );
    using index_t = iterations_t::key_type;
    index_t const iterationIndex;

private:
    template< typename Iteration_t >
    IndexedIteration( Iteration_t && it, index_t index )
        : Iteration( std::forward< Iteration_t >( it ) )
        , iterationIndex( index )
    {
    }
};

class SeriesIterator
{
    using iteration_index_t = IndexedIteration::index_t;

    using maybe_series_t = auxiliary::Option< Series * >;

    maybe_series_t m_series;
    iteration_index_t m_currentIteration = 0;

    //! construct the end() iterator
    SeriesIterator();

public:
    SeriesIterator( Series * );

    SeriesIterator &
    operator++();

    IndexedIteration
    operator*();

    bool
    operator==( SeriesIterator const & other ) const;

    bool
    operator!=( SeriesIterator const & other ) const;

    static SeriesIterator
    end();
};

/**
 * @brief Reading side of the streaming API.
 *
 * Create instance via Series::readIterations().
 * For use in a C++11-style foreach loop over iterations.
 * Designed to allow reading any kind of Series, streaming and non-
 * streaming alike.
 * Calling Iteration::close() manually before opening the next iteration is
 * encouraged and will implicitly flush all deferred IO actions.
 * Otherwise, Iteration::close() will be implicitly called upon
 * SeriesIterator::operator++(), i.e. upon going to the next iteration in
 * the foreach loop.
 * Since this is designed for streaming mode, reopening an iteration is
 * not possible once it has been closed.
 *
 */
class ReadIterations
{
    friend class Series;

private:
    using iterations_t = decltype( Series::iterations );
    using iterator_t = SeriesIterator;

    Series * m_series;

    ReadIterations( Series * );

public:
    iterator_t
    begin();

    iterator_t
    end();
};

/** Writing side of the streaming API.
 *
 * Create instance via Series::writeIterations().
 * For use via WriteIterations::operator[]().
 * Designed to allow reading any kind of Series, streaming and non-
 * streaming alike. Calling Iteration::close() manually before opening
 * the next iteration is encouraged and will implicitly flush all
 * deferred IO actions. Otherwise, Iteration::close() will be implicitly
 * called upon SeriesIterator::operator++(), i.e. upon going to the next
 * iteration in the foreach loop.
 *
 * Since this is designed for streaming mode, reopening an iteration is
 * not possible once it has been closed.
 *
 */
class WriteIterations : private Container< Iteration, uint64_t >
{
    friend class Series;

private:
    using iterations_t = Container< Iteration, uint64_t >;
    struct SharedResources
    {
        iterations_t iterations;
        auxiliary::Option< uint64_t > currentlyOpen;

        SharedResources( iterations_t );
        ~SharedResources();
    };

    using key_type = typename iterations_t::key_type;
    using value_type = typename iterations_t::key_type;
    WriteIterations( iterations_t );
    explicit WriteIterations() = default;
    //! Index of the last opened iteration
    std::shared_ptr< SharedResources > shared;

public:
    mapped_type &
    operator[]( key_type const & key ) override;
    mapped_type &
    operator[]( key_type && key ) override;
};
} // namespace openPMD
