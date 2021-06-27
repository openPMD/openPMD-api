// Dataset

#include "defs.hpp"

void define_julia_Dataset(jlcxx::Module &mod) {
  auto type = mod.add_type<Dataset>("Dataset");

  type.constructor<Datatype, Extent>();
  type.constructor<Datatype, Extent, const std::string &>();
  type.constructor<Extent>();

  type.method("extend!", &Dataset::extend);
  type.method("set_chunk_size!", &Dataset::setChunkSize);
  type.method("set_compression!", &Dataset::setCompression);
  type.method("set_custom_transform!", &Dataset::setCustomTransform);
  type.method("extent1", [](const Dataset &d) { return d.extent; });
  type.method("dtype1", [](const Dataset &d) { return d.dtype; });
  type.method("rank1", [](const Dataset &d) { return d.rank; });
  type.method("chunk_size1", [](const Dataset &d) { return d.chunkSize; });
  type.method("compression", [](const Dataset &d) { return d.compression; });
  type.method("transform", [](const Dataset &d) { return d.transform; });
  type.method("options", [](const Dataset &d) { return d.options; });
}
