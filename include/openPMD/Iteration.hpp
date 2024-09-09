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

#include "openPMD/CustomHierarchy.hpp"
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Streaming.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <set>
#include <tuple>

namespace openPMD
{
namespace internal
{
    /**
     * @brief Whether an iteration has been closed yet.
     *
     */
    enum class CloseStatus
    {
        ParseAccessDeferred, //!< The reader has not yet parsed this iteration
        Open, //!< Iteration has not been closed
        ClosedInFrontend, /*!< Iteration has been closed, but task has not yet
                               been propagated to the backend */
        ClosedInBackend, /*!< Iteration has been closed and task has been
                              propagated to the backend */
        ClosedTemporarily /*!< Iteration has been closed internally and may
                               be reopened later */
    };

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
        bool beginStep = false;
    };

    class IterationData : public CustomHierarchyData
    {
    public:
        /*
         * An iteration may be logically closed in the frontend,
         * but not necessarily yet in the backend.
         * Will be propagated to the backend upon next flush.
         * Store the current status.
         * Once an iteration has been closed, no further flushes shall be
         * performed. If flushing a closed file, the old file may otherwise be
         * overwritten.
         */
        CloseStatus m_closed = CloseStatus::Open;

        /**
         * Whether a step is currently active for this iteration.
         * Used for file-based iteration layout, see Series.hpp for
         * group-based layout.
         * Access via stepStatus() method to automatically select the correct
         * one among both flags.
         */
        StepStatus m_stepStatus = StepStatus::NoStep;

        /**
         * Information on a parsing request that has not yet been executed.
         * Otherwise empty.
         */
        std::optional<DeferredParseAccess> m_deferredParseAccess{};

        /**
         * Upon reading a file, set this field to the used file name.
         * In inconsistent iteration paddings, we must remember the name of the
         * file since it cannot be reconstructed from the filename pattern
         * alone.
         */
        std::optional<std::string> m_overrideFilebasedFilename{};
    };
} // namespace internal
/** @brief  Logical compilation of data from one snapshot (e.g. a single
 * simulation cycle).
 *
 * @see
 * https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#required-attributes-for-the-basepath
 */
class Iteration : public CustomHierarchy
{
public:
    using IterationIndex_t = uint64_t;

    /*
     * Some old compilers have trouble with befriending the entire Container
     * template here, so we restrict it
     * to Container<Iteration, IterationIndex_t>, more is not needed anyway.
     *
     * E.g. on gcc-7:
     * > error: specialization of 'openPMD::Container<openPMD::CustomHierarchy>'
     * > after instantiation
     * >      friend class Container;
     */
    friend class Container<Iteration, IterationIndex_t>;
    friend class Series;
    friend class WriteIterations;
    friend class SeriesIterator;
    friend class internal::AttributableData;
    template <typename T>
    friend T &internal::makeOwning(T &self, Series);

    Iteration(Iteration const &) = default;
    Iteration(Iteration &&) = default;
    Iteration &operator=(Iteration const &) = default;
    Iteration &operator=(Iteration &&) = default;

    // These use the openPMD Container class mainly for consistency.
    // But they are in fact only aliases that don't actually exist
    // in the backend.
    // Hence meshes.written() and particles.written() will always be false.
    Container<Mesh> meshes{};
    Container<ParticleSpecies> particles{};

    /**
     * @tparam  T   Floating point type of user-selected precision (e.g. float,
     * double).
     * @return  Global reference time for this iteration.
     */
    template <typename T>
    T time() const;
    /** Set the global reference time for this iteration.
     *
     * @tparam  T       Floating point type of user-selected precision (e.g.
     * float, double).
     * @param   newTime Global reference time for this iteration.
     * @return  Reference to modified iteration.
     */
    template <typename T>
    Iteration &setTime(T newTime);

    /**
     * @tparam  T   Floating point type of user-selected precision (e.g. float,
     * double).
     * @return  Time step used to reach this iteration.
     */
    template <typename T>
    T dt() const;
    /** Set the time step used to reach this iteration.
     *
     * @tparam  T     Floating point type of user-selected precision (e.g.
     * float, double).
     * @param   newDt Time step used to reach this iteration.
     * @return  Reference to modified iteration.
     */
    template <typename T>
    Iteration &setDt(T newDt);

    /**
     * @return Conversion factor to convert time and dt to seconds.
     */
    double timeUnitSI() const;
    /** Set the conversion factor to convert time and dt to seconds.
     *
     * @param  newTimeUnitSI new value for timeUnitSI
     * @return Reference to modified iteration.
     */
    Iteration &setTimeUnitSI(double newTimeUnitSI);

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
    Iteration &close(bool flush = true);

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
    Iteration &open();

    /**
     * @brief Has the iteration been closed?
     *        A closed iteration may not (yet) be reopened.
     *
     * @return Whether the iteration has been closed.
     */
    bool closed() const;

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
    [[deprecated("This attribute is no longer set by the openPMD-api.")]] bool
    closedByWriter() const;

    virtual ~Iteration() = default;

private:
    Iteration();

    using Data_t = internal::IterationData;
    std::shared_ptr<Data_t> m_iterationData;

    inline Data_t const &get() const
    {
        return *m_iterationData;
    }

    inline Data_t &get()
    {
        return *m_iterationData;
    }

    inline std::shared_ptr<Data_t> getShared()
    {
        return m_iterationData;
    }

    inline void setData(std::shared_ptr<Data_t> data)
    {
        m_iterationData = std::move(data);
        CustomHierarchy::setData(m_iterationData);
    }

    void flushFileBased(
        std::string const &, IterationIndex_t, internal::FlushParams const &);
    void flushGroupBased(IterationIndex_t, internal::FlushParams const &);
    void flushVariableBased(IterationIndex_t, internal::FlushParams const &);
    /*
     * Named flushIteration instead of flush to avoid naming
     * conflicts with overridden virtual flush from CustomHierarchy
     * class.
     */
    void flushIteration(internal::FlushParams const &);

    void sync_meshes_and_particles_from_alias_to_subgroups(
        internal::MeshesParticlesPath const &);
    void sync_meshes_and_particles_from_subgroups_to_alias(
        internal::MeshesParticlesPath const &);

    void deferParseAccess(internal::DeferredParseAccess);
    /*
     * Control flow for runDeferredParseAccess(), readFileBased(),
     * readGroupBased() and read_impl():
     * runDeferredParseAccess() is called as the entry point.
     * File-based and group-based
     * iteration layouts need to be parsed slightly differently:
     * In file-based iteration layout, each iteration's file also contains
     * attributes for the /data group. In group-based layout, those have
     * already been parsed during opening of the Series.
     * Hence, runDeferredParseAccess() will call either readFileBased() or
     * readGroupBased() to
     * allow for those different control flows.
     * Finally, read_impl() is called which contains the common parsing
     * logic for an iteration.
     *
     * reread() reads again an Iteration that has been previously read.
     * Calling it on an Iteration not yet parsed is an error.
     *
     */
    void reread(std::string const &path);
    void readFileBased(
        std::string const &filePath,
        std::string const &groupPath,
        bool beginStep);
    void readGorVBased(std::string const &groupPath, bool beginStep);
    void read_impl(std::string const &groupPath);

    /**
     * Status after beginning an IO step. Currently includes:
     * * The advance status (OK, OVER, RANDOMACCESS)
     * * The opened iterations, in case the snapshot attribute is found
     */
    struct BeginStepStatus
    {
        using AvailableIterations_t = std::optional<std::deque<uint64_t> >;

        AdvanceStatus stepStatus{};
        /*
         * If the iteration attribute `snapshot` is present, the value of that
         * attribute. Otherwise empty.
         */
        AvailableIterations_t iterationsInOpenedStep;

        /*
         * Most of the time, the AdvanceStatus part of this struct is what we
         * need, so let's make it easy to access.
         */
        inline operator AdvanceStatus() const
        {
            return stepStatus;
        }

        /*
         * Support for std::tie()
         */
        inline operator std::tuple<AdvanceStatus &, AvailableIterations_t &>()
        {
            return std::tuple<AdvanceStatus &, AvailableIterations_t &>{
                stepStatus, iterationsInOpenedStep};
        }
    };

    /**
     * @brief Begin an IO step on the IO file (or file-like object)
     *        containing this iteration. In case of group-based iteration
     *        layout, this will be the complete Series.
     *
     * @return BeginStepStatus
     */
    BeginStepStatus beginStep(bool reread);

    /*
     * Iteration-independent variant for beginStep().
     * Useful in group-based iteration encoding where the Iteration will only
     * be known after opening the step.
     */
    static BeginStepStatus beginStep(
        std::optional<Iteration> thisObject,
        Series &series,
        bool reread,
        std::set<IterationIndex_t> const &ignoreIterations = {});

    /**
     * @brief End an IO step on the IO file (or file-like object)
     *        containing this iteration. In case of group-based iteration
     *        layout, this will be the complete Series.
     */
    void endStep();

    /**
     * @brief Is a step currently active for this iteration?
     *
     * In case of group-based iteration layout, this information is global
     * (member of the Series object containing this iteration),
     * in case of file-based iteration layout, it is local (member of this very
     * object).
     */
    StepStatus getStepStatus();

    /**
     * @brief Set step activity status for this iteration.
     *
     * In case of group-based iteration layout, this information is set
     * globally (member of the Series object containing this iteration),
     * in case of file-based iteration layout, it is set locally (member of
     * this very object).
     */
    void setStepStatus(StepStatus);

    /*
     * @brief Check recursively whether this Iteration is dirty.
     *        It is dirty if any attribute or dataset is read from or written to
     *        the backend.
     *
     * @return true If dirty.
     * @return false Otherwise.
     */
    bool dirtyRecursive() const;

    /**
     * @brief Link with parent.
     *
     * @param w The Writable representing the parent.
     */
    void linkHierarchy(Writable &w);

    /**
     * @brief Access an iteration in read mode that has potentially not been
     *      parsed yet.
     *
     */
    void runDeferredParseAccess();
}; // Iteration

extern template float Iteration::time<float>() const;

extern template double Iteration::time<double>() const;

extern template long double Iteration::time<long double>() const;

template <typename T>
inline T Iteration::time() const
{
    return this->readFloatingpoint<T>("time");
}

extern template float Iteration::dt<float>() const;

extern template double Iteration::dt<double>() const;

extern template long double Iteration::dt<long double>() const;

template <typename T>
inline T Iteration::dt() const
{
    return this->readFloatingpoint<T>("dt");
}

/**
 * @brief Subclass of Iteration that knows its own index withing the containing
 *        Series.
 */
class IndexedIteration : public Iteration
{
    friend class SeriesIterator;
    friend class WriteIterations;

public:
    using index_t = Iteration::IterationIndex_t;
    index_t const iterationIndex;

private:
    template <typename Iteration_t>
    IndexedIteration(Iteration_t &&it, index_t index)
        : Iteration(std::forward<Iteration_t>(it)), iterationIndex(index)
    {}
};
} // namespace openPMD
