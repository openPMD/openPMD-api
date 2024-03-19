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
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Writable.hpp"

#include <exception>
#include <iostream>
#include <tuple>

namespace openPMD
{
using internal::CloseStatus;
using internal::DeferredParseAccess;

Iteration::Iteration() : Attributable(NoInit())
{
    setData(std::make_shared<Data_t>());
    setTime(static_cast<double>(0));
    setDt(static_cast<double>(1));
    setTimeUnitSI(1);
    meshes.writable().ownKeyWithinParent = "meshes";
    particles.writable().ownKeyWithinParent = "particles";
}

template <typename T>
Iteration &Iteration::setTime(T newTime)
{
    static_assert(
        std::is_floating_point<T>::value,
        "Type of attribute must be floating point");

    setAttribute("time", newTime);
    return *this;
}

template <typename T>
Iteration &Iteration::setDt(T newDt)
{
    static_assert(
        std::is_floating_point<T>::value,
        "Type of attribute must be floating point");

    setAttribute("dt", newDt);
    return *this;
}

double Iteration::timeUnitSI() const
{
    return getAttribute("timeUnitSI").get<double>();
}

Iteration &Iteration::setTimeUnitSI(double newTimeUnitSI)
{
    setAttribute("timeUnitSI", newTimeUnitSI);
    return *this;
}

using iterator_t = Container<Iteration, Iteration::IterationIndex_t>::iterator;

Iteration &Iteration::close(bool _flush)
{
    auto &it = get();
    StepStatus flag = getStepStatus();
    // update close status
    switch (it.m_closed)
    {
    case CloseStatus::Open:
    case CloseStatus::ClosedInFrontend:
        it.m_closed = CloseStatus::ClosedInFrontend;
        break;
    case CloseStatus::ClosedTemporarily:
        // should we bother to reopen?
        if (dirtyRecursive())
        {
            // let's reopen
            it.m_closed = CloseStatus::ClosedInFrontend;
        }
        else
        {
            // don't reopen
            it.m_closed = CloseStatus::ClosedInBackend;
        }
        break;
    case CloseStatus::ParseAccessDeferred:
    case CloseStatus::ClosedInBackend:
        // just keep it like it is
        // (this means that closing an iteration that has not been parsed
        // yet keeps it re-openable)
        break;
    }
    if (_flush)
    {
        if (flag == StepStatus::DuringStep)
        {
            endStep();
            setStepStatus(StepStatus::NoStep);
        }
        else
        {
            // flush things manually
            Series s = retrieveSeries();
            // figure out my iteration number
            auto begin = s.indexOf(*this);
            auto end = begin;
            ++end;

            s.flush_impl(begin, end, {FlushLevel::UserFlush});
        }
    }
    else
    {
        if (flag == StepStatus::DuringStep)
        {
            throw std::runtime_error(
                "Using deferred Iteration::close "
                "unimplemented in auto-stepping mode.");
        }
    }
    return *this;
}

Iteration &Iteration::open()
{
    auto &it = get();
    if (it.m_closed == CloseStatus::ParseAccessDeferred)
    {
        it.m_closed = CloseStatus::Open;
        runDeferredParseAccess();
    }
    Series s = retrieveSeries();
    // figure out my iteration number
    auto begin = s.indexOf(*this);
    s.openIteration(begin->first, *this);
    IOHandler()->flush(internal::defaultFlushParams);
    return *this;
}

bool Iteration::closed() const
{
    switch (get().m_closed)
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
    throw std::runtime_error("Unreachable!");
}

bool Iteration::closedByWriter() const
{
    using bool_type = unsigned char;
    if (containsAttribute("closed"))
    {
        return getAttribute("closed").get<bool_type>() == 0u ? false : true;
    }
    else
    {
        return false;
    }
}

void Iteration::flushFileBased(
    std::string const &filename,
    IterationIndex_t i,
    internal::FlushParams const &flushParams)
{
    /* Find the root point [Series] of this file,
     * meshesPath and particlesPath are stored there */
    Series s = retrieveSeries();

    if (!written())
    {
        /* create file */
        Parameter<Operation::CREATE_FILE> fCreate;
        fCreate.name = filename;
        IOHandler()->enqueue(IOTask(&s.writable(), fCreate));

        /*
         * If it was written before, then in the context of another iteration.
         */
        auto &attr = s.get().m_rankTable.m_attributable;
        attr.setWritten(false, Attributable::EnqueueAsynchronously::Yes);
        s.get()
            .m_rankTable.m_attributable.get()
            .m_writable.abstractFilePosition.reset();
        s.flushRankTable();

        /* create basePath */
        Parameter<Operation::CREATE_PATH> pCreate;
        pCreate.path = auxiliary::replace_first(s.basePath(), "%T/", "");
        IOHandler()->enqueue(IOTask(&s.iterations, pCreate));

        /* create iteration path */
        pCreate.path = std::to_string(i);
        IOHandler()->enqueue(IOTask(this, pCreate));
    }
    else
    {
        // operations for read/read-write mode
        /* open file */
        s.openIteration(i, *this);
    }

    switch (flushParams.flushLevel)
    {
    case FlushLevel::CreateOrOpenFiles:
        break;
    case FlushLevel::SkeletonOnly:
    case FlushLevel::InternalFlush:
    case FlushLevel::UserFlush:
        flush(flushParams);
        break;
    }
}

void Iteration::flushGroupBased(
    IterationIndex_t i, internal::FlushParams const &flushParams)
{
    if (!written())
    {
        /* create iteration path */
        Parameter<Operation::CREATE_PATH> pCreate;
        pCreate.path = std::to_string(i);
        IOHandler()->enqueue(IOTask(this, pCreate));
    }

    switch (flushParams.flushLevel)
    {
    case FlushLevel::CreateOrOpenFiles:
        break;
    case FlushLevel::SkeletonOnly:
    case FlushLevel::InternalFlush:
    case FlushLevel::UserFlush:
        flush(flushParams);
        break;
    }
}

void Iteration::flushVariableBased(
    IterationIndex_t i, internal::FlushParams const &flushParams)
{
    if (!written())
    {
        /* create iteration path */
        Parameter<Operation::OPEN_PATH> pOpen;
        pOpen.path = "";
        IOHandler()->enqueue(IOTask(this, pOpen));
    }

    switch (flushParams.flushLevel)
    {
    case FlushLevel::CreateOrOpenFiles:
        return;
    case FlushLevel::SkeletonOnly:
    case FlushLevel::InternalFlush:
    case FlushLevel::UserFlush:
        flush(flushParams);
        break;
    }

    if (!written())
    {
        /* create iteration path */
        Parameter<Operation::OPEN_PATH> pOpen;
        pOpen.path = "";
        IOHandler()->enqueue(IOTask(this, pOpen));
        /*
         * In v-based encoding, the snapshot attribute must always be written.
         * Reason: Even in backends that don't support changing attributes,
         * variable-based iteration encoding can be used to write one single
         * iteration. Then, this attribute determines which iteration it is.
         */
        Parameter<Operation::WRITE_ATT> wAttr;
        wAttr.changesOverSteps =
            Parameter<Operation::WRITE_ATT>::ChangesOverSteps::IfPossible;
        wAttr.name = "snapshot";
        wAttr.resource = (unsigned long long)i;
        wAttr.dtype = Datatype::ULONGLONG;
        IOHandler()->enqueue(IOTask(this, wAttr));
    }
}

void Iteration::flush(internal::FlushParams const &flushParams)
{
    Parameter<Operation::TOUCH> touch;
    IOHandler()->enqueue(IOTask(&writable(), touch));
    if (access::readOnly(IOHandler()->m_frontendAccess))
    {
        for (auto &m : meshes)
            m.second.flush(m.first, flushParams);
        for (auto &species : particles)
            species.second.flush(species.first, flushParams);
    }
    else
    {
        /* Find the root point [Series] of this file,
         * meshesPath and particlesPath are stored there */
        Series s = retrieveSeries();

        if (!meshes.empty() || s.containsAttribute("meshesPath"))
        {
            if (!s.containsAttribute("meshesPath"))
            {
                s.setMeshesPath("meshes/");
                s.flushMeshesPath();
            }
            meshes.flush(s.meshesPath(), flushParams);
            for (auto &m : meshes)
                m.second.flush(m.first, flushParams);
        }
        else
        {
            meshes.setDirty(false);
        }

        if (!particles.empty() || s.containsAttribute("particlesPath"))
        {
            if (!s.containsAttribute("particlesPath"))
            {
                s.setParticlesPath("particles/");
                s.flushParticlesPath();
            }
            particles.flush(s.particlesPath(), flushParams);
            for (auto &species : particles)
                species.second.flush(species.first, flushParams);
        }
        else
        {
            particles.setDirty(false);
        }

        flushAttributes(flushParams);
    }
    if (flushParams.flushLevel != FlushLevel::SkeletonOnly)
    {
        setDirty(false);
        meshes.setDirty(false);
        particles.setDirty(false);
    }
}

void Iteration::deferParseAccess(DeferredParseAccess dr)
{
    get().m_deferredParseAccess =
        std::make_optional<DeferredParseAccess>(std::move(dr));
}

void Iteration::reread(std::string const &path)
{
    if (get().m_deferredParseAccess.has_value())
    {
        throw std::runtime_error(
            "[Iteration] Internal control flow error: Trying to reread an "
            "iteration that has not yet been read for its first time.");
    }
    read_impl(path);
}

void Iteration::readFileBased(
    std::string const &filePath, std::string const &groupPath, bool doBeginStep)
{
    if (doBeginStep)
    {
        /*
         * beginStep() must take care to open files
         */
        beginStep(/* reread = */ false);
    }
    auto series = retrieveSeries();

    series.readOneIterationFileBased(filePath);
    get().m_overrideFilebasedFilename = filePath;

    read_impl(groupPath);
}

void Iteration::readGorVBased(std::string const &groupPath, bool doBeginStep)
{
    if (doBeginStep)
    {
        /*
         * beginStep() must take care to open files
         */
        beginStep(/* reread = */ false);
    }
    read_impl(groupPath);
}

void Iteration::read_impl(std::string const &groupPath)
{
    Parameter<Operation::OPEN_PATH> pOpen;
    pOpen.path = groupPath;
    IOHandler()->enqueue(IOTask(this, pOpen));

    using DT = Datatype;
    Parameter<Operation::READ_ATT> aRead;

    aRead.name = "dt";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::FLOAT)
        setDt(Attribute(*aRead.resource).get<float>());
    else if (*aRead.dtype == DT::DOUBLE)
        setDt(Attribute(*aRead.resource).get<double>());
    else if (*aRead.dtype == DT::LONG_DOUBLE)
        setDt(Attribute(*aRead.resource).get<long double>());
    // conversion cast if a backend reports an integer type
    else if (auto val = Attribute(*aRead.resource).getOptional<double>();
             val.has_value())
        setDt(val.value());
    else
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            "Unexpected Attribute datatype for 'dt' (expected double, found " +
                datatypeToString(Attribute(*aRead.resource).dtype) + ")");

    aRead.name = "time";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::FLOAT)
        setTime(Attribute(*aRead.resource).get<float>());
    else if (*aRead.dtype == DT::DOUBLE)
        setTime(Attribute(*aRead.resource).get<double>());
    else if (*aRead.dtype == DT::LONG_DOUBLE)
        setTime(Attribute(*aRead.resource).get<long double>());
    // conversion cast if a backend reports an integer type
    else if (auto val = Attribute(*aRead.resource).getOptional<double>();
             val.has_value())
        setTime(val.value());
    else
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            "Unexpected Attribute datatype for 'time' (expected double, "
            "found " +
                datatypeToString(Attribute(*aRead.resource).dtype) + ")");

    aRead.name = "timeUnitSI";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (auto val = Attribute(*aRead.resource).getOptional<double>();
        val.has_value())
        setTimeUnitSI(val.value());
    else
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            "Unexpected Attribute datatype for 'timeUnitSI' (expected double, "
            "found " +
                datatypeToString(Attribute(*aRead.resource).dtype) + ")");

    /* Find the root point [Series] of this file,
     * meshesPath and particlesPath are stored there */
    Series s = retrieveSeries();

    Parameter<Operation::LIST_PATHS> pList;
    std::string version = s.openPMD();
    bool hasMeshes = false;
    bool hasParticles = false;
    if (version == "1.0.0" || version == "1.0.1")
    {
        IOHandler()->enqueue(IOTask(this, pList));
        IOHandler()->flush(internal::defaultFlushParams);
        hasMeshes = std::count(
                        pList.paths->begin(),
                        pList.paths->end(),
                        auxiliary::replace_last(s.meshesPath(), "/", "")) == 1;
        hasParticles =
            std::count(
                pList.paths->begin(),
                pList.paths->end(),
                auxiliary::replace_last(s.particlesPath(), "/", "")) == 1;
        pList.paths->clear();
    }
    else
    {
        hasMeshes = s.containsAttribute("meshesPath");
        hasParticles = s.containsAttribute("particlesPath");
    }

    if (hasMeshes)
    {
        try
        {
            readMeshes(s.meshesPath());
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read meshes in iteration " << groupPath
                      << " and will skip them due to read error:\n"
                      << err.what() << std::endl;
            meshes = {};
        }
    }
    meshes.setDirty(false);

    if (hasParticles)
    {
        try
        {
            readParticles(s.particlesPath());
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read particles in iteration " << groupPath
                      << " and will skip them due to read error:\n"
                      << err.what() << std::endl;
            particles = {};
        }
    }
    particles.setDirty(false);

    readAttributes(ReadMode::FullyReread);
#ifdef openPMD_USE_INVASIVE_TESTS
    if (containsAttribute("__openPMD_internal_fail"))
    {
        if (getAttribute("__openPMD_internal_fail").get<std::string>() ==
            "asking for trouble")
        {
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::Other,
                {},
                "Deliberately failing this iteration for testing purposes");
        }
    }
#endif
}

void Iteration::readMeshes(std::string const &meshesPath)
{
    Parameter<Operation::OPEN_PATH> pOpen;
    Parameter<Operation::LIST_PATHS> pList;

    pOpen.path = meshesPath;
    IOHandler()->enqueue(IOTask(&meshes, pOpen));

    meshes.readAttributes(ReadMode::FullyReread);

    internal::EraseStaleEntries<decltype(meshes)> map{meshes};

    /* obtain all non-scalar meshes */
    IOHandler()->enqueue(IOTask(&meshes, pList));
    IOHandler()->flush(internal::defaultFlushParams);

    Parameter<Operation::LIST_ATTS> aList;
    for (auto const &mesh_name : *pList.paths)
    {
        Mesh &m = map[mesh_name];
        pOpen.path = mesh_name;
        aList.attributes->clear();
        IOHandler()->enqueue(IOTask(&m, pOpen));
        IOHandler()->enqueue(IOTask(&m, aList));
        IOHandler()->flush(internal::defaultFlushParams);

        auto att_begin = aList.attributes->begin();
        auto att_end = aList.attributes->end();
        auto value = std::find(att_begin, att_end, "value");
        auto shape = std::find(att_begin, att_end, "shape");
        if (value != att_end && shape != att_end)
        {
            MeshRecordComponent &mrc = m;
            IOHandler()->enqueue(IOTask(&mrc, pOpen));
            IOHandler()->flush(internal::defaultFlushParams);
            mrc.get().m_isConstant = true;
        }
        try
        {
            m.read();
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read mesh with name '" << mesh_name
                      << "' and will skip it due to read error:\n"
                      << err.what() << std::endl;
            map.forget(mesh_name);
        }
    }

    /* obtain all scalar meshes */
    Parameter<Operation::LIST_DATASETS> dList;
    IOHandler()->enqueue(IOTask(&meshes, dList));
    IOHandler()->flush(internal::defaultFlushParams);

    Parameter<Operation::OPEN_DATASET> dOpen;
    for (auto const &mesh_name : *dList.datasets)
    {
        Mesh &m = map[mesh_name];
        dOpen.name = mesh_name;
        IOHandler()->enqueue(IOTask(&m, dOpen));
        IOHandler()->flush(internal::defaultFlushParams);
        MeshRecordComponent &mrc = m;
        IOHandler()->enqueue(IOTask(&mrc, dOpen));
        IOHandler()->flush(internal::defaultFlushParams);
        mrc.setWritten(false, Attributable::EnqueueAsynchronously::No);
        mrc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        mrc.setWritten(true, Attributable::EnqueueAsynchronously::No);
        try
        {
            m.read();
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read mesh with name '" << mesh_name
                      << "' and will skip it due to read error:\n"
                      << err.what() << std::endl;
            map.forget(mesh_name);
        }
    }
}

void Iteration::readParticles(std::string const &particlesPath)
{
    Parameter<Operation::OPEN_PATH> pOpen;
    Parameter<Operation::LIST_PATHS> pList;

    pOpen.path = particlesPath;
    IOHandler()->enqueue(IOTask(&particles, pOpen));

    particles.readAttributes(ReadMode::FullyReread);

    /* obtain all particle species */
    pList.paths->clear();
    IOHandler()->enqueue(IOTask(&particles, pList));
    IOHandler()->flush(internal::defaultFlushParams);

    internal::EraseStaleEntries<decltype(particles)> map{particles};
    for (auto const &species_name : *pList.paths)
    {
        ParticleSpecies &p = map[species_name];
        pOpen.path = species_name;
        IOHandler()->enqueue(IOTask(&p, pOpen));
        IOHandler()->flush(internal::defaultFlushParams);
        try
        {
            p.read();
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read particle species with name '"
                      << species_name
                      << "' and will skip it due to read error:\n"
                      << err.what() << std::endl;
            map.forget(species_name);
        }
    }
}

auto Iteration::beginStep(bool reread) -> BeginStepStatus
{
    BeginStepStatus res;
    auto series = retrieveSeries();
    return beginStep({*this}, series, reread);
}

auto Iteration::beginStep(
    std::optional<Iteration> thisObject,
    Series &series,
    bool reread,
    std::set<IterationIndex_t> const &ignoreIterations) -> BeginStepStatus
{
    BeginStepStatus res;
    using IE = IterationEncoding;
    // Initialize file with this to quiet warnings
    // The following switch is comprehensive
    internal::AttributableData *file = nullptr;
    switch (series.iterationEncoding())
    {
    case IE::fileBased:
        if (thisObject.has_value())
        {
            file = &static_cast<Attributable &>(*thisObject).get();
        }
        else
        {
            throw error::Internal(
                "Advancing a step in file-based iteration encoding is "
                "iteration-specific.");
        }
        break;
    case IE::groupBased:
    case IE::variableBased:
        file = &series.get();
        break;
    }

    AdvanceStatus status;
    if (thisObject.has_value())
    {
        status = series.advance(
            AdvanceMode::BEGINSTEP,
            *file,
            series.indexOf(*thisObject),
            *thisObject);
    }
    else
    {
        status = series.advance(AdvanceMode::BEGINSTEP);
    }

    switch (status)
    {
    case AdvanceStatus::OVER:
        res.stepStatus = status;
        return res;
    case AdvanceStatus::OK:
    case AdvanceStatus::RANDOMACCESS:
        break;
    }

    // re-read -> new datasets might be available
    auto IOHandl = series.IOHandler();
    if (reread && status != AdvanceStatus::RANDOMACCESS &&
        (series.iterationEncoding() == IE::groupBased ||
         series.iterationEncoding() == IE::variableBased) &&
        access::read(series.IOHandler()->m_frontendAccess))
    {
        bool previous = series.iterations.written();
        series.iterations.setWritten(
            false, Attributable::EnqueueAsynchronously::Yes);
        auto oldStatus = IOHandl->m_seriesStatus;
        IOHandl->m_seriesStatus = internal::SeriesStatus::Parsing;
        try
        {
            res.iterationsInOpenedStep = series.readGorVBased(
                /* do_always_throw_errors = */ true,
                /* init = */ false,
                ignoreIterations);
        }
        catch (...)
        {
            IOHandl->m_seriesStatus = oldStatus;
            throw;
        }
        IOHandl->m_seriesStatus = oldStatus;
        series.iterations.setWritten(
            previous, Attributable::EnqueueAsynchronously::Yes);
    }

    res.stepStatus = status;
    return res;
}

void Iteration::endStep()
{
    using IE = IterationEncoding;
    auto series = retrieveSeries();
    // Initialize file with this to quiet warnings
    // The following switch is comprehensive
    internal::AttributableData *file = nullptr;
    switch (series.iterationEncoding())
    {
    case IE::fileBased:
        file = &Attributable::get();
        break;
    case IE::groupBased:
    case IE::variableBased:
        file = &series.get();
        break;
    }
    // @todo filebased check
    series.advance(AdvanceMode::ENDSTEP, *file, series.indexOf(*this), *this);
    series.get().m_currentlyActiveIterations.clear();
}

StepStatus Iteration::getStepStatus()
{
    Series s = retrieveSeries();
    switch (s.iterationEncoding())
    {
        using IE = IterationEncoding;
    case IE::fileBased:
        return get().m_stepStatus;
    case IE::groupBased:
    case IE::variableBased:
        return s.get().m_stepStatus;
    default:
        throw std::runtime_error("[Iteration] unreachable");
    }
}

void Iteration::setStepStatus(StepStatus status)
{
    Series s = retrieveSeries();
    switch (s.iterationEncoding())
    {
        using IE = IterationEncoding;
    case IE::fileBased:
        get().m_stepStatus = status;
        break;
    case IE::groupBased:
    case IE::variableBased:
        s.get().m_stepStatus = status;
        break;
    default:
        throw std::runtime_error("[Iteration] unreachable");
    }
}

void Iteration::linkHierarchy(Writable &w)
{
    Attributable::linkHierarchy(w);
    meshes.linkHierarchy(this->writable());
    particles.linkHierarchy(this->writable());
}

void Iteration::runDeferredParseAccess()
{
    if (access::read(IOHandler()->m_frontendAccess))
    {
        auto &it = get();
        if (!it.m_deferredParseAccess.has_value())
        {
            return;
        }
        auto const &deferred = it.m_deferredParseAccess.value();

        auto oldStatus = IOHandler()->m_seriesStatus;
        IOHandler()->m_seriesStatus = internal::SeriesStatus::Parsing;
        try
        {
            if (deferred.fileBased)
            {
                readFileBased(
                    deferred.filename, deferred.path, deferred.beginStep);
            }
            else
            {
                readGorVBased(deferred.path, deferred.beginStep);
            }
        }
        catch (...)
        {
            // reset this thing
            it.m_deferredParseAccess = std::optional<DeferredParseAccess>();
            IOHandler()->m_seriesStatus = oldStatus;
            throw;
        }
        // reset this thing
        it.m_deferredParseAccess = std::optional<DeferredParseAccess>();
        IOHandler()->m_seriesStatus = oldStatus;
    }
}

template float Iteration::time<float>() const;
template double Iteration::time<double>() const;
template long double Iteration::time<long double>() const;

template float Iteration::dt<float>() const;
template double Iteration::dt<double>() const;
template long double Iteration::dt<long double>() const;

template Iteration &Iteration::setTime<float>(float time);
template Iteration &Iteration::setTime<double>(double time);
template Iteration &Iteration::setTime<long double>(long double time);

template Iteration &Iteration::setDt<float>(float dt);
template Iteration &Iteration::setDt<double>(double dt);
template Iteration &Iteration::setDt<long double>(long double dt);
} // namespace openPMD
