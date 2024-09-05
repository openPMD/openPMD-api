/* Copyright 2023 Franz Poeschel
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

#include "openPMD/IO/ADIOS/ADIOS2Auxiliary.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/IO/InvalidatableFile.hpp"
#include "openPMD/config.hpp"

#if openPMD_HAVE_ADIOS2
#include <adios2.h>
#endif
#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#include <functional>
#include <string>

namespace openPMD
{
class ADIOS2IOHandlerImpl;
}

#if openPMD_HAVE_ADIOS2
namespace openPMD::detail
{
class ADIOS2File;

/*
 * IO-heavy action to be executed upon flushing.
 */
struct BufferedAction
{
    explicit BufferedAction() = default;
    virtual ~BufferedAction() = default;

    BufferedAction(BufferedAction const &other) = delete;
    BufferedAction(BufferedAction &&other) = default;

    BufferedAction &operator=(BufferedAction const &other) = delete;
    BufferedAction &operator=(BufferedAction &&other) = default;

    virtual void run(ADIOS2File &) = 0;
};

struct BufferedGet : BufferedAction
{
    std::string name;
    Parameter<Operation::READ_DATASET> param;

    void run(ADIOS2File &) override;
};

struct DatasetReader
{
    template <typename T>
    static void call(
        ADIOS2IOHandlerImpl *impl,
        BufferedGet &bp,
        adios2::IO &IO,
        adios2::Engine &engine,
        std::string const &fileName);

    static constexpr char const *errorMsg = "ADIOS2: readDataset()";
};

struct BufferedPut : BufferedAction
{
    std::string name;
    Parameter<Operation::WRITE_DATASET> param;

    void run(ADIOS2File &) override;
};

struct WriteDataset
{
    template <typename T>
    static void call(ADIOS2File &ba, BufferedPut &bp);

    template <int n, typename... Params>
    static void call(Params &&...);
};

struct BufferedUniquePtrPut
{
    std::string name;
    Offset offset;
    Extent extent;
    UniquePtrWithLambda<void> data;
    Datatype dtype = Datatype::UNDEFINED;

    void run(ADIOS2File &);
};

struct I_UpdateSpan
{
    virtual void *update() = 0;
    virtual ~I_UpdateSpan() = default;
};

template <typename T>
struct UpdateSpan : I_UpdateSpan
{
    adios2::detail::Span<T> span;

    UpdateSpan(adios2::detail::Span<T>);

    void *update() override;
};

/*
 * Manages per-file information about
 * (1) the file's IO and Engine objects
 * (2) the file's deferred IO-heavy actions
 */
class ADIOS2File
{
    friend struct BufferedGet;
    friend struct BufferedPut;
    friend struct RunUniquePtrPut;
    friend struct WriteDataset;

    using UseGroupTable = adios_defs::UseGroupTable;
    using FlushTarget = adios_defs::FlushTarget;

public:
    ADIOS2File(ADIOS2File const &) = delete;

    /**
     * The full path to the file created on disk, including the
     * containing directory and the file extension, as determined
     * by ADIOS2IOHandlerImpl::fileSuffix().
     * (Meaning, in case of the SST engine, no file suffix since the
     *  SST engine automatically adds its suffix unconditionally)
     */
    std::string m_file;
    /**
     * ADIOS requires giving names to instances of adios2::IO.
     * We make them different from the actual file name, because of the
     * possible following workflow:
     *
     * 1. create file foo.bp
     *    -> would create IO object named foo.bp
     * 2. delete that file
     *    (let's ignore that we don't support deletion yet and call it
     *     preplanning)
     * 3. create file foo.bp a second time
     *    -> would create another IO object named foo.bp
     *    -> craash
     *
     * So, we just give out names based on a counter for IO objects.
     * Hence, next to the actual file name, also store the name for the
     * IO.
     */
    std::string m_IOName;
    adios2::ADIOS &m_ADIOS;
    adios2::IO m_IO;
    /**
     * The default queue for deferred actions.
     * Drained upon ADIOS2File::flush().
     */
    std::vector<std::unique_ptr<BufferedAction>> m_buffer;
    /**
     * When receiving a unique_ptr, we know that the buffer is ours and
     * ours alone. So, for performance reasons, show the buffer to ADIOS2 as
     * late as possible and avoid unnecessary data copies in BP5 triggered
     * by PerformDataWrites().
     */
    std::vector<BufferedUniquePtrPut> m_uniquePtrPuts;
    /**
     * This contains deferred actions that have already been enqueued into
     * ADIOS2, but not yet performed in ADIOS2.
     * We must store them somewhere until the next PerformPuts/Gets, EndStep
     * or Close in ADIOS2 to avoid use after free conditions.
     */
    std::vector<std::unique_ptr<BufferedAction>> m_alreadyEnqueued;
    adios2::Mode m_mode;
    /**
     * The base pointer of an ADIOS2 span might change after reallocations.
     * The frontend will ask the backend for those updated base pointers.
     * Spans given out by the ADIOS2 backend to the frontend are hence
     * identified by an unsigned integer and stored in this member for later
     * retrieval of the updated base pointer.
     * This map is cleared upon flush points.
     */
    std::map<unsigned, std::unique_ptr<I_UpdateSpan>> m_updateSpans;

    /*
     * We call an attribute committed if the step during which it was
     * written has been closed.
     * A committed attribute cannot be modified.
     */
    std::set<std::string> uncommittedAttributes;

    /*
     * The openPMD API will generally create new attributes for each
     * iteration. This results in a growing number of attributes over time.
     * In streaming-based modes, these will be completely sent anew in each
     * iteration. If the following boolean is true, old attributes will be
     * removed upon CLOSE_GROUP.
     * Should not be set to true in persistent backends.
     * Will be automatically set by ADIOS2File::configure_IO depending
     * on chosen ADIOS2 engine and can not be explicitly overridden by user.
     */
    bool optimizeAttributesStreaming = false;

    using ParsePreference = Parameter<Operation::OPEN_FILE>::ParsePreference;
    ParsePreference parsePreference = ParsePreference::UpFront;

    using AttributeMap_t = std::map<std::string, adios2::Params>;

    ADIOS2File(ADIOS2IOHandlerImpl &impl, InvalidatableFile file);

    ~ADIOS2File();

    /**
     * Implementation of destructor, will only run once.
     *
     */
    void finalize();

    UseGroupTable detectGroupTable();

    adios2::Engine &getEngine();

    template <typename BA>
    void enqueue(BA &&ba)
    {
        enqueue<BA>(std::forward<BA>(ba), m_buffer);
    }

    template <typename BA>
    void enqueue(BA &&ba, decltype(m_buffer) &buffer)
    {
        using BA_ = typename std::remove_reference<BA>::type;
        buffer.emplace_back(
            std::unique_ptr<BufferedAction>(new BA_(std::forward<BA>(ba))));
    }

    template <typename... Args>
    void flush(Args &&...args);

    struct ADIOS2FlushParams
    {
        /*
         * Only execute performPutsGets if UserFlush.
         */
        FlushLevel level;
        FlushTarget flushTarget = FlushTarget::Disk;

        ADIOS2FlushParams(FlushLevel level_in) : level(level_in)
        {}

        ADIOS2FlushParams(FlushLevel level_in, FlushTarget flushTarget_in)
            : level(level_in), flushTarget(flushTarget_in)
        {}
    };

    /**
     * Flush deferred IO actions.
     *
     * @param flushParams Flush level and target.
     * @param performPutGets A functor that takes as parameters (1) *this
     *     and (2) the ADIOS2 engine.
     *     Its task is to ensure that ADIOS2 performs Put/Get operations.
     *     Several options for this:
     *     * adios2::Engine::EndStep
     *     * adios2::Engine::Perform(Puts|Gets)
     *     * adios2::Engine::Close
     * @param writeLatePuts Deferred until right before
     *        Engine::EndStep() or Engine::Close():
     *        Running unique_ptr Put()s.
     * @param flushUnconditionally Whether to run the functor even if no
     *     deferred IO tasks had been queued.
     */
    void flush_impl(
        ADIOS2FlushParams flushParams,
        std::function<void(ADIOS2File &, adios2::Engine &)> const
            &performPutGets,
        bool writeLatePuts,
        bool flushUnconditionally);

    /**
     * Overload of flush() that uses adios2::Engine::Perform(Puts|Gets)
     * and does not flush unconditionally.
     *
     */
    void flush_impl(ADIOS2FlushParams, bool writeLatePuts = false);

    /**
     * @brief Begin or end an ADIOS step.
     *
     * @param mode Whether to begin or end a step.
     * @return AdvanceStatus
     */
    AdvanceStatus advance(AdvanceMode mode);

    /*
     * Delete all buffered actions without running them.
     */
    void drop();

    AttributeMap_t const &availableAttributes();

    std::vector<std::string>
    availableAttributesPrefixed(std::string const &prefix);

    /*
     * See description below.
     */
    void invalidateAttributesMap();

    AttributeMap_t const &availableVariables();

    std::vector<std::string>
    availableVariablesPrefixed(std::string const &prefix);

    /*
     * See description below.
     */
    void invalidateVariablesMap();

    void markActive(Writable *);

    // bool isActive(std::string const & path);

    /*
     * streamStatus is NoStream for file-based ADIOS engines.
     * This is relevant for the method ADIOS2File::requireActiveStep,
     * where a step is only opened if the status is OutsideOfStep, but not
     * if NoStream. The rationale behind this is that parsing a Series
     * works differently for file-based and for stream-based engines:
     * * stream-based: Iterations are parsed as they arrive. For parsing an
     *   iteration, the iteration must be awaited.
     *   ADIOS2File::requireActiveStep takes care of this.
     * * file-based: The Series is parsed up front. If no step has been
     *   opened yet, ADIOS2 gives access to all variables and attributes
     *   from all steps. Upon opening a step, only the variables from that
     *   step are shown which hinders parsing. So, until a step is
     *   explicitly opened via ADIOS2IOHandlerImpl::advance, do not open
     *   one.
     *   This is to enable use of ADIOS files without the Streaming API
     *   (i.e. all iterations should be visible to the user upon opening
     *   the Series.)
     *   @todo Add a workflow without up-front parsing of all iterations
     *         for file-based engines.
     *         (This would merely be an optimization since the streaming
     *         API still works with files as intended.)
     *
     */
    enum class StreamStatus
    {
        /**
         * A step is currently active.
         */
        DuringStep,
        /**
         * A stream is active, but no step.
         */
        OutsideOfStep,
        /**
         * Stream has ended.
         */
        StreamOver,
        /**
         * File is not written is streaming fashion.
         * Begin/EndStep will be replaced by simple flushes.
         * Used for:
         * 1) Writing BP4 files without steps despite using the Streaming
         *    API. This is due to the fact that ADIOS2.6.0 requires using
         *    steps to read BP4 files written with steps, so using steps
         *    is opt-in for now.
         *    Notice that while the openPMD API requires ADIOS >= 2.7.0,
         *    the resulting files need to be readable from ADIOS 2.6.0 as
         *    well. This workaround is hence staying until switching to
         *    a new ADIOS schema.
         * 2) Reading with the Streaming API any file that has been written
         *    without steps. This is not a workaround since not using steps,
         *    while inefficient in ADIOS2, is something that we support.
         */
        ReadWithoutStream,
        /**
         * The stream status of a file-based engine will be decided upon
         * opening the engine if in read mode. Up until then, this right
         * here is the status.
         */
        Undecided
    };
    StreamStatus streamStatus = StreamStatus::OutsideOfStep;

    size_t currentStep();

private:
    ADIOS2IOHandlerImpl *m_impl;
    std::optional<adios2::Engine> m_engine; //! ADIOS engine

    /*
     * Not all engines support the CurrentStep() call, so we have to
     * implement this manually.
     */
    size_t m_currentStep = 0;

    /*
     * ADIOS2 does not give direct access to its internal attribute and
     * variable maps, but will instead give access to copies of them.
     * In order to avoid unnecessary copies, we buffer the returned map.
     * The downside of this is that we need to pay attention to invalidate
     * the map whenever an attribute/variable is altered. In that case, we
     * fetch the map anew.
     * If empty, the buffered map has been invalidated and needs to be
     * queried from ADIOS2 again. If full, the buffered map is equivalent to
     * the map that would be returned by a call to
     * IO::Available(Attributes|Variables).
     */
    std::optional<AttributeMap_t> m_availableAttributes;
    std::optional<AttributeMap_t> m_availableVariables;

    std::set<Writable *> m_pathsMarkedAsActive;

    /*
     * Cannot write attributes right after opening the engine
     * https://github.com/ornladios/ADIOS2/issues/3433
     */
    bool initializedDefaults = false;
    /*
     * finalize() will set this true to avoid running twice.
     */
    bool finalized = false;

    UseGroupTable useGroupTable() const;

    void create_IO();

    void configure_IO();
    void configure_IO_Read();
    void configure_IO_Write();
};

template <typename... Args>
void ADIOS2File::flush(Args &&...args)
{
    try
    {
        flush_impl(std::forward<Args>(args)...);
    }
    catch (error::ReadError const &)
    {
        /*
         * We need to take actions out of the buffer, since an exception
         * should reset everything from the current IOHandler->flush() call.
         * However, we cannot simply clear the buffer, since tasks may have
         * been enqueued to ADIOS2 already and we cannot undo that.
         * So, we need to keep the memory alive for the benefit of ADIOS2.
         * Luckily, we have m_alreadyEnqueued for exactly that purpose.
         */
        for (auto &task : m_buffer)
        {
            m_alreadyEnqueued.emplace_back(std::move(task));
        }
        m_buffer.clear();
        throw;
    }
}
} // namespace openPMD::detail
#endif
