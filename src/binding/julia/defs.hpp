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
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace openPMD;

template <typename T> using array7 = std::array<T, 7>;

// Generate code fo all openPMD types. Use is e.g. as follows:
//   #define USE_TYPE(NAME, ENUM, TYPE) \
//     type.method("get_" NAME, &Attribute::get<TYPE>);
//   FORALL_OPENPMD_TYPES
//   #undef USE_TYPE
//
// We disable `long double` since Julia does not support this type
// We disable `long double` since Julia does not support this type
#define FORALL_OPENPMD_TYPES                                                   \
  USE_TYPE("CHAR", Datatype::CHAR, char)                                       \
  USE_TYPE("UCHAR", Datatype::UCHAR, unsigned char)                            \
  USE_TYPE("SHORT", Datatype::SHORT, short)                                    \
  USE_TYPE("INT", Datatype::INT, int)                                          \
  USE_TYPE("LONG", Datatype::LONG, long)                                       \
  USE_TYPE("LONGLONG", Datatype::LONGLONG, long long)                          \
  USE_TYPE("USHORT", Datatype::USHORT, unsigned short)                         \
  USE_TYPE("UINT", Datatype::UINT, unsigned int)                               \
  USE_TYPE("ULONG", Datatype::ULONG, unsigned long)                            \
  USE_TYPE("ULONGLONG", Datatype::ULONGLONG, unsigned long long)               \
  USE_TYPE("FLOAT", Datatype::FLOAT, float)                                    \
  USE_TYPE("DOUBLE", Datatype::DOUBLE, double)                                 \
  /* USE_TYPE("LONG_DOUBLE", Datatype::LONG_DOUBLE, long double) */            \
  USE_TYPE("CFLOAT", Datatype::CFLOAT, std::complex<float>)                    \
  USE_TYPE("CDOUBLE", Datatype::CDOUBLE, std::complex<double>)                 \
  /* USE_TYPE("CLONG_DOUBLE", Datatype::CLONG_DOUBLE, std::complex<long        \
   * double>) */                                                               \
  USE_TYPE("STRING", Datatype::STRING, std::string)                            \
  USE_TYPE("VEC_CHAR", Datatype::VEC_CHAR, std::vector<char>)                  \
  USE_TYPE("VEC_UCHAR", Datatype::VEC_UCHAR, std::vector<unsigned char>)       \
  USE_TYPE("VEC_SHORT", Datatype::VEC_SHORT, std::vector<short>)               \
  USE_TYPE("VEC_INT", Datatype::VEC_INT, std::vector<int>)                     \
  USE_TYPE("VEC_LONG", Datatype::VEC_LONG, std::vector<long>)                  \
  USE_TYPE("VEC_LONGLONG", Datatype::VEC_LONGLONG, std::vector<long long>)     \
  USE_TYPE("VEC_USHORT", Datatype::VEC_USHORT, std::vector<unsigned short>)    \
  USE_TYPE("VEC_UINT", Datatype::VEC_UINT, std::vector<unsigned int>)          \
  USE_TYPE("VEC_ULONG", Datatype::VEC_ULONG, std::vector<unsigned long>)       \
  USE_TYPE("VEC_ULONGLONG", Datatype::VEC_ULONGLONG,                           \
           std::vector<unsigned long long>)                                    \
  USE_TYPE("VEC_FLOAT", Datatype::VEC_FLOAT, std::vector<float>)               \
  USE_TYPE("VEC_DOUBLE", Datatype::VEC_DOUBLE, std::vector<double>)            \
  /* USE_TYPE("VEC_LONG_DOUBLE", Datatype::VEC_LONG_DOUBLE, std::vector<long   \
   * double>) */                                                               \
  USE_TYPE("VEC_CFLOAT", Datatype::VEC_CFLOAT,                                 \
           std::vector<std::complex<float>>)                                   \
  USE_TYPE("VEC_CDOUBLE", Datatype::VEC_CDOUBLE,                               \
           std::vector<std::complex<double>>)                                  \
  /* USE_TYPE("VEC_CLONG_DOUBLE", Datatype::VEC_CLONG_DOUBLE,                  \
   * std::vector<std::complex<long double>>) */                                \
  USE_TYPE("VEC_STRING", Datatype::VEC_STRING, std::vector<std::string>)       \
  USE_TYPE("ARR_DBL_7", Datatype::ARR_DBL_7, array7<double>)                   \
  USE_TYPE("BOOL", Datatype::BOOL, bool)

#define FORALL_SCALAR_OPENPMD_TYPES                                            \
  USE_TYPE("CHAR", Datatype::CHAR, char)                                       \
  USE_TYPE("UCHAR", Datatype::UCHAR, unsigned char)                            \
  USE_TYPE("SHORT", Datatype::SHORT, short)                                    \
  USE_TYPE("INT", Datatype::INT, int)                                          \
  USE_TYPE("LONG", Datatype::LONG, long)                                       \
  USE_TYPE("LONGLONG", Datatype::LONGLONG, long long)                          \
  USE_TYPE("USHORT", Datatype::USHORT, unsigned short)                         \
  USE_TYPE("UINT", Datatype::UINT, unsigned int)                               \
  USE_TYPE("ULONG", Datatype::ULONG, unsigned long)                            \
  USE_TYPE("ULONGLONG", Datatype::ULONGLONG, unsigned long long)               \
  USE_TYPE("FLOAT", Datatype::FLOAT, float)                                    \
  USE_TYPE("DOUBLE", Datatype::DOUBLE, double)                                 \
  /* USE_TYPE("LONG_DOUBLE", Datatype::LONG_DOUBLE, long double) */            \
  USE_TYPE("CFLOAT", Datatype::CFLOAT, std::complex<float>)                    \
  USE_TYPE("CDOUBLE", Datatype::CDOUBLE, std::complex<double>)                 \
  /* USE_TYPE("CLONG_DOUBLE", Datatype::CLONG_DOUBLE, std::complex<long        \
   * double>) */                                                               \
  USE_TYPE("STRING", Datatype::STRING, std::string)                            \
  USE_TYPE("ARR_DBL_7", Datatype::ARR_DBL_7, array7<double>)                   \
  USE_TYPE("BOOL", Datatype::BOOL, bool)

namespace {
template <typename T, typename U>
std::vector<std::pair<T, U>> map_to_vector_pair(const std::map<T, U> &m) {
  std::vector<std::pair<std::string, bool>> vp;
  vp.reserve(m.size());
  for (const auto &p : m)
    vp.push_back(p);
  return vp;
}

template <typename T, typename U>
std::vector<std::tuple<T, U>> map_to_vector_tuple(const std::map<T, U> &m) {
  std::vector<std::tuple<std::string, bool>> vp;
  vp.reserve(m.size());
  for (const auto &p : m)
    vp.emplace_back(p.first, p.second);
  return vp;
}

template <typename T>
std::shared_ptr<T> capture_vector_as_buffer(std::vector<T> &vec) {
  if constexpr (std::is_same_v<T, bool>) {
    // We cannot handle std::vector<bool> because it is special
    std::abort();
  } else {
    auto deleter = [](T *) { /* do not delete anything */ };
    std::shared_ptr<T> ptr(vec.data(), std::move(deleter));
    return ptr;
  }
}

template <typename T> std::shared_ptr<T> capture_vector(std::vector<T> vec) {
  if constexpr (std::is_same_v<T, bool>) {
    // Copy the vector, because std::vector<bool> is special
    T *dataptr = new T[vec.size()];
    std::shared_ptr<T> ptr(dataptr, std::default_delete<T[]>());
    std::copy(vec.begin(), vec.end(), dataptr);
    return ptr;
  } else {
    // Capture the vector
    T *dataptr = vec.data();
    auto deleter = [vec = std::move(vec)](T *) {
      // We moved the vector into the anonymous function, and thus it will be
      // destructed when the anonymous function is destructed. There is no need
      // to call a destructor manually.
    };
    std::shared_ptr<T> ptr(dataptr, std::move(deleter));
    return ptr;
  }
}

template <typename T, std::size_t N>
void add_array_type(jlcxx::Module &mod, const std::string &name) {
  mod.add_type<std::array<T, N>>(name)
      .template constructor<>()
      .template constructor<const std::array<T, N> &>()
      .method("size1", &std::array<T, N>::size)
      .method("getindex1",
              [](const std::array<T, N> &a, std::size_t n) { return a[n]; });
  jlcxx::stl::apply_stl<std::array<T, N>>(mod);
}

template <typename T, std::size_t N>
void map_array_type(jlcxx::Module &mod, const std::string &name) {
  mod.map_type<std::array<T, N>>(name);
  mod.method("size1", [](const std::array<T, N> &a) { return a.size(); });
  mod.method("getindex1",
             [](const std::array<T, N> &a, std::size_t n) { return a[n]; });
  jlcxx::stl::apply_stl<std::array<T, N>>(mod);
}

template <typename T, typename U>
void add_pair_type(jlcxx::Module &mod, const std::string &name) {
  mod.add_type<std::pair<T, U>>(name)
      .template constructor<>()
      .template constructor<const std::pair<T, U> &>()
      .method("first", [](const std::pair<T, U> &p) { return p.first; })
      .method("second", [](const std::pair<T, U> &p) { return p.second; });
  jlcxx::stl::apply_stl<std::pair<T, U>>(mod);
}

} // namespace

namespace jlcxx {
template <> struct IsMirroredType<std::array<double, 7>> : std::false_type {};
} // namespace jlcxx

// We use one function per header file
void define_julia_Access(jlcxx::Module &mod);
void define_julia_Attributable(jlcxx::Module &mod);
void define_julia_Attribute(jlcxx::Module &mod);
void define_julia_BaseRecordComponent(jlcxx::Module &mod);
template <typename Eltype, typename Keytype = std::string>
void define_julia_Container(jlcxx::Module &mod);
void define_julia_Dataset(jlcxx::Module &mod);
void define_julia_Datatype(jlcxx::Module &mod);
void define_julia_Format(jlcxx::Module &mod);
void define_julia_Iteration(jlcxx::Module &mod);
void define_julia_Mesh(jlcxx::Module &mod);
void define_julia_MeshRecordComponent(jlcxx::Module &mod);
void define_julia_RecordComponent(jlcxx::Module &mod);
void define_julia_RecordComponent_copy_chunk(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type);
void define_julia_RecordComponent_load_chunk(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type);
void define_julia_RecordComponent_load_chunk_buffer(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type);
void define_julia_RecordComponent_make_constant(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type);
void define_julia_RecordComponent_store_chunk(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type);
void define_julia_RecordComponent_store_chunk_copy(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type);
void define_julia_Series(jlcxx::Module &mod);
void define_julia_UnitDimension(jlcxx::Module &mod);
void define_julia_ReadIterations(jlcxx::Module &mod);
void define_julia_WriteIterations(jlcxx::Module &mod);
void define_julia_version(jlcxx::Module &mod);

#endif // #ifndef DEFS_HPP
