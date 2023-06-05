// Iteration

#include "defs.hpp"

#include <string>

// Define supertype relationships
namespace jlcxx
{
template <>
struct SuperType<Iteration>
{
    using type = Attributable;
};
} // namespace jlcxx

void define_julia_Iteration(jlcxx::Module &mod)
{
    auto type = mod.add_type<Iteration>(
        "CXX_Iteration", jlcxx::julia_base_type<Attributable>());

    type.method("cxx_time", &Iteration::time<double>);
    type.method("cxx_set_time!", &Iteration::setTime<double>);
    type.method("cxx_dt", &Iteration::dt<double>);
    type.method("cxx_set_dt!", &Iteration::setDt<double>);
    type.method("cxx_time_unit_SI", &Iteration::timeUnitSI);
    type.method("cxx_set_time_unit_SI!", &Iteration::setTimeUnitSI);
    type.method("cxx_close", overload_cast<bool>(&Iteration::close));
    type.method("cxx_open", &Iteration::open);
    type.method("cxx_closed", &Iteration::closed);
    type.method("cxx_meshes", [](Iteration &iter) -> Container<Mesh> & {
        return iter.meshes;
    });
    // TODO: particles
}
