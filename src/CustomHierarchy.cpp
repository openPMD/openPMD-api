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

#include "openPMD/CustomHierarchy.hpp"
#include "openPMD/auxiliary/Defer.hpp"
#include "openPMD/backend/Container.hpp"

namespace openPMD
{
namespace traits
{
    template <typename Container_const_or_not>
    void DeferredInitPolicy<Container<CustomHierarchy>>::call(
        Container_const_or_not &container)
    {
        container.writable().objectType.ifGroup(
            [&](auto &group_data) { syncContainers(container, group_data); });
        // if constexpr (!std::is_const_v<Container_const_or_not>)
        // {
        //     if (!access::read(container.IOHandler()->m_backendAccess))
        //     {
        //         container.writable().objectType.ifGroup([&](auto &group_data)
        //         {
        //             if (group_data.phantom || group_data.is_read)
        //             {
        //                 return;
        //             }
        //             dynamic_cast<CustomHierarchy *>(&container)->read(1);
        //         });
        //     }
        // }
    }
    template void DeferredInitPolicy<Container<CustomHierarchy>>::call(
        Container<CustomHierarchy> &);
    template void DeferredInitPolicy<Container<CustomHierarchy>>::call(
        Container<CustomHierarchy> const &);

    template <typename Container_const_or_not>
    void DeferredInitPolicy<Container<CustomHierarchy>>::syncContainers(
        Container_const_or_not &container,
        internal::object_type::GroupMetaData const &group_data)
    {
        // Need to sync backend objects into the CustomHierarchy instance
        // Need to be a bit sneaky, we must modify my_container&, but this
        // method might be called as const. shared_ptr<>s implement interior
        // mutability, so use that here.

        // auto &container_front = container.container_front();
        auto &container_front = container.m_containerData->m_container;
        auto &container_back = group_data.m_children;

        auto size_front = container_front.size();
        auto size_back = container_back.size();

        if (size_front == size_back)
        {
            return;
        }
        else if (size_front > size_back)
        {
            std::stringstream error;
            auto print = [&error](auto const &map) -> std::stringstream & {
                if (map.empty())
                {
                    error << "[]";
                }
                else
                {
                    error << '[';
                    auto it = map.begin();
                    error << (it++)->first;
                    auto end = map.end();
                    for (; it != end; ++it)
                    {
                        error << ", " << it->first;
                    }
                    error << ']';
                }
                return error;
            };
            error << "CustomHierarchy went into illegal state at '"
                  << container.myPath().openPMDPath()
                  << "':\nfront container: ";
            print(container_front) << "\nback container:  ";
            print(container_back) << '\n';
            throw error::Internal(error.str());
        }

        GenerationPolicy<CustomHierarchy> gen;

        auto it = container_front.begin();
        auto end = container_front.end();
        for (auto const &[key, attributable] : container_back)
        {
            if (it == end || it->first != key)
            {
                // under the invariant that the front container contains no
                // elements that are not present in the back container, it
                // now points to an entry past the to-be-inserted key
                it = container_front.emplace_hint(
                    it, key, CustomHierarchy(attributable));
                gen(container, it);
            }
            ++it;
        }
    }
} // namespace traits

// TODO: Add visitor over real type, using HierarchyVisitor class
//       Then, maybe even implement HierarchyVisitor in terms of CustomHierarchy

template <>
auto ConvertibleContainer<CustomHierarchy>::isDataset() -> bool
{
    return this->writable().objectType.isDataset();
}

template <>
auto ConvertibleContainer<CustomHierarchy>::asDataset() -> RecordComponent
{
    auto castableToConstantDataset = [&]() {
        return this->containsAttribute("value") &&
            this->containsAttribute("shape") && this->empty();
    };
    auto initDataset = [&]() { this->writable().objectType.initDataset(); };
    auto makeResult = [&]() {
        RecordComponent res;
        res.get().cloneFrom(*this->m_attri);
        return res;
    };
    if (this->written())
    {
        if (this->writable().objectType.isGroup())
        {
            if (!castableToConstantDataset())
            {
                throw error::WrongAPIUsage(
                    "Can't cast a group object into a dataset.");
            }
            else
            { // Also, what about shape elision in SoA structures?
                // Probably does not make sense outside of Meshes / Particles
                initDataset();
                auto res = makeResult();
                res.get().isConstant() = true;
                res.read();
                return res;
            }
        }
        return makeResult();
    }
    else
    {
        if (this->writable().objectType.isDataset() ||
            access::write(this->IOHandler()->m_frontendAccess) ||
            this->IOHandler()->m_seriesStatus ==
                internal::SeriesStatus::Parsing)
        {
            // this is now a dataset.
            initDataset();
            return makeResult();
        }
        else
        {
            // TODO check if this has valid uses
            throw error::WrongAPIUsage(
                "Read only: Trying to access a non-written object as a "
                "RecordComponent.");
        }
    }
}

CustomHierarchy::CustomHierarchy() : ConvertibleContainer(NoInit{})
{
    setData(std::make_shared<Data_t>());
}

CustomHierarchy::CustomHierarchy(NoInit) : ConvertibleContainer(NoInit{})
{}

CustomHierarchy::CustomHierarchy(
    std::shared_ptr<internal::SharedAttributableData> other)
    : ConvertibleContainer(NoInit{})
{
    auto data = std::make_shared<Data_t>();
    data->asSharedPtrOfAttributable() = std::move(other);
    setData(std::move(data));
}

CustomHierarchy::CustomHierarchy(Attributable const &other)
    : CustomHierarchy(other.m_attri->asSharedPtrOfAttributable())
{}

auto CustomHierarchy::read(size_t const max_recursion_depth) -> CustomHierarchy
{
    auxiliary::opaque_defer_type reset_parsing_status;
    if (IOHandler()->m_seriesStatus != internal::SeriesStatus::Parsing)
    {
        IOHandler()->m_seriesStatus = internal::SeriesStatus::Parsing;
        reset_parsing_status = auxiliary::defer([&]() {
            IOHandler()->m_seriesStatus = internal::SeriesStatus::Default;
        });
    }
    if (!writable().written)
    {
        auto do_throw = [&]() {
            throw error::WrongAPIUsage(
                "Cannot read contents of CustomHierarchy path '" +
                myPath().openPMDPath() +
                "', since the backend does not yet / no longer know about this "
                "object. Ensure that the object is open (e.g. by opening its "
                "containing Iteration or by reading parent paths first)");
        };
        auto parent = writable().parent;
        if (!parent)
        {
            do_throw();
        }
        Attributable parent_attributable(NoInit{});
        parent_attributable.setData(
            std::shared_ptr<internal::AttributableData>{
                parent->attributable, [](auto const *) {}});
        CustomHierarchy parent_(parent_attributable);
        parent_.read(1);
        if (!writable() /* still not */.written)
        {
            do_throw();
        }
    }
    /*
     * Convention for CustomHierarchy::flush and CustomHierarchy::read:
     * Path is created/opened already at entry point of method, method needs
     * to create/open path for contained subpaths.
     */

    Attributable::readAttributes(ReadMode::FullyReread);

    if (writable().objectType.isDataset())
    {
        return *this;
    }

    auto do_recurse = [this, max_recursion_depth](CustomHierarchy &child) {
        switch (max_recursion_depth)
        {
        case 0:
            IOHandler()->flush(internal::defaultFlushParams);
            child.read(0);
            break;
        case 1:
            break;
        default:
            IOHandler()->flush(internal::defaultFlushParams);
            child.read(max_recursion_depth - 1);
            break;
        }
    };

    Parameter<Operation::LIST_PATHS> pList;
    IOHandler()->enqueue(IOTask(this, pList));

    Parameter<Operation::LIST_DATASETS> dList;
    IOHandler()->enqueue(IOTask(this, dList));

    IOHandler()->flush(internal::defaultFlushParams);

    auto &container_back_ = container_back(/* verify = */ false);
    for (auto const &path : *pList.paths)
    {
        if (auto it = container_back_.find(path);
            it != container_back_.end() && it->second->m_writable.written)
        {
            // Is already known
            continue;
        }
        Parameter<Operation::OPEN_PATH> pOpen;
        pOpen.path = path;
        auto &subpath = this->operator[](path);
        subpath.linkHierarchy(this->writable());
        IOHandler()->enqueue(IOTask(&subpath, pOpen));
        do_recurse(subpath);
    }

    for (auto const &path : *dList.datasets)
    {
        if (auto it = container_back_.find(path);
            it != container_back_.end() && it->second->m_writable.written)
        {
            // Is already known
            continue;
        }
        Parameter<Operation::OPEN_DATASET> dOpen;
        dOpen.name = path;

        auto &subpath = this->operator[](path);
        subpath.linkHierarchy(this->writable());
        IOHandler()->enqueue(IOTask(&subpath, dOpen));
        IOHandler()->flush(internal::defaultFlushParams);

        subpath.setWritten(false, Attributable::EnqueueAsynchronously::No);
        subpath.asDataset().resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        subpath.setWritten(true, Attributable::EnqueueAsynchronously::No);
        do_recurse(subpath);
    }

    setDirty(false);
    IOHandler()->flush(internal::defaultFlushParams);

    return *this;
}

void CustomHierarchy::flush(
    std::string const & /* path */, internal::FlushParams const &)
{
    throw std::runtime_error("Use Attributable::customHierarchyFlush instead!");
}

void CustomHierarchy::linkHierarchy(Writable &w)
{
    Attributable::linkHierarchy(w);
}

void CustomHierarchy::printRecursively()
{
    std::cout << &writable() << " [" << dirty() << "," << dirtyRecursive()
              << "]  ";
    if (empty())
    {
        std::cout << "<EMPTY>\n";
        return;
    }
    else
    {
        std::cout << writable().ownKeyWithinParent << '\n';
        printRecursively("");
        std::cout.flush();
    }
}

void CustomHierarchy::printRecursively(std::string indent)
{
    auto print_indent = [&indent]() { std::cout << indent; };
    auto it = begin();
    auto end_ = end();
    if (it == end_)
    {
        return;
    }
    auto prev = it;
    ++it;
    auto next_indent = indent + "│ ";
    for (; it != end_; prev = it, ++it)
    {
        std::cout << &prev->second.writable() << " [" << prev->second.dirty()
                  << "," << prev->second.dirtyRecursive() << "]  ";
        print_indent();
        std::cout << "├─" << prev->first << '\n';
        prev->second.printRecursively(next_indent);
    }
    std::cout << &prev->second.writable() << " [" << prev->second.dirty() << ","
              << prev->second.dirtyRecursive() << "]  ";
    print_indent();
    std::cout << "└─" << prev->first << '\n';
    prev->second.printRecursively(indent + "  ");
}

void CustomHierarchy::visitHierarchyImpl(HierarchyVisitor &v, bool recursive)
{
    visitHierarchyContainer<CustomHierarchy>(v, recursive);
}
} // namespace openPMD

#if 0
#include "openPMD/Dataset.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/Writable.hpp"

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>

// @todo add handselected choice of [:punct:] characters to this
// using a macro here to make string interpolation simpler
#define OPENPMD_LEGAL_IDENTIFIER_CHARS "[:alnum:]_"
#define OPENPMD_SINGLE_GLOBBING_CHAR "%"
#define OPENPMD_DOUBLE_GLOBBING_CHAR "%%"

namespace
{
template <typename Iterator>
std::string
concatWithSep(Iterator &&begin, Iterator const &end, std::string const &sep)
{
    if (begin == end)
    {
        return "";
    }
    std::stringstream res;
    res << *(begin++);
    for (; begin != end; ++begin)
    {
        res << sep << *begin;
    }
    return res.str();
}

std::string
concatWithSep(std::vector<std::string> const &v, std::string const &sep)
{
    return concatWithSep(v.begin(), v.end(), sep);
}

// Not specifying std::regex_constants::optimize here, only using it where
// it makes sense to.
constexpr std::regex_constants::syntax_option_type regex_flags =
    std::regex_constants::egrep;

template <typename OutParam>
void setDefaultMeshesParticlesPath(
    std::vector<std::string> const &meshes,
    std::vector<std::string> const &particles,
    OutParam &writeTarget)
{
    std::regex is_default_path_specification(
        "[" OPENPMD_LEGAL_IDENTIFIER_CHARS "]+/",
        regex_flags | std::regex_constants::optimize);
    constexpr char const *default_default_mesh = "meshes";
    constexpr char const *default_default_particle = "particles";
    for (auto [vec, defaultPath, default_default] :
         {std::make_tuple(
              &meshes, &writeTarget.m_defaultMeshesPath, default_default_mesh),
          std::make_tuple(
              &particles,
              &writeTarget.m_defaultParticlesPath,
              default_default_particle)})
    {
        bool set_default = true;
        /*
         * The first eligible path in meshesPath/particlesPath is used as
         * the default, "meshes"/"particles" otherwise.
         */
        for (auto const &path : *vec)
        {
            if (std::regex_match(path, is_default_path_specification))
            {
                *defaultPath = openPMD::auxiliary::replace_last(path, "/", "");
                set_default = false;
                break;
            }
        }
        if (set_default)
        {
            *defaultPath = default_default;
        }
    }
}

bool anyPathRegexMatches(
    std::regex const &regex, std::vector<std::string> const &path)
{
    std::string pathToMatch = '/' + concatWithSep(path, "/") + '/';
    return std::regex_match(pathToMatch, regex);
}
} // namespace

namespace openPMD
{
namespace internal
{
    namespace
    {
        std::string globToRegexLongForm(std::string const &glob)
        {
            return auxiliary::replace_all(
                auxiliary::replace_all(
                    glob,
                    OPENPMD_DOUBLE_GLOBBING_CHAR,
                    "([" OPENPMD_LEGAL_IDENTIFIER_CHARS "/]*)"),
                OPENPMD_SINGLE_GLOBBING_CHAR,
                "([" OPENPMD_LEGAL_IDENTIFIER_CHARS "]*)");
        }

        std::string globToRegexShortForm(std::string const &glob)
        {
            return "[" OPENPMD_LEGAL_IDENTIFIER_CHARS "/]*/" + glob;
        }
    } // namespace

    MeshesParticlesPath::MeshesParticlesPath(
        std::vector<std::string> const &meshes,
        std::vector<std::string> const &particles)
    {
        /*
         * /group/meshes/E is a mesh if the meshes path contains:
         *
         * 1) '/group/meshes/' (absolute path to mesh container)
         * 2) 'meshes/' (relative path to mesh container)
         *
         * All this analogously for particles path.
         */

        // regex for detecting option 1)
        // e.g. '/path/to/meshes/': The path to the meshes. Mandatory slashes at
        //                          beginning and end, possibly slashes in
        //                          between. Mandatory slash at beginning might
        //                          be replaced with '%%' to enable paths like
        //                          '%%/path/to/meshes'.
        // resolves to: `(/|%%)[[:alnum:]_%/]+/`
        std::regex is_legal_long_path_specification(
            "(/|" OPENPMD_DOUBLE_GLOBBING_CHAR
            ")[" OPENPMD_LEGAL_IDENTIFIER_CHARS OPENPMD_SINGLE_GLOBBING_CHAR
            "/]+/",
            regex_flags | std::regex_constants::optimize);

        // Regex for detecting option 2)
        // e.g. 'meshes/': The name without path. One single mandatory slash
        //                 at the end, no slashes otherwise.
        // resolves to `[[:alnum:]_]+/`
        std::regex is_legal_short_path_specification(
            "[" OPENPMD_LEGAL_IDENTIFIER_CHARS "]+/",
            regex_flags | std::regex_constants::optimize);

        for (auto [target_regex, vec] :
             {std::make_tuple(&this->meshRegex, &meshes),
              std::make_tuple(&this->particleRegex, &particles)})
        {
            std::stringstream build_regex;
            // neutral element: empty language, regex doesn't match anything
            build_regex << "(a^)";
            for (auto const &entry : *vec)
            {
                if (std::regex_match(entry, is_legal_short_path_specification))
                {
                    build_regex << "|(" << globToRegexShortForm(entry) << ')';
                }
                else if (std::regex_match(
                             entry, is_legal_long_path_specification))
                {
                    build_regex << "|(" << globToRegexLongForm(entry) << ')';
                }
                else
                {
                    std::cerr
                        << "[WARNING] Not a legal meshes-/particles-path: '"
                        << entry << "'. Will skip." << std::endl;
                }
            }
            auto regex_string = build_regex.str();
            // std::cout << "Using regex string: " << regex_string << std::endl;
            *target_regex = std::regex(
                regex_string, regex_flags | std::regex_constants::optimize);
        }
        setDefaultMeshesParticlesPath(meshes, particles, *this);
    }

    ContainedType MeshesParticlesPath::determineType(
        std::vector<std::string> const &path) const
    {
        if (isMeshContainer(path))
        {
            return ContainedType::Mesh;
        }
        else if (isParticleContainer(path))
        {
            return ContainedType::Particle;
        }
        else
        {
            return ContainedType::Group;
        }
    }

    bool MeshesParticlesPath::isParticleContainer(
        std::vector<std::string> const &path) const
    {
        return anyPathRegexMatches(particleRegex, path);
    }
    bool MeshesParticlesPath::isMeshContainer(
        std::vector<std::string> const &path) const
    {
        return anyPathRegexMatches(meshRegex, path);
    }

    CustomHierarchyData::CustomHierarchyData()
    {
        syncAttributables();
    }

    void CustomHierarchyData::syncAttributables()
    {
        /*
         * m_embeddeddatasets and its friends should point to the same instance
         * of Attributable.
         * Not strictly necessary to do this explicitly due to virtual
         * inheritance (all Attributable instances are the same anyway),
         * but let's be explicit about this.
         */
        for (auto p : std::initializer_list<AttributableData *>{
                 static_cast<ContainerData<CustomHierarchy> *>(this),
                 static_cast<ContainerData<RecordComponent> *>(this),
                 static_cast<ContainerData<Mesh> *>(this),
                 static_cast<ContainerData<ParticleSpecies> *>(this)})
        {
            p->asSharedPtrOfAttributable() = this->asSharedPtrOfAttributable();
        }
    }
} // namespace internal

// template
// class ConversibleContainer<CustomHierarchy>;

CustomHierarchy::CustomHierarchy() : ConversibleContainer(NoInit{})
{
    setData(std::make_shared<Data_t>());
}
CustomHierarchy::CustomHierarchy(NoInit) : ConversibleContainer(NoInit{})
{}

void CustomHierarchy::readNonscalarMesh(
    EraseStaleMeshes &map, std::string const &mesh_name)
{
    Parameter<Operation::OPEN_PATH> pOpen;
    Parameter<Operation::LIST_ATTS> aList;

    Mesh &m = map[mesh_name];

    pOpen.path = mesh_name;
    aList.attributes->clear();
    IOHandler()->enqueue(IOTask(&m, pOpen));
    IOHandler()->enqueue(IOTask(&m, aList));
    IOHandler()->flush(internal::defaultFlushParams);

    // Find constant scalar meshes. shape generally required for meshes,
    // shape also required for scalars.
    // https://github.com/openPMD/openPMD-standard/pull/289
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

void CustomHierarchy::readScalarMesh(
    EraseStaleMeshes &map, std::string const &mesh_name)
{
    Parameter<Operation::OPEN_PATH> pOpen;
    Parameter<Operation::LIST_PATHS> pList;

    Parameter<Operation::OPEN_DATASET> dOpen;
    Mesh &m = map[mesh_name];
    dOpen.name = mesh_name;
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

void CustomHierarchy::readParticleSpecies(
    EraseStaleParticles &map, std::string const &species_name)
{
    Parameter<Operation::OPEN_PATH> pOpen;
    Parameter<Operation::LIST_PATHS> pList;

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
        std::cerr << "Cannot read particle species with name '" << species_name
                  << "' and will skip it due to read error:\n"
                  << err.what() << std::endl;
        map.forget(species_name);
    }
}

void CustomHierarchy::read(internal::MeshesParticlesPath const &mpp)
{
    std::vector<std::string> currentPath;
    read(mpp, currentPath);
}

void CustomHierarchy::read(
    internal::MeshesParticlesPath const &mpp,
    std::vector<std::string> &currentPath)
{
    /*
     * Convention for CustomHierarchy::flush and CustomHierarchy::read:
     * Path is created/opened already at entry point of method, method needs
     * to create/open path for contained subpaths.
     */

    Parameter<Operation::LIST_PATHS> pList;
    IOHandler()->enqueue(IOTask(this, pList));

    Attributable::readAttributes(ReadMode::FullyReread);
    Parameter<Operation::LIST_DATASETS> dList;
    IOHandler()->enqueue(IOTask(this, dList));
    IOHandler()->flush(internal::defaultFlushParams);

    std::deque<std::string> constantComponentsPushback;
    auto &data = get();
    auto embeddedMeshes = data.embeddedMeshesWrapped();
    auto embeddedParticles = data.embeddedParticlesWrapped();
    EraseStaleMeshes meshesMap(embeddedMeshes);
    EraseStaleParticles particlesMap(embeddedParticles);
    for (auto const &path : *pList.paths)
    {
        switch (mpp.determineType(currentPath))
        {
        case internal::ContainedType::Group: {
            Parameter<Operation::OPEN_PATH> pOpen;
            pOpen.path = path;
            auto &subpath = this->operator[](path);
            IOHandler()->enqueue(IOTask(&subpath, pOpen));
            currentPath.emplace_back(path);
            try
            {
                subpath.read(mpp, currentPath);
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read subgroup '" << path << "' at path '"
                          << myPath().openPMDPath()
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;
                container().erase(path);
            }
            currentPath.pop_back();
            if (subpath.size() == 0 && subpath.containsAttribute("shape") &&
                subpath.containsAttribute("value"))
            {
                // This is not a group, but a constant record component
                // Writable::~Writable() will deal with removing this from the
                // backend again.
                constantComponentsPushback.push_back(path);
                container().erase(path);
            }
            break;
        }
        case internal::ContainedType::Mesh: {
            try
            {
                readNonscalarMesh(meshesMap, path);
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read mesh at location '"
                          << myPath().openPMDPath() << "/" << path
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;
                meshesMap.forget(path);
            }
            break;
        }
        case internal::ContainedType::Particle: {
            try
            {
                readParticleSpecies(particlesMap, path);
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read particle species at location '"
                          << myPath().openPMDPath() << "/" << path
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;
                particlesMap.forget(path);
            }
            break;
        }
        }
    }
    for (auto const &path : *dList.datasets)
    {
        switch (mpp.determineType(currentPath))
        {

        case internal::ContainedType::Particle:
            std::cerr << "[Warning] Dataset found at '"
                      << (concatWithSep(currentPath, "/") + "/" + path)
                      << "' inside the particles path. A particle species is "
                         "always a group, never a dataset. Will parse as a "
                         "custom dataset. Storing custom datasets inside the "
                         "particles path is discouraged."
                      << std::endl;
            [[fallthrough]];
        // Group is a bit of an internal misnomer here, it just means that
        // it matches neither meshes nor particles path
        case internal::ContainedType::Group: {
            auto embeddedDatasets = data.embeddedDatasetsWrapped();
            auto &rc = embeddedDatasets[path];
            Parameter<Operation::OPEN_DATASET> dOpen;
            dOpen.name = path;
            IOHandler()->enqueue(IOTask(&rc, dOpen));
            try
            {
                IOHandler()->flush(internal::defaultFlushParams);
                rc.setWritten(false, Attributable::EnqueueAsynchronously::No);
                rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
                rc.setWritten(true, Attributable::EnqueueAsynchronously::No);
                rc.read(/* read_defaults = */ false);
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read contained custom dataset '" << path
                          << "' at path '" << myPath().openPMDPath()
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;
                embeddedDatasets.erase(path);
            }
            break;
        }
        case internal::ContainedType::Mesh:
            try
            {
                readScalarMesh(meshesMap, path);
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read scalar mesh at location '"
                          << myPath().openPMDPath() << "/" << path
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;
                meshesMap.forget(path);
            }
            break;
        }
    }

    for (auto const &path : constantComponentsPushback)
    {
        auto embeddedDatasets = data.embeddedDatasetsWrapped();
        auto &rc = embeddedDatasets[path];
        try
        {
            Parameter<Operation::OPEN_PATH> pOpen;
            pOpen.path = path;
            IOHandler()->enqueue(IOTask(&rc, pOpen));
            rc.get().m_isConstant = true;
            rc.read(/* read_defaults = */ false);
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read dataset at location '"
                      << myPath().openPMDPath() << "/" << path
                      << "' and will skip it due to read error:\n"
                      << err.what() << std::endl;
            embeddedDatasets.erase(path);
        }
    }
    setDirty(false);
}

void CustomHierarchy::flush_internal(
    internal::FlushParams const &flushParams,
    internal::MeshesParticlesPath &mpp,
    std::vector<std::string> currentPath)
{
    if (!dirtyRecursive())
    {
        return;
    }
    /*
     * Convention for CustomHierarchy::flush and CustomHierarchy::read:
     * Path is created/opened already at entry point of method, method needs
     * to create/open path for contained subpaths.
     */

    // No need to do anything in access::readOnly since meshes and particles
    // are initialized as aliases for subgroups at parsing time
    auto &data = get();
    if (access::write(IOHandler()->m_frontendAccess))
    {
        flushAttributes(flushParams);
    }

    Parameter<Operation::CREATE_PATH> pCreate;
    for (auto &[name, subpath] : *this)
    {
        if (!subpath.written())
        {
            pCreate.path = name;
            IOHandler()->enqueue(IOTask(&subpath, pCreate));
        }
        currentPath.emplace_back(name);
        subpath.flush_internal(flushParams, mpp, currentPath);
        currentPath.pop_back();
    }
    for (auto &[name, mesh] : data.embeddedMeshesInternal())
    {
        if (!mpp.isMeshContainer(currentPath))
        {
            std::string extend_meshes_path;
            // Check if this can be covered by shorthand notation
            // (e.g. meshesPath == "meshes/")
            if (!currentPath.empty() &&
                *currentPath.rbegin() == mpp.m_defaultMeshesPath)
            {
                extend_meshes_path = *currentPath.rbegin() + "/";
            }
            else
            {
                // Otherwise use full path
                extend_meshes_path = "/" +
                    (currentPath.empty()
                         ? ""
                         : concatWithSep(currentPath, "/") + "/");
            }
            mpp.collectNewMeshesPaths.emplace(std::move(extend_meshes_path));
        }
        mesh.flush(name, flushParams);
    }
    for (auto &[name, particleSpecies] : data.embeddedParticlesInternal())
    {
        if (!mpp.isParticleContainer(currentPath))
        {
            std::string extend_particles_path;
            if (!currentPath.empty() &&
                *currentPath.rbegin() == mpp.m_defaultParticlesPath)
            {
                // Check if this can be covered by shorthand notation
                // (e.g. particlesPath == "particles/")
                extend_particles_path = *currentPath.rbegin() + "/";
            }
            else
            {
                // Otherwise use full path
                extend_particles_path = "/" +
                    (currentPath.empty()
                         ? ""
                         : concatWithSep(currentPath, "/") + "/");
                ;
            }
            mpp.collectNewParticlesPaths.emplace(
                std::move(extend_particles_path));
        }
        particleSpecies.flush(name, flushParams);
    }
    for (auto &[name, dataset] : get().embeddedDatasetsInternal())
    {
        dataset.flush(name, flushParams, /* set_defaults = */ false);
    }

    if (flushParams.flushLevel != FlushLevel::SkeletonOnly &&
        flushParams.flushLevel != FlushLevel::CreateOrOpenFiles)
    {
        setDirty(false);
    }
}

void CustomHierarchy::flush(
    std::string const & /* path */, internal::FlushParams const &)
{
    throw std::runtime_error(
        "[CustomHierarchy::flush()] Don't use this method. Flushing should be "
        "triggered via Iteration class.");
}

void CustomHierarchy::linkHierarchy(Writable &w)
{
    Attributable::linkHierarchy(w);
}
} // namespace openPMD

#undef OPENPMD_LEGAL_IDENTIFIER_CHARS
#undef OPENPMD_SINGLE_GLOBBING_CHAR
#undef OPENPMD_DOUBLE_GLOBBING_CHAR
#endif
