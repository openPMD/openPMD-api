// Datatype

#include "defs.hpp"

void define_julia_Datatype(jlcxx::Module &mod)
{
    mod.add_bits<Datatype>("Datatype", jlcxx::julia_type("CppEnum"));
    jlcxx::stl::apply_stl<Datatype>(mod);

#define USE_TYPE(NAME, ENUM, TYPE) mod.set_const(NAME, ENUM);
    {FORALL_OPENPMD_TYPES(USE_TYPE)}
#undef USE_TYPE
    mod.set_const("UNDEFINED", Datatype::UNDEFINED);

    mod.set_const("openPMD_datatypes", openPMD_Datatypes);
    // mod.method("determine_datatype", determineDatatype);
    mod.method("cxx_to_bytes", toBytes);
    mod.method("cxx_to_bits", toBits);
    mod.method("cxx_is_vector", isVector);
    mod.method("cxx_is_floating_point", (bool (*)(Datatype))isFloatingPoint);
    mod.method(
        "cxx_is_complex_floating_point",
        static_cast<bool (*)(Datatype)>(isComplexFloatingPoint));
    mod.method(
        "cxx_is_integer", (std::tuple<bool, bool>(*)(Datatype))isInteger);
    // isSameFloatingPoint
    // isSameComplexFloatingPoint
    // isSameInteger
    mod.method("cxx_is_same", isSame);
    mod.method("cxx_basic_datatype", basicDatatype);
    mod.method("cxx_to_vector_type", toVectorType);
    mod.method("cxx_datatype_to_string", datatypeToString);
    mod.method("cxx_string_to_datatype", stringToDatatype);
    mod.method("cxx_warn_wrong_datatype", warnWrongDtype);
    // mod.method("==", operator==);
    // mod.method("!=", operator!=);
}
