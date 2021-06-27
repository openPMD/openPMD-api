// IO/UnitDimension

#include "defs.hpp"

void define_julia_UnitDimension(jlcxx::Module &mod) {
  mod.add_bits<UnitDimension>("UnitDimension", jlcxx::julia_type("CppEnum"));
  jlcxx::stl::apply_stl<UnitDimension>(mod);

  mod.set_const("L", UnitDimension::L);
  mod.set_const("M", UnitDimension::M);
  mod.set_const("T", UnitDimension::T);
  mod.set_const("I", UnitDimension::I);
  mod.set_const("θ", UnitDimension::theta);
  mod.set_const("N", UnitDimension::N);
  mod.set_const("J", UnitDimension::J);
}
