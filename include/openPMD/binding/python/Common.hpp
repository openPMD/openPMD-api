/* Copyright 2023-2025 The openPMD Community, Franz Poeschel
 *
 * This header is used to centrally define classes that shall not violate the
 * C++ one-definition-rule (ODR) for various Python translation units.
 *
 * Authors: Axel Huebl
 * License: LGPL-3.0-or-later
 */
#pragma once

#include "openPMD/ChunkInfo.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/bind_map.h>
#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/complex.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#if openPMD_USE_FILESYSTEM_HEADER
#include <nanobind/stl/filesystem.h>
#endif
// not yet used:
//   nanobind/stl/function.h  // for std::function

#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

using PyVecChunkInfo = std::vector<openPMD::ChunkInfo>;

NB_MAKE_OPAQUE(PyVecChunkInfo)
NB_MAKE_OPAQUE(openPMD::ChunkTable)
NB_MAKE_OPAQUE(openPMD::chunk_assignment::Assignment)

// used exclusively in all our Python .cpp files
namespace nb = nanobind;
using namespace openPMD;

// opaque types
using PyIterationContainer = Series::IterationsContainer_t;
using PyMeshContainer = Container<Mesh>;
using PyPartContainer = Container<ParticleSpecies>;
using PyPatchContainer = Container<ParticlePatches>;
using PyRecordContainer = Container<Record>;
using PyPatchRecordContainer = Container<PatchRecord>;
using PyRecordComponentContainer = Container<RecordComponent>;
using PyMeshRecordComponentContainer = Container<MeshRecordComponent>;
using PyPatchRecordComponentContainer = Container<PatchRecordComponent>;
using PyBaseRecordRecordComponent = BaseRecord<RecordComponent>;
using PyBaseRecordMeshRecordComponent = BaseRecord<MeshRecordComponent>;
using PyBaseRecordPatchRecordComponent = BaseRecord<PatchRecordComponent>;

/*
 * Inheritance in the openPMD object model vs. nanobind
 * ====================================================
 *
 * nanobind supports single inheritance only and, when passing an instance of
 * a derived class to a function expecting a base class, it reinterprets the
 * instance pointer without adjusting it. This is only valid if the base class
 * subobject is located at offset zero.
 * In the openPMD object model, this is not the case:
 *
 * - Attributable is a virtual base class of Container<T> and
 *   BaseRecordComponent.
 * - BaseRecord<T> derives from Container<T> (primary base) and from the
 *   record component type T (secondary base).
 *
 * To deal with this, we do the following:
 *
 * 1. Every Python class for a type derived from Attributable is created with
 *    bind_class<>() below, which registers an upcast function for the type.
 * 2. Methods bound on Attributable and on the record component classes accept
 *    `self` (and other arguments of these types) as Self<T>, which converts
 *    from any registered type with correct pointer adjustment.
 * 3. When returning a reference/pointer to one of these types, the type_hook
 *    specialization below makes nanobind create an object of the dynamic
 *    type only if that requires no pointer adjustment.
 * 4. BaseRecord<T> is bound with Container<T> as its Python base, and the
 *    methods of T are attached via inherit_attributes().
 */
namespace openPMD::python
{
using UpcastToAttributable = Attributable *(*)(void *);

inline auto upcastRegistry()
    -> std::unordered_map<std::type_index, UpcastToAttributable> &
{
    static std::unordered_map<std::type_index, UpcastToAttributable> registry;
    return registry;
}

template <typename T>
void registerAttributableType()
{
    upcastRegistry()[std::type_index(typeid(T))] =
        [](void *ptr) -> Attributable * { return static_cast<T *>(ptr); };
}

/** Get a pointer to the C++ object of type T held by a Python object.
 *
 * Returns nullptr if the Python object does not hold a (fully constructed)
 * object of a registered type derived from T.
 */
template <typename T>
T *upcast(nb::handle h) noexcept
{
    if (!nb::inst_check(h) || !nb::inst_ready(h))
    {
        return nullptr;
    }
    std::type_info const &type = nb::type_info(h.type());
    void *ptr = nb::inst_ptr<void>(h);
    if (type == typeid(T))
    {
        return static_cast<T *>(ptr);
    }
    auto &registry = upcastRegistry();
    auto it = registry.find(std::type_index(type));
    if (it == registry.end())
    {
        return nullptr;
    }
    return dynamic_cast<T *>(it->second(ptr));
}

/** Function argument type for an Attributable-derived object T
 *
 * Use instead of T & in bound functions, see above.
 */
template <typename T>
struct Self
{
    T *ptr = nullptr;
    /** The Python object */
    nb::handle handle;

    T &operator*() const
    {
        return *ptr;
    }
    T *operator->() const
    {
        return ptr;
    }
};

namespace detail
{
    template <typename R>
    constexpr bool returns_attributable_ref_v = std::is_lvalue_reference_v<R> &&
        std::is_base_of_v<Attributable, std::remove_reference_t<R>>;

    template <typename T, typename R, typename F, typename... Args>
    auto make_self_method(F f)
    {
        if constexpr (returns_attributable_ref_v<R>)
        {
            return [f](Self<T> self, Args... args) -> nb::object {
                R res = ((*self).*f)(std::forward<Args>(args)...);
                // Methods returning *this (fluent API): return the Python
                // object itself, keeping its type (e.g. Mesh for a
                // MeshRecordComponent method called on a scalar Mesh).
                if (dynamic_cast<void const *>(&res) ==
                    dynamic_cast<void const *>(self.ptr))
                {
                    return nb::borrow(self.handle);
                }
                return nb::cast(res, nb::rv_policy::copy);
            };
        }
        else
        {
            return [f](Self<T> self, Args... args) -> R {
                return ((*self).*f)(std::forward<Args>(args)...);
            };
        }
    }
} // namespace detail

/** Wrap a member function of T such that `self` is taken as Self<T> */
template <typename T = void, typename R, typename C, typename... Args>
auto self_method(R (C::*f)(Args...))
{
    using T_ = std::conditional_t<std::is_void_v<T>, C, T>;
    return detail::make_self_method<T_, R, decltype(f), Args...>(f);
}

template <typename T = void, typename R, typename C, typename... Args>
auto self_method(R (C::*f)(Args...) const)
{
    using T_ = std::conditional_t<std::is_void_v<T>, C, T>;
    return detail::make_self_method<T_, R, decltype(f), Args...>(f);
}

/** Make enum entries with the same value aliases of the first one
 *
 * Python enums (and pybind11) treat entries with the same value as aliases,
 * nanobind creates distinct members that would compare unequal.
 */
inline void make_enum_aliases(nb::handle enum_type)
{
    nb::dict first_by_value;
    nb::dict member_map = nb::borrow<nb::dict>(enum_type.attr("_member_map_"));
    for (auto [name, member] : member_map)
    {
        nb::object value = member.attr("value");
        if (!first_by_value.contains(value))
        {
            first_by_value[value] = member;
            continue;
        }
        nb::object first = first_by_value[value];
        member_map[name] = first;
        // enum.EnumType.__setattr__ forbids reassigning members
        nb::handle(reinterpret_cast<PyObject *>(&PyType_Type))
            .attr("__setattr__")(enum_type, name, first);
    }
}

/** Create a nanobind class, registering it for Self<> if needed */
template <typename T, typename... Ts, typename... Extra>
nb::class_<T, Ts...>
bind_class(nb::handle scope, char const *name, Extra const &...extra)
{
    static_assert(
        sizeof...(Ts) <= 1, "nanobind supports a single base class only.");
    if constexpr (std::is_base_of_v<Attributable, T>)
    {
        registerAttributableType<T>();
    }
    return nb::class_<T, Ts...>(scope, name, extra...);
}

/** Attach the attributes from the MRO of class `from` to class `cls`
 *
 * Emulates (C++ and pybind11) multiple inheritance:
 * Attributes found in the MRO of `from` (stopping at class `stop`) are
 * attached to `cls` unless already present there. Special methods are
 * skipped, these must be defined explicitly.
 * The attached methods must accept `self` as Self<T>.
 */
inline void inherit_attributes(nb::handle cls, nb::handle from, nb::handle stop)
{
    for (nb::handle base : from.attr("__mro__"))
    {
        if (base.is(stop))
        {
            break;
        }
        for (nb::handle item : base.attr("__dict__").attr("items")())
        {
            nb::handle name = item[0];
            auto name_str = nb::cast<std::string>(name);
            if (name_str.size() >= 2 && name_str.substr(0, 2) == "__")
            {
                continue;
            }
            if (!nb::hasattr(cls, name))
            {
                nb::setattr(cls, name, item[1]);
            }
        }
    }
}
} // namespace openPMD::python

namespace nanobind::detail
{
template <typename T>
struct type_caster<openPMD::python::Self<T>>
{
    NB_TYPE_CASTER(openPMD::python::Self<T>, make_caster<T>::Name)

    bool from_python(handle src, uint32_t, cleanup_list *) noexcept
    {
        value.ptr = openPMD::python::upcast<T>(src);
        value.handle = src;
        return value.ptr != nullptr;
    }

    static handle
    from_cpp(openPMD::python::Self<T> const &, rv_policy, cleanup_list *)
    {
        return handle();
    }
};

/*
 * When returning a T & or T *, nanobind looks up the dynamic type of the
 * object and reinterprets the pointer as a pointer to that type.
 * Allow this only when no pointer adjustment is necessary and the dynamic
 * type is bound, otherwise fall back to the static type.
 */
template <typename T>
struct type_hook<T, enable_if_t<std::is_base_of_v<openPMD::Attributable, T>>>
{
    static std::type_info const *get(T *ptr)
    {
        if (ptr &&
            dynamic_cast<void const *>(ptr) == static_cast<void const *>(ptr) &&
            NB_CALL(nb_type_lookup)(NB_CTX, &typeid(*ptr)))
        {
            return &typeid(*ptr);
        }
        return &typeid(T);
    }
};
} // namespace nanobind::detail
