#ifndef CONTAINER_HPP
#define CONTAINER_HPP

// Container

#include "defs.hpp"

#include <memory>
#include <type_traits>

// Define supertype relationships
namespace jlcxx
{
template <typename T, typename K>
struct SuperType<Container<T, K>>
{
    using type = Attributable;
};
} // namespace jlcxx

using julia_Container_type_t =
    jlcxx::TypeWrapper<jlcxx::Parametric<jlcxx::TypeVar<1>, jlcxx::TypeVar<2>>>;
// TODO: use std::optional instead of std::unique_ptr
extern std::unique_ptr<julia_Container_type_t> julia_Container_type;

template <typename Eltype, typename Keytype>
void define_julia_Container(jlcxx::Module &mod)
{
    if (!julia_Container_type)
        julia_Container_type = std::make_unique<julia_Container_type_t>(
            mod.add_type<
                jlcxx::Parametric<jlcxx::TypeVar<1>, jlcxx::TypeVar<2>>>(
                "CXX_Container", jlcxx::julia_base_type<Attributable>()));

    julia_Container_type->apply<Container<Eltype, Keytype>>([](auto type) {
        using ContainerT = typename decltype(type)::type;
        using key_type = typename ContainerT::key_type;
        using mapped_type = typename ContainerT::mapped_type;
        using size_type = typename ContainerT::size_type;
        static_assert(std::is_same_v<Eltype, mapped_type>);
        static_assert(std::is_same_v<Keytype, key_type>);

        type.template constructor<const ContainerT &>();

        type.method("cxx_empty", &ContainerT::empty);
        type.method("cxx_length", &ContainerT::size);
        type.method("cxx_empty!", &ContainerT::clear);
        // type.method("cxx_getindex",
        //             static_cast<mapped_type &(ContainerT::*)(const key_type
        //             &)>(
        //                 &ContainerT::at));
        type.method(
            "cxx_getindex",
            [](ContainerT &cont, const key_type &key) -> mapped_type & {
                return cont[key];
            });
        type.method(
            "cxx_setindex!",
            [](ContainerT &cont,
               const mapped_type &value,
               const key_type &key) { return cont[key] = value; });
        type.method("cxx_count", &ContainerT::count);
        type.method("cxx_contains", &ContainerT::contains);
        type.method(
            "cxx_delete!",
            static_cast<size_type (ContainerT::*)(const key_type &)>(
                &ContainerT::erase));
        type.method("cxx_keys", [](const ContainerT &cont) {
            std::vector<key_type> res;
            res.reserve(cont.size());
            for (auto iter = cont.begin(); iter != cont.end(); ++iter)
                res.push_back(iter->first);
            return res;
        });
        // type.method("cxx_values", [](const ContainerT &cont) {
        //   std::vector<mapped_type *> res;
        //   res.reserve(cont.size());
        //   for (auto iter = cont.begin(); iter != cont.end(); ++iter)
        //     res.push_back(&iter->second);
        //   return res;
        // });
        // type.method("cxx_collect", [](const ContainerT &cont) {
        //   std::vector<std::pair<key_type, mapped_type *>> res;
        //   res.reserve(cont.size());
        //   for (auto iter = cont.begin(); iter != cont.end(); ++iter)
        //     res.emplace_back(iter->first, &iter->second);
        //   return res;
        // });
    });
}

#endif // #ifndef CONTAINER_HPP
