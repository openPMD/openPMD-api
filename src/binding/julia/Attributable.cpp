// Attributable

#include "defs.hpp"

void define_julia_Attributable(jlcxx::Module &mod)
{
    auto type = mod.add_type<Attributable>("CXX_Attributable");

#define USE_TYPE(NAME, ENUM, TYPE)                                             \
    type.method(                                                               \
        "cxx_set_attribute_" NAME "!", &Attributable::setAttribute<TYPE>);
    {
        FORALL_OPENPMD_TYPES(USE_TYPE)
    }
#undef USE_TYPE

    type.method("cxx_get_attribute", &Attributable::getAttribute);
    type.method("cxx_delete_attribute!", &Attributable::deleteAttribute);
    type.method("cxx_attributes", &Attributable::attributes);
    type.method("cxx_num_attributes", &Attributable::numAttributes);
    type.method("cxx_contains_attribute", &Attributable::containsAttribute);
    type.method("cxx_comment", &Attributable::comment);
    type.method("cxx_set_comment!", &Attributable::setComment);
    type.method(
        "cxx_series_flush",
        static_cast<void (Attributable::*)()>(&Attributable::seriesFlush));
}
