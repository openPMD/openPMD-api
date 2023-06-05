// shared_ptr

#include "defs.hpp"
#include "openPMD/Datatype.hpp"
#include <jlcxx/module.hpp>

namespace
{
struct UseType
{
    template <typename T>
    static void call(jlcxx::Module &mod)
    {
        mod.method(
            "create_aliasing_shared_ptr_" +
                datatypeToString(determineDatatype<T>()),
            &create_aliasing_shared_ptr<T>);
    }
};
} // namespace

void define_julia_shared_ptr(jlcxx::Module &mod)
{
    forallScalarJuliaTypes<UseType>(mod);
}
