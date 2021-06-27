// Iteration

#include "defs.hpp"

#include <string>

// Define supertype relationships
namespace jlcxx {
template <> struct SuperType<Iteration> { typedef Attributable type; };
} // namespace jlcxx

void define_julia_Iteration(jlcxx::Module &mod) {
  auto type = mod.add_type<Iteration>("Iteration",
                                      jlcxx::julia_base_type<Attributable>());

  type.method("time1", &Iteration::time<double>);
  type.method("set_time!", &Iteration::setTime<double>);
  type.method("dt", &Iteration::dt<double>);
  type.method("set_dt!", &Iteration::setDt<double>);
  type.method("time_unit_SI", &Iteration::timeUnitSI);
  type.method("set_time_unit_SI!", &Iteration::setTimeUnitSI);
  type.method("close1",
              static_cast<Iteration &(Iteration::*)(bool)>(&Iteration::close));
  type.method("open", &Iteration::open);
  type.method("closed", &Iteration::closed);
  type.method("closed_by_writer", &Iteration::closedByWriter);
  type.method("meshes",
              [](Iteration &iter) -> Container<Mesh> & { return iter.meshes; });
  // TODO: particles
}
