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

    void ElementAccessPolicy<CustomHierarchy>::call(CustomHierarchy &cont)
    {
        if (!cont.get().has_been_read &&
            access::read(cont.IOHandler()->m_backendAccess))
        {
            bool skip = false;
            cont.writable().objectType.ifGroup([&](auto &group_data) {
                if (group_data.phantom)
                {
                    skip = true;
                }
            });
            if (!skip)
            {
                cont.read(1);
            }
        }
    }

    void ElementAccessPolicy<CustomHierarchy>::call(CustomHierarchy const &)
    {}
} // namespace traits

// TODO: Add visitor over real type, using HierarchyVisitor class
//       Then, maybe even implement HierarchyVisitor in terms of CustomHierarchy

template <typename MappedType>
auto ConvertibleContainer<MappedType>::isDataset() const -> bool
{
    return this->writable().objectType.isDataset();
}

template <typename MappedType>
auto ConvertibleContainer<MappedType>::asDataset() -> CustomDataset
{
    auto castableToConstantDataset = [&]() {
        return this->containsAttribute("value") &&
            this->containsAttribute("shape") && this->empty();
    };
    auto initDataset = [&]() { this->writable().objectType.initDataset(); };
    auto makeResult = [&]() {
        CustomDataset res;
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
        else
        {
            return makeResult();
        }
    }
    else
    {
        if (this->writable().objectType.isGroup() && !this->empty())
        {
            throw error::WrongAPIUsage(
                "Can't cast a group that has contained objects into a "
                "dataset.");
        }
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

template <>
auto ConvertibleContainer<CustomHierarchy>::datasets()
    -> ConvertibleContainer<CustomDataset>
{
    ConvertibleContainer<CustomDataset> res;
    res.m_attri->cloneFrom(*this->m_attri);
    res.m_child_filter.kind = internal::ChildFilter::Kind::Datasets;
    return res;
}

template <>
auto ConvertibleContainer<CustomDataset>::datasets()
    -> ConvertibleContainer<CustomDataset>
{
    return *this;
}

template <typename MappedType>
auto ConvertibleContainer<MappedType>::subgroups()
    -> ConvertibleContainer<CustomHierarchy>
{
    ConvertibleContainer<CustomHierarchy> res;
    res.m_attri->cloneFrom(*this->m_attri);
    res.m_child_filter.kind = internal::ChildFilter::Kind::Groups;
    return res;
}

template class ConvertibleContainer<CustomHierarchy>;
template class ConvertibleContainer<CustomDataset>;

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

auto CustomHierarchy::read(size_t const max_recursion_depth)
    -> CustomHierarchy &
{
    auxiliary::opaque_defer_type reset_parsing_status;
    if (IOHandler()->m_seriesStatus != internal::SeriesStatus::Parsing)
    {
        IOHandler()->m_seriesStatus = internal::SeriesStatus::Parsing;
        reset_parsing_status = auxiliary::defer([&]() {
            IOHandler()->m_seriesStatus = internal::SeriesStatus::Default;
        });
    }
    auto was_written = writable().written;
    if (!was_written)
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

    auto &data = get();
    auto defer_bookkeeping = auxiliary::defer([&]() {
        data.has_been_read = true;
        setDirty(false);
    });

    if (writable().objectType.isDataset())
    {
        {
            asDataset().read();
        }
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

    if (!data.has_been_read)
    {
        Parameter<Operation::LIST_PATHS> pList;
        IOHandler()->enqueue(IOTask(this, pList));

        Parameter<Operation::LIST_DATASETS> dList;
        IOHandler()->enqueue(IOTask(this, dList));

        IOHandler()->flush(internal::defaultFlushParams);

        for (auto const &path : *pList.paths)
        {
            auto &subpath = this->container().get_or_create(
                path, /* access_policy = */ false);
            subpath.linkHierarchy(this->writable());
        }

        for (auto const &path : *dList.datasets)
        {
            auto &subpath = this->container().get_or_create(
                path, /* access_policy = */ false);
            subpath.linkHierarchy(this->writable());

            subpath.asDataset();
            do_recurse(subpath);
        }
    }

    auto view = this->container();
    view.set_access_policy(false);
    for (auto &pair : view)
    {
        auto &path = pair.first;
        auto &subpath = pair.second;
        if (subpath.get().has_been_read)
        {
            continue;
        }

        std::visit(
            auxiliary::overloaded{
                [&](internal::object_type::GroupMetaData &group_data) {
                    if (group_data.phantom)
                    {
                        return;
                    }
                    if (!subpath.written())
                    {
                        Parameter<Operation::OPEN_PATH> pOpen;
                        pOpen.path = path;
                        subpath.linkHierarchy(this->writable());
                        IOHandler()->enqueue(IOTask(&subpath, pOpen));
                    }
                    do_recurse(subpath);
                },
                [&](internal::object_type::DatasetMetaData &) {
                    if (!subpath.written())
                    {
                        Parameter<Operation::OPEN_DATASET> dOpen;
                        dOpen.name = path;

                        IOHandler()->enqueue(IOTask(&subpath, dOpen));
                        IOHandler()->flush(internal::defaultFlushParams);

                        subpath.setWritten(
                            false, Attributable::EnqueueAsynchronously::No);
                        subpath.asDataset().resetDataset(
                            Dataset(*dOpen.dtype, *dOpen.extent));
                        subpath.setWritten(
                            true, Attributable::EnqueueAsynchronously::No);
                    }
                    do_recurse(subpath);
                }},
            subpath.writable().objectType.as_base());
    }

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
    if (!writable().objectType.isGroup())
    {
        return;
    }
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
    /*
     * The frontend container of a CustomHierarchy mirrors the backend group's
     * children, which is a superset of the objects flagged as
     * managed_as_custom_hierarchy: The flag marks only those objects that were
     * *created* by a CustomHierarchy view, while the frontend container
     * additionally aliases objects managed by the regular openPMD hierarchy
     * (e.g. Meshes, Particles and their descendants).
     *
     * Attributable::visitHierarchy() recurses into the custom-managed children
     * after calling this method. To visit every object exactly once, only
     * traverse those frontend children that are *not* marked as custom-managed
     * here, and leave the rest to the base class.
     */
    if (recursive)
    {
        writable().objectType.ifGroup([&](auto &) {
            for (auto &pair : *this)
            {
                if (!pair.second.Attributable::get()
                         .managed_as_custom_hierarchy)
                {
                    pair.second.visitHierarchy(v, recursive);
                }
            }
        });
    }
    v(*this);
}
} // namespace openPMD
