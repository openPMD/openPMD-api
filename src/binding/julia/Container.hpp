#ifndef CONTAINER_HPP
#define CONTAINER_HPP

// Container

#include "defs.hpp"

#include <memory>
#include <type_traits>

// Define supertype relationships
namespace jlcxx {
template <typename T, typename K> struct SuperType<Container<T, K>> {
  typedef Attributable type;
};
} // namespace jlcxx

// TODO: use std::optional instead of std::unique_ptr
typedef jlcxx::TypeWrapper<
    jlcxx::Parametric<jlcxx::TypeVar<1>, jlcxx::TypeVar<2>>>
    julia_Container_type_t;
extern std::unique_ptr<julia_Container_type_t> julia_Container_type;

template <typename Eltype, typename Keytype>
void define_julia_Container(jlcxx::Module &mod) {
  if (!julia_Container_type)
    julia_Container_type = std::make_unique<julia_Container_type_t>(
        mod.add_type<jlcxx::Parametric<jlcxx::TypeVar<1>, jlcxx::TypeVar<2>>>(
            "Container", jlcxx::julia_base_type<Attributable>()));

  julia_Container_type->apply<Container<Eltype, Keytype>>([](auto type) {
    using ContainerT = typename decltype(type)::type;
    using key_type = typename ContainerT::key_type;
    using mapped_type = typename ContainerT::mapped_type;
    using size_type = typename ContainerT::size_type;
    static_assert(std::is_same_v<Eltype, mapped_type>);
    static_assert(std::is_same_v<Keytype, key_type>);

    type.template constructor<const ContainerT &>();

    type.method("empty1", &ContainerT::empty);
    type.method("length1", &ContainerT::size);
    type.method("empty1!", &ContainerT::clear);
    type.method("getindex1",
                static_cast<mapped_type &(ContainerT::*)(const key_type &)>(
                    &ContainerT::at));
    type.method("get1!",
                [](ContainerT &cont, const key_type &key) -> mapped_type & {
                  return cont[key];
                });
    type.method("setindex1!",
                [](ContainerT &cont, const mapped_type &value,
                   const key_type &key) { return cont[key] = value; });
    type.method("count1", &ContainerT::count);
    type.method("contains1", &ContainerT::contains);
    type.method("delete1!",
                static_cast<size_type (ContainerT::*)(const key_type &)>(
                    &ContainerT::erase));
    type.method("keys1", [](const ContainerT &cont) {
      std::vector<key_type> res;
      res.reserve(cont.size());
      for (auto iter = cont.begin(); iter != cont.end(); ++iter)
        res.push_back(iter->first);
      return res;
    });
    // type.method("values1", [](const ContainerT &cont) {
    //   std::vector<mapped_type *> res;
    //   res.reserve(cont.size());
    //   for (auto iter = cont.begin(); iter != cont.end(); ++iter)
    //     res.push_back(&iter->second);
    //   return res;
    // });
    // type.method("collect1", [](const ContainerT &cont) {
    //   std::vector<std::pair<key_type, mapped_type *>> res;
    //   res.reserve(cont.size());
    //   for (auto iter = cont.begin(); iter != cont.end(); ++iter)
    //     res.emplace_back(iter->first, &iter->second);
    //   return res;
    // });
  });
}

#endif // #ifndef CONTAINER_HPP
