// Dataset

#include "defs.hpp"

void define_julia_Dataset(jlcxx::Module &mod) {
  auto type = mod.add_type<Dataset>("Dataset");

  type.constructor<Datatype, Extent>();
  type.constructor<Datatype, Extent, const std::string &>();
  type.constructor<Extent>();

  type.method("cxx_extend!", &Dataset::extend);
  // TODO type.method("cxx_set_chunk_size!", &Dataset::setChunkSize);
  // TODO type.method("set_compression!", &Dataset::setCompression);
  // TODO type.method("set_custom_transform!", &Dataset::setCustomTransform);
  type.method("cxx_extent", [](const Dataset &d) { return d.extent; });
  type.method("cxx_dtype", [](const Dataset &d) { return d.dtype; });
  type.method("cxx_rank", [](const Dataset &d) { return d.rank; });
  // TODO type.method("cxx_chunk_size", [](const Dataset &d) { return
  // d.chunkSize; });
  // TODO type.method("compression", [](const Dataset &d) { return
  // d.compression; });
  // TODO type.method("transform", [](const Dataset &d) { return d.transform;
  // });
  type.method("options", [](const Dataset &d) { return d.options; });
}
