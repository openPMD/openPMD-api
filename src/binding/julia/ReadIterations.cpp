// ReadIterations

#include "defs.hpp"

#include <cstdint>

void define_julia_ReadIterations(jlcxx::Module &mod) {
  using iterations_t = Container<Iteration, uint64_t>;
  using key_type = typename iterations_t::key_type;
  using mapped_type = typename iterations_t::mapped_type;

  auto type = mod.add_type<ReadIterations>("ReadIterations");
  // TODO: SeriesIterator
  // TODO: begin, end
}
