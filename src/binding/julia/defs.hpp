/* General definitions
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#ifndef DEFS_HPP
#define DEFS_HPP

#include "openPMD/openPMD.hpp"

#include <jlcxx/array.hpp>
#include <jlcxx/jlcxx.hpp>
#include <jlcxx/module.hpp>
#include <jlcxx/stl.hpp>
#include <jlcxx/tuple.hpp>
#include <jlcxx/type_conversion.hpp>

#include <algorithm>
#include <array>
#include <complex>
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using namespace openPMD;

template <std::size_t I>
struct sized_uint;
template <>
struct sized_uint<1>
{
    using type = std::uint8_t;
};
template <>
struct sized_uint<2>
{
    using type = std::uint16_t;
};
template <>
struct sized_uint<4>
{
    using type = std::uint32_t;
};
template <>
struct sized_uint<8>
{
    using type = std::uint64_t;
};
template <std::size_t I>
using sized_uint_t = typename sized_uint<I>::type;

using array_double_7 = std::array<double, 7>;

// From pybind11, see
// share/openPMD/thirdParty/pybind11/include/pybind11/detail/common.h
// in the source tree
struct non_const_tag
{};
struct const_tag
{};

namespace detail
{
template <typename... Args>
struct overload_cast_impl
{
    template <typename Return>
    constexpr auto operator()(Return (*pf)(Args...)) const noexcept
        -> decltype(pf)
    {
        return pf;
    }

    template <typename Return, typename Class>
    constexpr auto
    operator()(Return (Class::*pmf)(Args...), non_const_tag = {}) const noexcept
        -> decltype(pmf)
    {
        return pmf;
    }

    template <typename Return, typename Class>
    constexpr auto
    operator()(Return (Class::*pmf)(Args...) const, const_tag) const noexcept
        -> decltype(pmf)
    {
        return pmf;
    }
};
} // namespace detail

template <typename... Args>
static constexpr ::detail::overload_cast_impl<Args...> overload_cast{};
constexpr const_tag const const_;

/**
 * Generalizes the repeated application of a function template for all
 * scalar openPMD datatypes.
 *
 * Will call the function template found at Action::template call<T>(),
 * instantiating T with every scalar openPMD datatype as found
 * in the Datatype enum.
 *
 * @tparam Action The struct containing the function template.
 * @tparam Args The function template's argument types.
 * @param args The function template's arguments.
 */
template <typename Action, typename... Args>
void forallScalarJuliaTypes(Args &&...args)
{
    // Do NOT call std::forward<Args>(args)... here
    // Move semantics must be avoided due to repeated application
    Action::template call<char>(args...);
    Action::template call<unsigned char>(args...);
    Action::template call<signed char>(args...);
    Action::template call<short>(args...);
    Action::template call<int>(args...);
    Action::template call<long>(args...);
    Action::template call<long long>(args...);
    Action::template call<unsigned short>(args...);
    Action::template call<unsigned int>(args...);
    Action::template call<unsigned long>(args...);
    Action::template call<unsigned long long>(args...);
    Action::template call<float>(args...);
    Action::template call<double>(args...);
    // We disable `long double` since Julia does not support this type
    // Action::template call<long double>(args...);
    Action::template call<std::complex<float>>(args...);
    Action::template call<std::complex<double>>(args...);
    // Action::template call<std::complex<long double>>(args...);
    Action::template call<std::string>(args...);
    Action::template call<bool>(args...);
}
/**
 * Generalizes the repeated application of a function template for all
 * openPMD datatypes.
 *
 * Will call the function template found at Action::template call<T>(),
 * instantiating T with every scalar datatype as found in the Datatype enum.
 *
 * @tparam Action The struct containing the function template.
 * @tparam Args The function template's argument types.
 * @param args The function template's arguments.
 */
template <typename Action, typename... Args>
void forallJuliaTypes(Args &&...args)
{
    // Do NOT call std::forward<Args>(args)... here
    // Move semantics must be avoided due to repeated application
    Action::template call<char>(args...);
    Action::template call<unsigned char>(args...);
    Action::template call<signed char>(args...);
    Action::template call<short>(args...);
    Action::template call<int>(args...);
    Action::template call<long>(args...);
    Action::template call<long long>(args...);
    Action::template call<unsigned short>(args...);
    Action::template call<unsigned int>(args...);
    Action::template call<unsigned long>(args...);
    Action::template call<unsigned long long>(args...);
    Action::template call<float>(args...);
    Action::template call<double>(args...);
    // We disable `long double` since Julia does not support this type
    // Action::template call<long double>(args...);
    Action::template call<std::complex<float>>(args...);
    Action::template call<std::complex<double>>(args...);
    // Action::template call<std::complex<long double>>(args...);
    Action::template call<std::string>(args...);
    Action::template call<std::vector<char>>(args...);
    Action::template call<std::vector<short>>(args...);
    Action::template call<std::vector<int>>(args...);
    Action::template call<std::vector<long>>(args...);
    Action::template call<std::vector<long long>>(args...);
    Action::template call<std::vector<unsigned char>>(args...);
    Action::template call<std::vector<signed char>>(args...);
    Action::template call<std::vector<unsigned short>>(args...);
    Action::template call<std::vector<unsigned int>>(args...);
    Action::template call<std::vector<unsigned long>>(args...);
    Action::template call<std::vector<unsigned long long>>(args...);
    Action::template call<std::vector<float>>(args...);
    Action::template call<std::vector<double>>(args...);
    // Action::template call<std::vector<long double>>(args...);
    Action::template call<std::vector<std::complex<float>>>(args...);
    Action::template call<std::vector<std::complex<double>>>(args...);
    // Action::template call<std::vector<std::complex<long double>>>(args...);
    Action::template call<std::vector<std::string>>(args...);
    Action::template call<std::array<double, 7>>(args...);
    Action::template call<bool>(args...);
}

namespace
{
template <typename T, typename U>
std::vector<std::pair<T, U>> map_to_vector_pair(const std::map<T, U> &m)
{
    std::vector<std::pair<std::string, bool>> vp;
    vp.reserve(m.size());
    for (const auto &p : m)
        vp.push_back(p);
    return vp;
}

template <typename T, typename U>
std::vector<std::tuple<T, U>> map_to_vector_tuple(const std::map<T, U> &m)
{
    std::vector<std::tuple<std::string, bool>> vp;
    vp.reserve(m.size());
    for (const auto &p : m)
        vp.emplace_back(p.first, p.second);
    return vp;
}

template <typename T>
std::shared_ptr<T> create_aliasing_shared_ptr(T *ptr)
{
    auto null_deleter = [](T *) {};
    return std::shared_ptr<T>(ptr, null_deleter);
}

template <typename T>
std::shared_ptr<T> capture_vector_as_buffer(std::vector<T> &vec)
{
    if constexpr (std::is_same_v<T, bool>)
    {
        // We cannot handle std::vector<bool> because it is special
        std::abort();
    }
    else
    {
        auto deleter = [](T *) { /* do not delete anything */ };
        std::shared_ptr<T> ptr(vec.data(), std::move(deleter));
        return ptr;
    }
}

template <typename T>
std::shared_ptr<T> capture_vector(std::vector<T> vec)
{
    if constexpr (std::is_same_v<T, bool>)
    {
        // Copy the vector, because std::vector<bool> is special
        T *dataptr = new T[vec.size()];
        std::shared_ptr<T> ptr(dataptr, std::default_delete<T[]>());
        std::copy(vec.begin(), vec.end(), dataptr);
        return ptr;
    }
    else
    {
        // Capture the vector
        T *dataptr = vec.data();
        auto deleter = [vec = std::move(vec)](T *) {
            // We moved the vector into the anonymous function, and thus it will
            // be destructed when the anonymous function is destructed. There is
            // no need to call a destructor manually.
        };
        std::shared_ptr<T> ptr(dataptr, std::move(deleter));
        return ptr;
    }
}

template <typename T, std::size_t N>
void add_array_type(jlcxx::Module &mod, const std::string &name)
{
    mod.add_type<std::array<T, N>>(name)
        .template constructor<>()
        .template constructor<const std::array<T, N> &>()
        .method("size1", &std::array<T, N>::size)
        .method("getindex1", [](const std::array<T, N> &a, std::size_t n) {
            return a[n];
        });
    jlcxx::stl::apply_stl<std::array<T, N>>(mod);
}

template <typename T, std::size_t N>
void map_array_type(jlcxx::Module &mod, const std::string &name)
{
    mod.map_type<std::array<T, N>>(name);
    mod.method("size1", [](const std::array<T, N> &a) { return a.size(); });
    mod.method("getindex1", [](const std::array<T, N> &a, std::size_t n) {
        return a[n];
    });
    jlcxx::stl::apply_stl<std::array<T, N>>(mod);
}

template <typename T, typename U>
void add_pair_type(jlcxx::Module &mod, const std::string &name)
{
    mod.add_type<std::pair<T, U>>(name)
        .template constructor<>()
        .template constructor<const std::pair<T, U> &>()
        .method("first", [](const std::pair<T, U> &p) { return p.first; })
        .method("second", [](const std::pair<T, U> &p) { return p.second; });
    jlcxx::stl::apply_stl<std::pair<T, U>>(mod);
}

} // namespace

namespace jlcxx
{
template <>
struct IsMirroredType<array_double_7> : std::false_type
{};
} // namespace jlcxx

// We use one function per header file
void define_julia_Access(jlcxx::Module &mod);
void define_julia_Attributable(jlcxx::Module &mod);
void define_julia_Attribute(jlcxx::Module &mod);
void define_julia_BaseRecordComponent(jlcxx::Module &mod);
void define_julia_ChunkInfo(jlcxx::Module &mod);
template <typename Eltype, typename Keytype = std::string>
void define_julia_Container(jlcxx::Module &mod);
void define_julia_Dataset(jlcxx::Module &mod);
void define_julia_Datatype(jlcxx::Module &mod);
void define_julia_Format(jlcxx::Module &mod);
void define_julia_Iteration(jlcxx::Module &mod);
void define_julia_Mesh(jlcxx::Module &mod);
void define_julia_MeshRecordComponent(jlcxx::Module &mod);
void define_julia_RecordComponent(jlcxx::Module &mod);
void define_julia_RecordComponent_load_chunk(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type);
void define_julia_RecordComponent_make_constant(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type);
void define_julia_RecordComponent_store_chunk(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type);
void define_julia_Series(jlcxx::Module &mod);
void define_julia_UnitDimension(jlcxx::Module &mod);
void define_julia_ReadIterations(jlcxx::Module &mod);
void define_julia_WriteIterations(jlcxx::Module &mod);
void define_julia_shared_ptr(jlcxx::Module &mod);
void define_julia_version(jlcxx::Module &mod);

#endif // #ifndef DEFS_HPP
