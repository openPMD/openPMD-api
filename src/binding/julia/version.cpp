// version

#include "defs.hpp"

void define_julia_version(jlcxx::Module &mod) {
  mod.method("get_version", getVersion);
  mod.method("get_standard", getStandard);
  mod.method("get_standard_minimum", getStandardMinimum);
  mod.method("get_variants1",
             []() { return map_to_vector_pair(getVariants()); });
  mod.method("get_file_extensions", getFileExtensions);
}
