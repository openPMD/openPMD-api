// Datatype

#include "defs.hpp"

void define_julia_Datatype(jlcxx::Module &mod) {
  mod.add_bits<Datatype>("Datatype", jlcxx::julia_type("CppEnum"));
  jlcxx::stl::apply_stl<Datatype>(mod);

#define USE_TYPE(NAME, ENUM, TYPE) mod.set_const(NAME, ENUM);
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
  mod.set_const("DATATYPE", Datatype::DATATYPE);
  mod.set_const("UNDEFINED", Datatype::UNDEFINED);

  mod.set_const("openPMD_datatypes", openPMD_Datatypes);
  // mod.method("determine_datatype", determineDatatype);
  mod.method("to_bytes1", toBytes);
  mod.method("to_bits1", toBits);
  mod.method("is_vector1", isVector);
  mod.method("is_floating_point1", (bool (*)(Datatype))isFloatingPoint);
  mod.method("is_complex_floating_point1",
             static_cast<bool (*)(Datatype)>(isComplexFloatingPoint));
  mod.method("is_integer1", (std::tuple<bool, bool>(*)(Datatype))isInteger);
  // isSameFloatingPoint
  // isSameComplexFloatingPoint
  // isSameInteger
  mod.method("is_same1", isSame);
  mod.method("basic_datatype1", basicDatatype);
  mod.method("to_vector_type1", toVectorType);
  mod.method("datatype_to_string1", datatypeToString);
  mod.method("string_to_datatype1", stringToDatatype);
  mod.method("warn_wrong_datatype1", warnWrongDtype);
  // mod.method("==", operator==);
  // mod.method("!=", operator!=);
}
