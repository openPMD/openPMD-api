// Attributable

#include "defs.hpp"

void define_julia_Attributable(jlcxx::Module &mod) {
  auto type = mod.add_type<Attributable>("Attributable");

#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("set_attribute1!", &Attributable::setAttribute<TYPE>);
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE

  type.method("get_attribute1", &Attributable::getAttribute);
  type.method("delete_attribute!", &Attributable::deleteAttribute);
  type.method("attributes", &Attributable::attributes);
  type.method("num_attributes1", &Attributable::numAttributes);
  type.method("contains_attribute", &Attributable::containsAttribute);
  type.method("comment", &Attributable::comment);
  type.method("set_comment!", &Attributable::setComment);
  type.method("series_flush", static_cast<void (Attributable::*)()>(
                                  &Attributable::seriesFlush));
}
