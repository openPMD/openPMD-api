/* Copyright 2018-2025 Axel Huebl, Franz Poeschel
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

#include "Common.hpp"

#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

/*
 * nanobind does not wrap the numpy C API (unlike pybind11 with py::array and
 * py::dtype), so the helpers in this file go through the Python-level numpy
 * API and the Python buffer protocol instead.
 */
namespace openPMD::python
{
/** The numpy module, imported on first use */
inline nb::handle numpy()
{
    // intentionally leaked: must not be released after interpreter shutdown
    static PyObject *module = nb::module_::import_("numpy").release().ptr();
    return module;
}

namespace detail
{
    /** isinstance(obj, numpy.<type_name>), false if numpy is not available
     */
    inline bool is_numpy_instance(PyObject *obj, char const *type_name)
    {
        try
        {
            int res = PyObject_IsInstance(obj, numpy().attr(type_name).ptr());
            if (res < 0)
            {
                PyErr_Clear();
            }
            return res == 1;
        }
        catch (std::exception const &)
        {
            return false;
        }
    }

    inline bool is_numpy_dtype(PyObject *obj)
    {
        return is_numpy_instance(obj, "dtype");
    }

    inline bool check_buffer(PyObject *obj)
    {
        return PyObject_CheckBuffer(obj) != 0;
    }
} // namespace detail

/** A numpy.dtype object */
class dtype : public nb::object
{
    NB_OBJECT_DEFAULT(dtype, object, "numpy.dtype", detail::is_numpy_dtype)

    /** numpy.dtype(arg) */
    explicit dtype(char const *format)
        : nb::object(numpy().attr("dtype")(format))
    {}

    /** The character code of the dtype */
    char char_() const
    {
        return nb::cast<std::string>(attr("char")).at(0);
    }
};

/** Any object supporting the Python buffer protocol */
class buffer : public nb::object
{
    NB_OBJECT_DEFAULT(
        buffer, object, "collections.abc.Buffer", detail::check_buffer)
};

/** Information on a buffer as returned by the Python buffer protocol
 *
 * The view is released upon construction, so the referenced object must be
 * kept alive while using ptr.
 */
struct buffer_info
{
    void *ptr = nullptr;
    Py_ssize_t itemsize = 0;
    Py_ssize_t size = 0;
    std::string format;
    Py_ssize_t ndim = 0;
    std::vector<Py_ssize_t> shape;
    std::vector<Py_ssize_t> strides;
    bool readonly = false;
    /** PyBuffer_IsContiguous(view, 'C') and (..., 'A') */
    bool c_contiguous = false;
    bool any_contiguous = false;

    buffer_info(nb::handle obj, bool writable = false)
    {
        Py_buffer view;
        int flags = PyBUF_STRIDES | PyBUF_FORMAT;
        if (writable)
        {
            flags |= PyBUF_WRITABLE;
        }
        if (PyObject_GetBuffer(obj.ptr(), &view, flags) != 0)
        {
            throw nb::python_error();
        }
        ptr = view.buf;
        itemsize = view.itemsize;
        format = view.format ? view.format : "B";
        ndim = view.ndim;
        shape.assign(view.shape, view.shape + view.ndim);
        strides.assign(view.strides, view.strides + view.ndim);
        size = 1;
        for (auto extent : shape)
        {
            size *= extent;
        }
        readonly = view.readonly != 0;
        c_contiguous = PyBuffer_IsContiguous(&view, 'C') != 0;
        any_contiguous = PyBuffer_IsContiguous(&view, 'A') != 0;
        PyBuffer_Release(&view);
    }
};

/** A numpy.ndarray (or subclass), same as pybind11::array */
class array : public nb::object
{
public:
    array() = default;
    explicit array(nb::object obj) : nb::object(std::move(obj))
    {}

    /** numpy.empty(shape, dtype) */
    template <typename Shape>
    array(python::dtype const &dt, Shape const &shape)
        : nb::object(numpy().attr("empty")(nb::cast(shape), dt))
    {}

    static bool check_(nb::handle h)
    {
        return detail::is_numpy_instance(h.ptr(), "ndarray");
    }

    Py_ssize_t ndim() const
    {
        return nb::cast<Py_ssize_t>(attr("ndim"));
    }

    std::vector<Py_ssize_t> shape() const
    {
        return nb::cast<std::vector<Py_ssize_t>>(attr("shape"));
    }

    python::dtype dtype() const
    {
        return nb::borrow<python::dtype>(attr("dtype"));
    }

    void *mutable_data() const
    {
        if (!nb::cast<bool>(attr("flags").attr("writeable")))
        {
            throw std::domain_error("array is not writeable");
        }
        return buffer_info(*this).ptr;
    }
};
} // namespace openPMD::python

namespace nanobind::detail
{
template <>
struct type_caster<openPMD::python::array>
{
    NB_TYPE_CASTER(openPMD::python::array, const_name("numpy.ndarray"))

    bool from_python(handle src, uint32_t, cleanup_list *) noexcept
    {
        if (!openPMD::python::array::check_(src))
        {
            return false;
        }
        value = openPMD::python::array(borrow(src));
        return true;
    }

    static handle from_cpp(
        openPMD::python::array const &value, rv_policy, cleanup_list *) noexcept
    {
        return value.inc_ref();
    }
};
} // namespace nanobind::detail

namespace openPMD
{
inline Datatype dtype_from_numpy(python::dtype const dt)
{
    // Use explicit PEP 3118 / struct format character codes for portable
    // cross-platform behavior. This ensures consistency with
    // dtype_from_bufferformat() and avoids platform-dependent interpretation
    // of numpy type name strings.
    // ref: https://docs.python.org/3/library/struct.html#format-characters
    // ref: https://numpy.org/doc/stable/reference/arrays.interface.html
    // ref: https://docs.scipy.org/doc/numpy/user/basics.types.html
    char const c = dt.char_();
    switch (c)
    {
    case 'b': // signed char
        if constexpr (std::is_signed_v<char>)
        {
            return Datatype::CHAR;
        }
        else
        {
            return Datatype::SCHAR;
        }
    case 'B': // unsigned char
        if constexpr (std::is_unsigned_v<char>)
        {
            return Datatype::CHAR;
        }
        else
        {
            return Datatype::UCHAR;
        }
    case 'h': // short
        return Datatype::SHORT;
    case 'H': // unsigned short
        return Datatype::USHORT;
    case 'i': // int
        return Datatype::INT;
    case 'I': // unsigned int
        return Datatype::UINT;
    case 'l': // long
        return Datatype::LONG;
    case 'L': // unsigned long
        return Datatype::ULONG;
    case 'q': // long long
        return Datatype::LONGLONG;
    case 'Q': // unsigned long long
        return Datatype::ULONGLONG;
    case 'f': // float
        return Datatype::FLOAT;
    case 'd': // double
        return Datatype::DOUBLE;
    case 'g': // long double
        return Datatype::LONG_DOUBLE;
    case 'F': // complex float
        return Datatype::CFLOAT;
    case 'D': // complex double
        return Datatype::CDOUBLE;
    case 'G': // complex long double
        return Datatype::CLONG_DOUBLE;
    case '?': // bool
        return Datatype::BOOL;
    default:
        nb::print(dt);
        throw std::runtime_error(
            std::string("Datatype '") + c +
            std::string("' not known in 'dtype_from_numpy'!"));
    }
}

inline Datatype dtype_from_numpy(nb::object dt)
{
    if (nb::isinstance<python::dtype>(dt))
    {
        return dtype_from_numpy(nb::borrow<python::dtype>(dt));
    }
    else
    {
        nb::handle numpy;
        try
        {
            numpy = python::numpy();
        }
        catch (std::exception const &e)
        {
            throw std::runtime_error(
                std::string(
                    "dtype_from_numpy: Cannot convert from object type to "
                    "datatype without numpy, failed importing: ") +
                e.what());
        }
        nb::object create_dtype = numpy.attr("dtype");
        nb::object dtype_obj;
        try
        {
            dtype_obj = create_dtype(dt);
        }
        catch (std::exception const &e)
        {
            throw std::runtime_error(
                std::string("dtype_from_numpy: Failed to create dtype from: ") +
                nb::cast<std::string>(nb::str(dt)) + std::string(", error: ") +
                e.what());
        }
        return dtype_from_numpy(nb::borrow<python::dtype>(dtype_obj));
    }
}

/** Return openPMD::Datatype from python::buffer_info::format
 */
inline Datatype dtype_from_bufferformat(std::string const &fmt)
{
    using DT = Datatype;

    // refs:
    //   https://docs.scipy.org/doc/numpy-1.15.0/reference/arrays.interface.html
    //   https://docs.python.org/3/library/struct.html#format-characters
    // std::cout << "  scalar type '" << fmt << "'" << std::endl;
    // typestring: encoding + type + number of bytes
    if (fmt.find("?") != std::string::npos)
        return DT::BOOL;
    else if (fmt.find("b") != std::string::npos)
        if constexpr (std::is_signed_v<char>)
        {
            return Datatype::CHAR;
        }
        else
        {
            return Datatype::SCHAR;
        }
    else if (fmt.find("h") != std::string::npos)
        return DT::SHORT;
    else if (fmt.find("i") != std::string::npos)
        return DT::INT;
    else if (fmt.find("l") != std::string::npos)
        return DT::LONG;
    else if (fmt.find("q") != std::string::npos)
        return DT::LONGLONG;
    else if (fmt.find("B") != std::string::npos)
        if constexpr (std::is_unsigned_v<char>)
        {
            return Datatype::CHAR;
        }
        else
        {
            return Datatype::UCHAR;
        }
    else if (fmt.find("H") != std::string::npos)
        return DT::USHORT;
    else if (fmt.find("I") != std::string::npos)
        return DT::UINT;
    else if (fmt.find("L") != std::string::npos)
        return DT::ULONG;
    else if (fmt.find("Q") != std::string::npos)
        return DT::ULONGLONG;
    else if (fmt.find("Zf") != std::string::npos)
        return DT::CFLOAT;
    else if (fmt.find("Zd") != std::string::npos)
        return DT::CDOUBLE;
    else if (fmt.find("Zg") != std::string::npos)
        return DT::CLONG_DOUBLE;
    else if (fmt.find("f") != std::string::npos)
        return DT::FLOAT;
    else if (fmt.find("d") != std::string::npos)
        return DT::DOUBLE;
    else if (fmt.find("g") != std::string::npos)
        return DT::LONG_DOUBLE;
    else
        throw std::runtime_error(
            "dtype_from_bufferformat: Unknown "
            "Python type '" +
            fmt + "'");
}

inline python::dtype dtype_to_numpy(Datatype const dt)
{
    // Use explicit PEP 3118 format character codes for portable behavior.
    // This ensures round-trip consistency with dtype_from_numpy().
    using DT = Datatype;
    switch (dt)
    {
    case DT::CHAR:
    case DT::VEC_CHAR:
    case DT::STRING:
    case DT::VEC_STRING:
        if constexpr (std::is_signed_v<char>)
        {
            return python::dtype("b");
        }
        else
        {
            return python::dtype("B");
        }
    case DT::SCHAR:
    case DT::VEC_SCHAR:
        return python::dtype("b");
    case DT::UCHAR:
    case DT::VEC_UCHAR:
        return python::dtype("B");
    case DT::SHORT:
    case DT::VEC_SHORT:
        return python::dtype("h");
    case DT::INT:
    case DT::VEC_INT:
        return python::dtype("i");
    case DT::LONG:
    case DT::VEC_LONG:
        return python::dtype("l");
    case DT::LONGLONG:
    case DT::VEC_LONGLONG:
        return python::dtype("q");
    case DT::USHORT:
    case DT::VEC_USHORT:
        return python::dtype("H");
    case DT::UINT:
    case DT::VEC_UINT:
        return python::dtype("I");
    case DT::ULONG:
    case DT::VEC_ULONG:
        return python::dtype("L");
    case DT::ULONGLONG:
    case DT::VEC_ULONGLONG:
        return python::dtype("Q");
    case DT::FLOAT:
    case DT::VEC_FLOAT:
        return python::dtype("f");
    case DT::DOUBLE:
    case DT::VEC_DOUBLE:
    case DT::ARR_DBL_7:
        return python::dtype("d");
    case DT::LONG_DOUBLE:
    case DT::VEC_LONG_DOUBLE:
        return python::dtype("g");
    case DT::CFLOAT:
    case DT::VEC_CFLOAT:
        return python::dtype("F");
    case DT::CDOUBLE:
    case DT::VEC_CDOUBLE:
        return python::dtype("D");
    case DT::CLONG_DOUBLE:
    case DT::VEC_CLONG_DOUBLE:
        return python::dtype("G");
    case DT::BOOL:
        return python::dtype("?");
    case DT::UNDEFINED:
    default:
        throw std::runtime_error("dtype_to_numpy: Invalid Datatype!");
    }
}
} // namespace openPMD
