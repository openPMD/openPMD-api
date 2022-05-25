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

#include "openPMD/Datatype.hpp"
#include "openPMD/auxiliary/Variant.hpp"

#include <algorithm>
#include <array>
#include <complex>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace openPMD
{
// TODO This might have to be a Writable
// Reasoning - Flushes are expected to be done often.
// Attributes should not be written unless dirty.
// At the moment the dirty check is done at Attributable level,
// resulting in all of an Attributables Attributes being written to disk even if
// only one changes
/** Varidic datatype supporting at least all formats for attributes specified in
 * the openPMD standard.
 *
 * @note Extending and/or modifying the available formats requires identical
 *       modifications to Datatype.
 */
class Attribute
    : public auxiliary::Variant<
          Datatype,
          char,
          unsigned char, // signed char,
          short,
          int,
          long,
          long long,
          unsigned short,
          unsigned int,
          unsigned long,
          unsigned long long,
          float,
          double,
          long double,
          std::complex<float>,
          std::complex<double>,
          std::complex<long double>,
          std::string,
          std::vector<char>,
          std::vector<short>,
          std::vector<int>,
          std::vector<long>,
          std::vector<long long>,
          std::vector<unsigned char>,
          std::vector<unsigned short>,
          std::vector<unsigned int>,
          std::vector<unsigned long>,
          std::vector<unsigned long long>,
          std::vector<float>,
          std::vector<double>,
          std::vector<long double>,
          std::vector<std::complex<float> >,
          std::vector<std::complex<double> >,
          std::vector<std::complex<long double> >,
          std::vector<std::string>,
          std::array<double, 7>,
          bool>
{
public:
    Attribute(resource r) : Variant(std::move(r))
    {}

    /** Retrieve a stored specific Attribute and cast if convertible.
     *
     * @note This performs a static_cast and might introduce precision loss if
     *       requested. Check dtype explicitly beforehand if needed.
     *
     * @throw   std::runtime_error if stored object is not static castable to U.
     * @tparam  U   Type of the object to be casted to.
     * @return  Copy of the retrieved object, casted to type U.
     */
    template <typename U>
    U get() const;
};

template <
    typename T,
    typename U,
    bool isConvertible = std::is_convertible<T, U>::value>
struct DoConvert;

template <typename T, typename U>
struct DoConvert<T, U, false>
{
    U operator()(T *)
    {
        throw std::runtime_error("getCast: no cast possible.");
    }
};

template <typename T, typename U>
struct DoConvert<T, U, true>
{
    U operator()(T *pv)
    {
        return static_cast<U>(*pv);
    }
};

template <typename T, typename U>
struct DoConvert<std::vector<T>, std::vector<U>, false>
{
    static constexpr bool convertible = std::is_convertible<T, U>::value;

    template <typename UU = U>
    auto operator()(std::vector<T> const *pv) ->
        typename std::enable_if<convertible, std::vector<UU> >::type
    {
        std::vector<U> u;
        u.reserve(pv->size());
        std::copy(pv->begin(), pv->end(), std::back_inserter(u));
        return u;
    }

    template <typename UU = U>
    auto operator()(std::vector<T> const *) ->
        typename std::enable_if<!convertible, std::vector<UU> >::type
    {
        throw std::runtime_error("getCast: no vector cast possible.");
    }
};

// conversion cast: turn a single value into a 1-element vector
template <typename T, typename U>
struct DoConvert<T, std::vector<U>, false>
{
    static constexpr bool convertible = std::is_convertible<T, U>::value;

    template <typename UU = U>
    auto operator()(T const *pv) ->
        typename std::enable_if<convertible, std::vector<UU> >::type
    {
        std::vector<U> u;
        u.reserve(1);
        u.push_back(static_cast<U>(*pv));
        return u;
    }

    template <typename UU = U>
    auto operator()(T const *) ->
        typename std::enable_if<!convertible, std::vector<UU> >::type
    {
        throw std::runtime_error(
            "getCast: no scalar to vector conversion possible.");
    }
};

// conversion cast: array to vector
// if a backend reports a std::array<> for something where the frontend expects
// a vector
template <typename T, typename U, size_t n>
struct DoConvert<std::array<T, n>, std::vector<U>, false>
{
    static constexpr bool convertible = std::is_convertible<T, U>::value;

    template <typename UU = U>
    auto operator()(std::array<T, n> const *pv) ->
        typename std::enable_if<convertible, std::vector<UU> >::type
    {
        std::vector<U> u;
        u.reserve(n);
        std::copy(pv->begin(), pv->end(), std::back_inserter(u));
        return u;
    }

    template <typename UU = U>
    auto operator()(std::array<T, n> const *) ->
        typename std::enable_if<!convertible, std::vector<UU> >::type
    {
        throw std::runtime_error(
            "getCast: no array to vector conversion possible.");
    }
};

// conversion cast: vector to array
// if a backend reports a std::vector<> for something where the frontend expects
// an array
template <typename T, typename U, size_t n>
struct DoConvert<std::vector<T>, std::array<U, n>, false>
{
    static constexpr bool convertible = std::is_convertible<T, U>::value;

    template <typename UU = U>
    auto operator()(std::vector<T> const *pv) ->
        typename std::enable_if<convertible, std::array<UU, n> >::type
    {
        std::array<U, n> u;
        if (n != pv->size())
        {
            throw std::runtime_error(
                "getCast: no vector to array conversion possible "
                "(wrong requested array size).");
        }
        for (size_t i = 0; i < n; ++i)
        {
            u[i] = static_cast<U>((*pv)[i]);
        }
        return u;
    }

    template <typename UU = U>
    auto operator()(std::vector<T> const *) ->
        typename std::enable_if<!convertible, std::array<UU, n> >::type
    {
        throw std::runtime_error(
            "getCast: no vector to array conversion possible.");
    }
};

/** Retrieve a stored specific Attribute and cast if convertible.
 *
 * @throw   std::runtime_error if stored object is not static castable to U.
 * @tparam  U   Type of the object to be casted to.
 * @return  Copy of the retrieved object, casted to type U.
 */
template <typename U>
inline U getCast(Attribute const &a)
{
    auto v = a.getResource();

    // icpc 2021.3.0 does not like variantSrc::visit (with mpark-variant)
    // we use variantSrc::visit for the other compilers to avoid having an
    // endless list of if-then-else
    // also, once we switch to C++17, we might throw this out in
    // favor of a hopefully working std::visit
#if defined(__ICC) || defined(__INTEL_COMPILER)
    if (auto pvalue_c = variantSrc::get_if<char>(&v))
        return DoConvert<char, U>{}(pvalue_c);
    else if (auto pvalue_uc = variantSrc::get_if<unsigned char>(&v))
        return DoConvert<unsigned char, U>{}(pvalue_uc);
    else if (auto pvalue_s = variantSrc::get_if<short>(&v))
        return DoConvert<short, U>{}(pvalue_s);
    else if (auto pvalue_i = variantSrc::get_if<int>(&v))
        return DoConvert<int, U>{}(pvalue_i);
    else if (auto pvalue_l = variantSrc::get_if<long>(&v))
        return DoConvert<long, U>{}(pvalue_l);
    else if (auto pvalue_ll = variantSrc::get_if<long long>(&v))
        return DoConvert<long long, U>{}(pvalue_ll);
    else if (auto pvalue_us = variantSrc::get_if<unsigned short>(&v))
        return DoConvert<unsigned short, U>{}(pvalue_us);
    else if (auto pvalue_ui = variantSrc::get_if<unsigned int>(&v))
        return DoConvert<unsigned int, U>{}(pvalue_ui);
    else if (auto pvalue_ul = variantSrc::get_if<unsigned long>(&v))
        return DoConvert<unsigned long, U>{}(pvalue_ul);
    else if (auto pvalue_ull = variantSrc::get_if<unsigned long long>(&v))
        return DoConvert<unsigned long long, U>{}(pvalue_ull);
    else if (auto pvalue_f = variantSrc::get_if<float>(&v))
        return DoConvert<float, U>{}(pvalue_f);
    else if (auto pvalue_d = variantSrc::get_if<double>(&v))
        return DoConvert<double, U>{}(pvalue_d);
    else if (auto pvalue_ld = variantSrc::get_if<long double>(&v))
        return DoConvert<long double, U>{}(pvalue_ld);
    else if (auto pvalue_cf = variantSrc::get_if<std::complex<float> >(&v))
        return DoConvert<std::complex<float>, U>{}(pvalue_cf);
    else if (auto pvalue_cd = variantSrc::get_if<std::complex<double> >(&v))
        return DoConvert<std::complex<double>, U>{}(pvalue_cd);
    else if (
        auto pvalue_cld = variantSrc::get_if<std::complex<long double> >(&v))
        return DoConvert<std::complex<long double>, U>{}(pvalue_cld);
    else if (auto pvalue_str = variantSrc::get_if<std::string>(&v))
        return DoConvert<std::string, U>{}(pvalue_str);
    // vector
    else if (auto pvalue_vc = variantSrc::get_if<std::vector<char> >(&v))
        return DoConvert<std::vector<char>, U>{}(pvalue_vc);
    else if (
        auto pvalue_vuc = variantSrc::get_if<std::vector<unsigned char> >(&v))
        return DoConvert<std::vector<unsigned char>, U>{}(pvalue_vuc);
    else if (auto pvalue_vs = variantSrc::get_if<std::vector<short> >(&v))
        return DoConvert<std::vector<short>, U>{}(pvalue_vs);
    else if (auto pvalue_vi = variantSrc::get_if<std::vector<int> >(&v))
        return DoConvert<std::vector<int>, U>{}(pvalue_vi);
    else if (auto pvalue_vl = variantSrc::get_if<std::vector<long> >(&v))
        return DoConvert<std::vector<long>, U>{}(pvalue_vl);
    else if (auto pvalue_vll = variantSrc::get_if<std::vector<long long> >(&v))
        return DoConvert<std::vector<long long>, U>{}(pvalue_vll);
    else if (
        auto pvalue_vus = variantSrc::get_if<std::vector<unsigned short> >(&v))
        return DoConvert<std::vector<unsigned short>, U>{}(pvalue_vus);
    else if (
        auto pvalue_vui = variantSrc::get_if<std::vector<unsigned int> >(&v))
        return DoConvert<std::vector<unsigned int>, U>{}(pvalue_vui);
    else if (
        auto pvalue_vul = variantSrc::get_if<std::vector<unsigned long> >(&v))
        return DoConvert<std::vector<unsigned long>, U>{}(pvalue_vul);
    else if (
        auto pvalue_vull =
            variantSrc::get_if<std::vector<unsigned long long> >(&v))
        return DoConvert<std::vector<unsigned long long>, U>{}(pvalue_vull);
    else if (auto pvalue_vf = variantSrc::get_if<std::vector<float> >(&v))
        return DoConvert<std::vector<float>, U>{}(pvalue_vf);
    else if (auto pvalue_vd = variantSrc::get_if<std::vector<double> >(&v))
        return DoConvert<std::vector<double>, U>{}(pvalue_vd);
    else if (
        auto pvalue_vld = variantSrc::get_if<std::vector<long double> >(&v))
        return DoConvert<std::vector<long double>, U>{}(pvalue_vld);
    else if (
        auto pvalue_vcf =
            variantSrc::get_if<std::vector<std::complex<float> > >(&v))
        return DoConvert<std::vector<std::complex<float> >, U>{}(pvalue_vcf);
    else if (
        auto pvalue_vcd =
            variantSrc::get_if<std::vector<std::complex<double> > >(&v))
        return DoConvert<std::vector<std::complex<double> >, U>{}(pvalue_vcd);
    else if (
        auto pvalue_vcld =
            variantSrc::get_if<std::vector<std::complex<long double> > >(&v))
        return DoConvert<std::vector<std::complex<long double> >, U>{}(
            pvalue_vcld);
    else if (
        auto pvalue_vstr = variantSrc::get_if<std::vector<std::string> >(&v))
        return DoConvert<std::vector<std::string>, U>{}(pvalue_vstr);
    // extra
    else if (auto pvalue_vad = variantSrc::get_if<std::array<double, 7> >(&v))
        return DoConvert<std::array<double, 7>, U>{}(pvalue_vad);
    else if (auto pvalue_b = variantSrc::get_if<bool>(&v))
        return DoConvert<bool, U>{}(pvalue_b);
    else
        throw std::runtime_error("getCast: unknown Datatype.");

#else
    return variantSrc::visit(
        [](auto &&containedValue) -> U {
            using containedType = std::decay_t<decltype(containedValue)>;
            return DoConvert<containedType, U>{}(&containedValue);
        },
        v);
#endif
}

template <typename U>
U Attribute::get() const
{
    return getCast<U>(Variant::getResource());
}

} // namespace openPMD
