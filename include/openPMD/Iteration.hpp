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

#include "openPMD/auxiliary/Option.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Streaming.hpp"


namespace openPMD
{
/** @brief  Logical compilation of data from one snapshot (e.g. a single simulation cycle).
 *
 * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#required-attributes-for-the-basepath
 */
class Iteration : public LegacyAttributable
{
    template<
            typename T,
            typename T_key,
            typename T_container
    >
    friend class Container;
    friend class SeriesImpl;
    friend class WriteIterations;
    friend class SeriesIterator;

public:
    Iteration( Iteration const & ) = default;
    Iteration & operator=( Iteration const & ) = default;

    /**
     * @tparam  T   Floating point type of user-selected precision (e.g. float, double).
     * @return  Global reference time for this iteration.
     */
    template< typename T >
    T time() const;
    /** Set the global reference time for this iteration.
     *
     * @tparam  T       Floating point type of user-selected precision (e.g. float, double).
     * @param   newTime Global reference time for this iteration.
     * @return  Reference to modified iteration.
     */
    template< typename T >
    Iteration& setTime(T newTime);

    /**
     * @tparam  T   Floating point type of user-selected precision (e.g. float, double).
     * @return  Time step used to reach this iteration.
     */
    template< typename T >
    T dt() const;
    /** Set the time step used to reach this iteration.
     *
     * @tparam  T     Floating point type of user-selected precision (e.g. float, double).
     * @param   newDt Time step used to reach this iteration.
     * @return  Reference to modified iteration.
     */
    template< typename T >
    Iteration& setDt(T newDt);

    /**
     * @return Conversion factor to convert time and dt to seconds.
     */
    double timeUnitSI() const;
    /** Set the conversion factor to convert time and dt to seconds.
     *
     * @param  newTimeUnitSI new value for timeUnitSI
     * @return Reference to modified iteration.
     */
    Iteration& setTimeUnitSI(double newTimeUnitSI);

    /** Close an iteration
     *
     * No further (backend-propagating) accesses may be performed on this
     * iteration. A closed iteration may not (yet) be reopened.
     *
     * With an MPI-parallel series, close is an MPI-collective operation.
     *
     * @return Reference to iteration.
     */
    /*
     * Note: If the API is changed in future to allow reopening closed
     * iterations, measures should be taken to prevent this in the streaming
     * API. Currently, disallowing to reopen closed iterations satisfies
     * the requirements of the streaming API.
     */
    Iteration &
    close( bool flush = true );

    /** Open an iteration
     *
     * Explicitly open an iteration.
     * Usually, file-open operations are delayed until the first load/storeChunk
     * operation is flush-ed. In parallel contexts where it is know that such a
     * first access needs to be run non-collectively, one can explicitly open
     * an iteration through this collective call.
     * Also necessary when using defer_iteration_parsing.
     * The Streaming API (i.e. Series::readIterations()) will call this method
     * implicitly and users need not call it.
     *
     * @return Reference to iteration.
     */
    Iteration &
    open();

    /**
     * @brief Has the iteration been closed?
     *        A closed iteration may not (yet) be reopened.
     *
     * @return Whether the iteration has been closed.
     */
    bool
    closed() const;

    /**
     * @brief Has the iteration been closed by the writer?
     *        Background: Upon calling Iteration::close(), the openPMD API
     *        will add metadata to the iteration in form of an attribute,
     *        indicating that the iteration has indeed been closed.
     *        Useful mainly in streaming context when a reader inquires from
     *        a writer that it is done writing.
     *
     * @return Whether the iteration has been explicitly closed (yet) by the
     *         writer.
     */
    bool
    closedByWriter() const;

    Container< Mesh > meshes;
    Container< ParticleSpecies > particles; //particleSpecies?

    virtual ~Iteration() = default;
private:
    Iteration();

    struct DeferredParseAccess
    {
        /**
         * The group path within /data containing this iteration.
         * Example: "1" for iteration 1, "" in variable-based iteration
         * encoding.
         */
        std::string path;
        /**
         * The iteration index as accessed by the user in series.iterations[i]
         */
        uint64_t iteration = 0;
        /**
         * If this iteration is part of a Series with file-based layout.
         * (Group- and variable-based parsing shares the same code logic.)
         */
        bool fileBased = false;
        /**
         * If fileBased == true, the file name (without file path) of the file
         * containing this iteration.
         */
        std::string filename;
    };

    void flushFileBased(std::string const&, uint64_t);
    void flushGroupBased(uint64_t);
    void flushVariableBased(uint64_t);
    void flush();
    void deferParseAccess( DeferredParseAccess );
    /*
     * Control flow for read(), readFileBased(), readGroupBased() and
     * read_impl():
     * read() is called as the entry point. File-based and group-based
     * iteration layouts need to be parsed slightly differently:
     * In file-based iteration layout, each iteration's file also contains
     * attributes for the /data group. In group-based layout, those have
     * already been parsed during opening of the Series.
     * Hence, read() will call either readFileBased() or readGroupBased() to
     * allow for those different control flows.
     * Finally, read_impl() is called which contains the common parsing
     * logic for an iteration.
     * 
     * reread() reads again an Iteration that has been previously read.
     * Calling it on an Iteration not yet parsed is an error.
     *
     */
    void read();
    void reread( std::string const & path );
    void readFileBased( std::string filePath, std::string const & groupPath );
    void readGorVBased( std::string const & groupPath );
    void read_impl( std::string const & groupPath );

    /**
     * @brief Whether an iteration has been closed yet.
     *
     */
    enum class CloseStatus
    {
        ParseAccessDeferred, //!< The reader has not yet parsed this iteration
        Open,             //!< Iteration has not been closed
        ClosedInFrontend, /*!< Iteration has been closed, but task has not yet
                               been propagated to the backend */
        ClosedInBackend, /*!< Iteration has been closed and task has been
                              propagated to the backend */
        ClosedTemporarily /*!< Iteration has been closed internally and may
                               be reopened later */
    };

    /*
     * An iteration may be logically closed in the frontend,
     * but not necessarily yet in the backend.
     * Will be propagated to the backend upon next flush.
     * Store the current status.
     * Once an iteration has been closed, no further flushes shall be performed.
     * If flushing a closed file, the old file may otherwise be overwritten.
     */
    std::shared_ptr< CloseStatus > m_closed =
        std::make_shared< CloseStatus >( CloseStatus::Open );

    /**
     * Whether a step is currently active for this iteration.
     * Used for file-based iteration layout, see Series.hpp for
     * group-based layout.
     * Access via stepStatus() method to automatically select the correct
     * one among both flags.
     */
    std::shared_ptr< StepStatus > m_stepStatus =
        std::make_shared< StepStatus >( StepStatus::NoStep );

    std::shared_ptr< auxiliary::Option< DeferredParseAccess > >
        m_deferredParseAccess =
            std::make_shared< auxiliary::Option< DeferredParseAccess > >(
                auxiliary::Option< DeferredParseAccess >() );

    /**
     * @brief Begin an IO step on the IO file (or file-like object)
     *        containing this iteration. In case of group-based iteration
     *        layout, this will be the complete Series.
     *
     * @return AdvanceStatus
     */
    AdvanceStatus
    beginStep();

    /**
     * @brief End an IO step on the IO file (or file-like object)
     *        containing this iteration. In case of group-based iteration
     *        layout, this will be the complete Series.
     *
     * @return AdvanceStatus
     */
    void
    endStep();

    /**
     * @brief Is a step currently active for this iteration?
     *
     * In case of group-based iteration layout, this information is global
     * (member of the Series object containing this iteration),
     * in case of file-based iteration layout, it is local (member of this very
     * object).
     */
    StepStatus
    getStepStatus();

    /**
     * @brief Set step activity status for this iteration.
     *
     * In case of group-based iteration layout, this information is set
     * globally (member of the Series object containing this iteration),
     * in case of file-based iteration layout, it is set locally (member of
     * this very object).
     */
    void setStepStatus( StepStatus );

    /*
     * @brief Check recursively whether this Iteration is dirty.
     *        It is dirty if any attribute or dataset is read from or written to
     *        the backend.
     *
     * @return true If dirty.
     * @return false Otherwise.
     */
    bool
    dirtyRecursive() const;

    /**
     * @brief Link with parent.
     * 
     * @param w The Writable representing the parent.
     */
    virtual void linkHierarchy(Writable& w);

    /**
     * @brief Access an iteration in read mode that has potentially not been
     *      parsed yet.
     * 
     */
    void runDeferredParseAccess();
};  // Iteration

extern template
float
Iteration::time< float >() const;

extern template
double
Iteration::time< double >() const;

extern template
long double
Iteration::time< long double >() const;

template< typename T >
inline T
Iteration::time() const
{ return this->readFloatingpoint< T >("time"); }


extern template
float
Iteration::dt< float >() const;

extern template
double
Iteration::dt< double >() const;

extern template
long double
Iteration::dt< long double >() const;

template< typename T >
inline T
Iteration::dt() const
{ return this->readFloatingpoint< T >("dt"); }
} // openPMD
