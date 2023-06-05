// Attribute

#include "defs.hpp"

namespace
{
struct UseType
{
    template <typename T>
    static void call(jlcxx::TypeWrapper<Attribute> type)
    {
        type.method(
            "cxx_get_" + datatypeToString(determineDatatype<T>()),
            &Attribute::get<T>);
    }
};
} // namespace

void define_julia_Attribute(jlcxx::Module &mod)
{
    auto type = mod.add_type<Attribute>("CXX_Attribute");

    type.method("cxx_dtype", [](const Attribute &attr) { return attr.dtype; });

    forallJuliaTypes<UseType>(type);
}
