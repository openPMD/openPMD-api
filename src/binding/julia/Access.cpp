// IO/Access

#include "defs.hpp"

void define_julia_Access(jlcxx::Module &mod) {
  mod.add_bits<Access>("Access", jlcxx::julia_type("CppEnum"));
  jlcxx::stl::apply_stl<Access>(mod);

  mod.set_const("READ_ONLY", Access::READ_ONLY);
  mod.set_const("READ_WRITE", Access::READ_WRITE);
  mod.set_const("CREATE", Access::CREATE);
}
