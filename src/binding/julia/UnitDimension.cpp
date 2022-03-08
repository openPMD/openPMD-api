// IO/UnitDimension

#include "defs.hpp"

void define_julia_UnitDimension(jlcxx::Module &mod)
{
    mod.add_bits<UnitDimension>("UnitDimension", jlcxx::julia_type("CppEnum"));
    jlcxx::stl::apply_stl<UnitDimension>(mod);

    mod.set_const("UNITDIMENSION_L", UnitDimension::L);
    mod.set_const("UNITDIMENSION_M", UnitDimension::M);
    mod.set_const("UNITDIMENSION_T", UnitDimension::T);
    mod.set_const("UNITDIMENSION_I", UnitDimension::I);
    mod.set_const("UNITDIMENSION_θ", UnitDimension::theta);
    mod.set_const("UNITDIMENSION_N", UnitDimension::N);
    mod.set_const("UNITDIMENSION_J", UnitDimension::J);
}
