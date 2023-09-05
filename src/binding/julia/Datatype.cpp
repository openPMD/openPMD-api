/* Bindings for Datatype
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#include "defs.hpp"

namespace
{
struct UseType
{
    template <typename T>
    static void call(jlcxx::Module &mod)
    {
        Datatype const dt = determineDatatype<T>();
        mod.set_const(datatypeToString(dt), dt);
    }
};
} // namespace

void define_julia_Datatype(jlcxx::Module &mod)
{
    mod.add_bits<Datatype>("Datatype", jlcxx::julia_type("CppEnum"));
    jlcxx::stl::apply_stl<Datatype>(mod);

    forallJuliaTypes<UseType>(mod);

    mod.set_const("UNDEFINED", Datatype::UNDEFINED);

    // mod.method("openpmd_datatypes", openPMD_datatypes);
    // mod.method("determine_datatype", determineDatatype);
    mod.method("cxx_to_bytes", toBytes);
    mod.method("cxx_to_bits", toBits);
    mod.method("cxx_is_vector", isVector);
    mod.method(
        "cxx_is_floating_point",
        static_cast<bool (*)(Datatype)>(isFloatingPoint));
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
