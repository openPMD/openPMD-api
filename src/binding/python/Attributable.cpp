/* Copyright 2018-2021 Axel Huebl
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
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/DatatypeHelpers.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attribute.hpp"

#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Numpy.hpp"

#include <pybind11/detail/common.h>

#include <algorithm>
#include <array>
#include <complex>
#include <exception>
#include <iterator>
#include <map>
#include <optional>
#include <pybind11/pytypes.h>
#include <string>
#include <type_traits>
#include <vector>

using PyAttributeKeys = std::vector<std::string>;

bool setAttributeFromBufferInfo(
    Attributable &attr, std::string const &key, py::buffer &a)
{
    using DT = Datatype;

    py::buffer_info buf = a.request();

    // Numpy: Handling of arrays and scalars
    // work-around for https://github.com/pybind/pybind11/issues/1224
    // -> passing numpy scalars as buffers needs numpy 1.15+
    //    https://github.com/numpy/numpy/issues/10265
    //    https://github.com/pybind/pybind11/issues/1224#issuecomment-354357392
    // scalars, see PEP 3118
    // requires Numpy 1.15+
    if (buf.ndim == 0)
    {
        // refs:
        //   https://docs.scipy.org/doc/numpy-1.15.0/reference/arrays.interface.html
        //   https://docs.python.org/3/library/struct.html#format-characters
        // std::cout << "  scalar type '" << buf.format << "'" << std::endl;
        // typestring: encoding + type + number of bytes
        switch (dtype_from_bufferformat(buf.format))
        {
        case DT::BOOL:
            return attr.setAttribute(key, *static_cast<bool *>(buf.ptr));
            break;
        case DT::SHORT:
            return attr.setAttribute(key, *static_cast<short *>(buf.ptr));
            break;
        case DT::INT:
            return attr.setAttribute(key, *static_cast<int *>(buf.ptr));
            break;
        case DT::LONG:
            return attr.setAttribute(key, *static_cast<long *>(buf.ptr));
            break;
        case DT::LONGLONG:
            return attr.setAttribute(key, *static_cast<long long *>(buf.ptr));
            break;
        case DT::USHORT:
            return attr.setAttribute(
                key, *static_cast<unsigned short *>(buf.ptr));
            break;
        case DT::UINT:
            return attr.setAttribute(
                key, *static_cast<unsigned int *>(buf.ptr));
            break;
        case DT::ULONG:
            return attr.setAttribute(
                key, *static_cast<unsigned long *>(buf.ptr));
            break;
        case DT::ULONGLONG:
            return attr.setAttribute(
                key, *static_cast<unsigned long long *>(buf.ptr));
            break;
        case DT::FLOAT:
            return attr.setAttribute(key, *static_cast<float *>(buf.ptr));
            break;
        case DT::DOUBLE:
            return attr.setAttribute(key, *static_cast<double *>(buf.ptr));
            break;
        case DT::LONG_DOUBLE:
            return attr.setAttribute(key, *static_cast<long double *>(buf.ptr));
            break;
        case DT::CFLOAT:
            return attr.setAttribute(
                key, *static_cast<std::complex<float> *>(buf.ptr));
            break;
        case DT::CDOUBLE:
            return attr.setAttribute(
                key, *static_cast<std::complex<double> *>(buf.ptr));
            break;
        case DT::CLONG_DOUBLE:
            return attr.setAttribute(
                key, *static_cast<std::complex<long double> *>(buf.ptr));
            break;
        default:
            throw std::runtime_error(
                "set_attribute: Unknown "
                "Python type '" +
                buf.format + "' for attribute '" + key + "'");
        }
        return false;
    }
    // lists & ndarrays: all will be flattended to 1D lists
    else
    {
        // std::cout << "  array type '" << buf.format << "'" << std::endl;

        /* required are contiguous buffers
         *
         * - not strided with paddings
         * - not a view in another buffer that results in striding
         */
        auto *view = new Py_buffer();
        int flags = PyBUF_STRIDES | PyBUF_FORMAT;
        if (PyObject_GetBuffer(a.ptr(), view, flags) != 0)
        {
            delete view;
            throw py::error_already_set();
        }
        bool isContiguous = (PyBuffer_IsContiguous(view, 'A') != 0);
        PyBuffer_Release(view);
        delete view;

        if (!isContiguous)
            throw py::index_error(
                "non-contiguous buffer provided, handling not implemented!");
        // @todo in order to implement stride handling, one needs to
        //       loop over the input data strides during write below,
        //       also data might not be owned!

        // dtype handling
        /*if( buf.format.find("?") != std::string::npos )
            return attr.setAttribute( key,
                std::vector<bool>(
                    static_cast<bool*>(buf.ptr),
                    static_cast<bool*>(buf.ptr) + buf.size
                ) );
        else */
        // std::cout << "+++++++++++ BUFFER: " << buf.format << std::endl;
        if (buf.format.find('b') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<char>(
                    static_cast<char *>(buf.ptr),
                    static_cast<char *>(buf.ptr) + buf.size));
        else if (buf.format.find('h') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<short>(
                    static_cast<short *>(buf.ptr),
                    static_cast<short *>(buf.ptr) + buf.size));
        else if (buf.format.find('i') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<int>(
                    static_cast<int *>(buf.ptr),
                    static_cast<int *>(buf.ptr) + buf.size));
        else if (buf.format.find('l') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<long>(
                    static_cast<long *>(buf.ptr),
                    static_cast<long *>(buf.ptr) + buf.size));
        else if (buf.format.find('q') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<long long>(
                    static_cast<long long *>(buf.ptr),
                    static_cast<long long *>(buf.ptr) + buf.size));
        else if (buf.format.find('B') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<unsigned char>(
                    static_cast<unsigned char *>(buf.ptr),
                    static_cast<unsigned char *>(buf.ptr) + buf.size));
        else if (buf.format.find('H') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<unsigned short>(
                    static_cast<unsigned short *>(buf.ptr),
                    static_cast<unsigned short *>(buf.ptr) + buf.size));
        else if (buf.format.find('I') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<unsigned int>(
                    static_cast<unsigned int *>(buf.ptr),
                    static_cast<unsigned int *>(buf.ptr) + buf.size));
        else if (buf.format.find('L') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<unsigned long>(
                    static_cast<unsigned long *>(buf.ptr),
                    static_cast<unsigned long *>(buf.ptr) + buf.size));
        else if (buf.format.find('Q') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<unsigned long long>(
                    static_cast<unsigned long long *>(buf.ptr),
                    static_cast<unsigned long long *>(buf.ptr) + buf.size));
        else if (buf.format.find("Zf") != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<std::complex<float>>(
                    static_cast<std::complex<float> *>(buf.ptr),
                    static_cast<std::complex<float> *>(buf.ptr) + buf.size));
        else if (buf.format.find("Zd") != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<std::complex<double>>(
                    static_cast<std::complex<double> *>(buf.ptr),
                    static_cast<std::complex<double> *>(buf.ptr) + buf.size));
        else if (buf.format.find("Zg") != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<std::complex<long double>>(
                    static_cast<std::complex<long double> *>(buf.ptr),
                    static_cast<std::complex<long double> *>(buf.ptr) +
                        buf.size));
        else if (buf.format.find('f') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<float>(
                    static_cast<float *>(buf.ptr),
                    static_cast<float *>(buf.ptr) + buf.size));
        else if (buf.format.find('d') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<double>(
                    static_cast<double *>(buf.ptr),
                    static_cast<double *>(buf.ptr) + buf.size));
        else if (buf.format.find('g') != std::string::npos)
            return attr.setAttribute(
                key,
                std::vector<long double>(
                    static_cast<long double *>(buf.ptr),
                    static_cast<long double *>(buf.ptr) + buf.size));
        else
            throw std::runtime_error(
                "set_attribute: Unknown "
                "Python type '" +
                buf.format + "' for attribute '" + key + "'");

        return false;
    }
}

namespace detail
{
template <typename RequestedType>
bool setAttributeFromObject_default(
    Attributable &attr, std::string const &key, py::object &obj)
{
    if (std::string(py::str(obj.get_type())) == "<class 'list'>")
    {
        using ListType = std::vector<RequestedType>;
        return attr.setAttribute<ListType>(key, obj.cast<ListType>());
    }
    else
    {
        return attr.setAttribute<RequestedType>(key, obj.cast<RequestedType>());
    }
}

bool setAttributeFromObject_double(
    Attributable &attr, std::string const &key, py::object &obj)
{
    if (std::string(py::str(obj.get_type())) == "<class 'list'>")
    {
        using ListType = std::vector<double>;
        using ArrayType = std::array<double, 7>;
        ListType const &asVector = obj.cast<ListType>();
        if (asVector.size() == 7 && key == "unitDimension")
        {
            ArrayType asArray;
            std::copy_n(asVector.begin(), 7, asArray.begin());
            return attr.setAttribute<ArrayType>(key, asArray);
        }
        else
        {
            return attr.setAttribute<ListType>(key, asVector);
        }
    }
    else
    {
        return attr.setAttribute<double>(key, obj.cast<double>());
    }
}

bool setAttributeFromObject_bool(
    Attributable &attr, std::string const &key, py::object &obj)
{
    return attr.setAttribute<bool>(key, obj.cast<bool>());
}

template <bool is_signed = std::is_signed_v<char>>
struct char_to_explicit_char;

template <>
struct char_to_explicit_char<true>
{
    using type = signed char;
    using opposite_type = unsigned char;
};

template <>
struct char_to_explicit_char<false>
{
    using type = unsigned char;
    using opposite_type = signed char;
};

template <typename TargetType>
std::optional<TargetType> tryCast(py::object const &obj)
{
    // Do a check to avoid throwing exceptions
    if constexpr (std::is_default_constructible_v<TargetType>)
    {
        TargetType val{};
        auto python_val = py::cast(std::move(val));
        if (!py::isinstance(obj, python_val.get_type()))
        {
            return std::nullopt;
        }
    }
    try
    {
        return obj.cast<TargetType>();
    }
    catch (py::cast_error const &)
    {
        return std::nullopt;
    }
    catch (py::value_error const &err)
    {
        return std::nullopt;
    }
}

template <typename Char_t>
bool setAttributeFromObject_char(
    Attributable &attr, std::string const &key, py::object &obj)
{
    using explicit_char_type = std::conditional_t<
        std::is_same_v<Char_t, char>,
        typename char_to_explicit_char<>::type,
        Char_t>;
    using ListString = std::vector<std::string>;

    /*
     * We cannot distinguish strings with length 1 from chars at this place,
     * so avoid this cast. If the attribute is actually a char, skipping this
     * branch means that it might be upcasted to string.
     */
#if 0
    using ListChar = std::vector<char>;
    if (auto casted_char = tryCast<char>(obj); casted_char.has_value())
    {
        return attr.setAttribute<char>(key, *casted_char);
    } else
#endif

    // This must come after tryCast<char>
    // because tryCast<string> implicitly covers chars as well.
    if (auto casted_string = tryCast<std::string>(obj);
        casted_string.has_value())
    {
        return attr.setAttribute<std::string>(key, std::move(*casted_string));
    }
    // Assuming `char` is signed on the current platform,
    // then this cast will cover `signed char`.
    // It's a bit weird: The Numpy datatype will be the same as `char`
    // (.ie. 'b'), but the `py::object` will contain an integer.
    // Similar for `unsigned char` if `char`s are unsigned.
    else if (auto casted_int = tryCast<int>(obj); casted_int.has_value())
    {
        return attr.setAttribute<explicit_char_type>(
            key, explicit_char_type(*casted_int));
    }

    // NOW: List casts.
    // All list casts must come after all scalar casts,
    // because list casts implicitly cover scalars too.

    /*
     * We cannot distinguish strings with length 1 from chars at this place,
     * so avoid this cast. If the attribute is actually a vector of char,
     * skipping this branch means that it might be upcasted to a vector of
     * string.
     */
#if 0
    else if (auto list_of_char = tryCast<ListChar>(obj);
             list_of_char.has_value())
    {
        return attr.setAttribute<ListChar>(key, std::move(*list_of_char));
    }
#endif
    // this must come after tryCast<vector<char>>,
    // because tryCast<vector<string>> implicitly covers chars as well
    else if (auto list_of_string = tryCast<ListString>(obj);
             list_of_string.has_value())
    {
        return attr.setAttribute<ListString>(key, std::move(*list_of_string));
    }
    // Again: `char` vs. `signed char`, resp. `char` vs. `unsigned char`
    // depending on `char`'s signedness.
    else if (auto list_of_int = tryCast<std::vector<int>>(obj);
             list_of_int.has_value())
    {
        std::vector<explicit_char_type> casted;
        casted.reserve(list_of_int->size());
        std::transform(
            list_of_int->begin(),
            list_of_int->end(),
            std::back_inserter(casted),
            [](int const val) { return explicit_char_type(val); });
        return attr.setAttribute<std::vector<explicit_char_type>>(
            key, std::move(casted));
    }
    else
    {
        throw std::runtime_error(
            "[Python SetAttributeFromObject<char>] Was not able to use passed "
            "object as any char-based type.");
    }
}
} // namespace detail

struct SetAttributeFromObject
{
    static constexpr char const *errorMsg = "Attributable.set_attribute()";

    template <typename RequestedType>
    static bool
    call(Attributable &attr, std::string const &key, py::object &obj)
    {
        if constexpr (std::is_same_v<RequestedType, double>)
        {
            return ::detail::setAttributeFromObject_double(attr, key, obj);
        }
        else if constexpr (std::is_same_v<RequestedType, bool>)
        {
            return ::detail::setAttributeFromObject_bool(attr, key, obj);
        }
        else if constexpr (
            std::is_same_v<RequestedType, char> ||
            std::is_same_v<RequestedType, signed char> ||
            std::is_same_v<RequestedType, unsigned char>)
        {
            return ::detail::setAttributeFromObject_char<RequestedType>(
                attr, key, obj);
        }
        else
        {
            return ::detail::setAttributeFromObject_default<RequestedType>(
                attr, key, obj);
        }
    }
};

bool setAttributeFromObject(
    Attributable &attr,
    std::string const &key,
    py::object &obj,
    pybind11::dtype datatype)
{
    Datatype requestedDatatype = dtype_from_numpy(std::move(datatype));
    return switchNonVectorType<SetAttributeFromObject>(
        requestedDatatype, attr, key, obj);
}

void init_Attributable(py::module &m)
{
    py::class_<Attributable::MyPath>(m, "AttributablePath")
        .def_readonly("directory", &Attributable::MyPath::directory)
        .def_readonly("series_name", &Attributable::MyPath::seriesName)
        .def_readonly(
            "series_extension", &Attributable::MyPath::seriesExtension)
        .def_readonly("group", &Attributable::MyPath::group)
        .def_readonly("access", &Attributable::MyPath::access)
        .def_property_readonly("file_path", &Attributable::MyPath::filePath);

    py::class_<Attributable>(m, "Attributable")
        .def(py::init<Attributable const &>())

        .def(
            "__repr__",
            [](Attributable const &attr) {
                return "<openPMD.Attributable with '" +
                    std::to_string(attr.numAttributes()) + "' attribute(s)>";
            })
        .def(
            "series_flush",
            py::overload_cast<std::string>(&Attributable::seriesFlush),
            py::arg("backend_config") = "{}")
        .def(
            "iteration_flush",
            py::overload_cast<std::string>(&Attributable::iterationFlush),
            py::arg("backend_config") = "{}")

        .def_property_readonly(
            "attributes",
            [](Attributable &attr) { return attr.attributes(); },
            // ref + keepalive
            py::return_value_policy::move)

        // C++ pass-through API: Setter
        // note that the order of overloads is important!
        // all buffer protocol compatible objects, including numpy arrays if not
        // specialized specifically...
        .def(
            "set_attribute",
            [](Attributable &attr, std::string const &key, py::buffer &a) {
                // std::cout << "set attr via py::buffer: " << key << std::endl;
                return setAttributeFromBufferInfo(attr, key, a);
            })
        .def(
            "set_attribute",
            [](Attributable &attr,
               std::string const &key,
               py::object &obj,
               pybind11::dtype datatype) {
                return setAttributeFromObject(
                    attr, key, obj, std::move(datatype));
            },
            py::arg("key"),
            py::arg("value"),
            py::arg("datatype"))

        // fundamental Python types
        .def("set_attribute", &Attributable::setAttribute<bool>)
        .def(
            "set_attribute",
            &Attributable::setAttribute<
                typename ::detail::char_to_explicit_char<>::opposite_type>)
        // -> handle all native python integers as long
        // .def("set_attribute", &Attributable::setAttribute< short >)
        // .def("set_attribute", &Attributable::setAttribute< int >)
        // .def("set_attribute", &Attributable::setAttribute< long >)
        // .def("set_attribute", &Attributable::setAttribute< long long >)
        // .def("set_attribute", &Attributable::setAttribute< unsigned short >)
        // .def("set_attribute", &Attributable::setAttribute< unsigned int >)
        // .def("set_attribute", &Attributable::setAttribute< unsigned long >)
        // .def("set_attribute", &Attributable::setAttribute< unsigned long long
        // >)
        .def("set_attribute", &Attributable::setAttribute<long>)
        // work-around for https://github.com/pybind/pybind11/issues/1512
        // -> handle all native python floats as double
        // .def("set_attribute", &Attributable::setAttribute< float >)
        // .def("set_attribute", &Attributable::setAttribute< long double >)
        .def("set_attribute", &Attributable::setAttribute<double>)
        // work-around for https://github.com/pybind/pybind11/issues/1509
        // -> since there is only str in Python, chars are strings
        // .def("set_attribute", &Attributable::setAttribute< char >)
        .def(
            "set_attribute",
            [](Attributable &attr,
               std::string const &key,
               std::string const &value) {
                return attr.setAttribute(key, value);
            })

        // Plain Python arrays and plain python lists of homogeneous,
        // fundamental Python types not specialized in C++ API
        // .def("set_attribute", &Attributable::setAttribute< std::vector< bool
        // > >) there is only str in Python, chars are strings
        // .def("set_attribute", &Attributable::setAttribute< std::vector< char
        // > >)
        .def(
            "set_attribute",
            &Attributable::setAttribute<std::vector<unsigned char>>)
        .def("set_attribute", &Attributable::setAttribute<std::vector<long>>)
        .def(
            "set_attribute",
            &Attributable::setAttribute<std::vector<
                double>>) // TODO: this implicitly casts list of complex
        // probably affected by bug
        // https://github.com/pybind/pybind11/issues/1258
        .def(
            "set_attribute",
            [](Attributable &attr,
               std::string const &key,
               std::vector<std::string> const &value) {
                return attr.setAttribute(key, value);
            })
        // .def("set_attribute", &Attributable::setAttribute< std::array<
        // double, 7 > >)

        // C++ pass-through API: Getter
        .def(
            "get_attribute",
            [](Attributable &attr, std::string const &key) {
                auto v = attr.getAttribute(key);
                return v.getResource();
                // TODO instead of returning lists, return all arrays (ndim > 0)
                // as numpy arrays?
            })
        .def_property_readonly(
            "attribute_dtypes",
            [](Attributable const &attributable) {
                std::map<std::string, pybind11::dtype> dtypes;
                for (auto const &attr : attributable.attributes())
                {
                    dtypes[attr] =
                        dtype_to_numpy(attributable.getAttribute(attr).dtype);
                }
                return dtypes;
            })
        .def("delete_attribute", &Attributable::deleteAttribute)
        .def("contains_attribute", &Attributable::containsAttribute)

        // @todo _ipython_key_completions_ if we find a way to add a []
        // interface

        .def_property(
            "comment", &Attributable::comment, &Attributable::setComment)
        // TODO remove in future versions (deprecated)
        .def("set_comment", &Attributable::setComment)
        .def("my_path", &Attributable::myPath);

    py::bind_vector<PyAttributeKeys>(m, "Attribute_Keys");
}
